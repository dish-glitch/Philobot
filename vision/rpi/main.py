"""
Philo combined vision stack:
  - Person following   (YOLOv8n-pose → bounding box)
  - Gesture stop       (wrist above shoulder for 5 frames)
  - ASL recognition    (MediaPipe hands → trained classifier)

Usage:
  python main.py --webcam                     laptop, no hardware
  python main.py --webcam --serial COM3       laptop + Arduino
  python main.py                              Pi camera
  python main.py --serial /dev/serial0        Pi + ESP32

Controls: Q to quit (window must be focused)
"""

import argparse
import math
import os
import pickle
import time
import urllib.request

import cv2

# MediaPipe is only needed for ASL. It has no wheel for some Python versions
# (e.g. 3.13 on the Pi), so make it optional — person-following works without it.
try:
    import mediapipe as mp
    HAS_MEDIAPIPE = True
except ImportError:
    HAS_MEDIAPIPE = False

from tracker import PoseTracker
from controller import PhiloController
from asl_features import landmarks_to_features

FRAME_W, FRAME_H = 640, 480
ASL_SKIP        = 2          # run ASL every N frames (keeps loop fast)
ASL_CONFIRM     = 5          # frames same letter must appear before sending
ASL_COOLDOWN    = 2.0        # seconds before re-sending same letter

MODEL_PATH = "hand_landmarker.task"
MODEL_URL  = (
    "https://storage.googleapis.com/mediapipe-models/"
    "hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
)
ASL_MODEL_FILE = "asl_model.pkl"


# ── camera ────────────────────────────────────────────────────────────────────

def open_camera(use_webcam, source=None):
    # --source: a stream URL (e.g. http://169.254.x.x:8080/video) or a device index
    if source is not None:
        src = int(source) if str(source).isdigit() else source
        return cv2.VideoCapture(src)
    if use_webcam:
        cap = cv2.VideoCapture(0)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)
        return cap
    from picamera2 import Picamera2
    cam = Picamera2()
    cam.configure(cam.create_preview_configuration(
        main={"format": "RGB888", "size": (FRAME_W, FRAME_H)}
    ))
    cam.start()
    return cam


def grab_frame(cap, use_webcam):
    if use_webcam:
        ok, frame = cap.read()
        return frame if ok else None
    return cap.capture_array()


# ── ASL helpers ───────────────────────────────────────────────────────────────

def download_hand_model():
    if not os.path.exists(MODEL_PATH):
        print("Downloading hand landmarker model (~25MB)...")
        urllib.request.urlretrieve(MODEL_URL, MODEL_PATH)
        print("Done.")


def create_landmarker():
    options = mp.tasks.vision.HandLandmarkerOptions(
        base_options=mp.tasks.BaseOptions(model_asset_path=MODEL_PATH),
        running_mode=mp.tasks.vision.RunningMode.IMAGE,
        num_hands=1,
        min_hand_detection_confidence=0.7,
    )
    return mp.tasks.vision.HandLandmarker.create_from_options(options)


def hand_features(result):
    if not result.hand_landmarks:
        return None
    return landmarks_to_features(result.hand_landmarks[0])


def draw_hand(frame, result):
    if not result.hand_landmarks:
        return
    h, w = frame.shape[:2]
    for hand in result.hand_landmarks:
        pts = [(int(lm.x * w), int(lm.y * h)) for lm in hand]
        for pt in pts:
            cv2.circle(frame, pt, 4, (255, 100, 0), -1)
        for a, b in [(0,1),(1,2),(2,3),(3,4),(0,5),(5,6),(6,7),(7,8),
                     (0,9),(9,10),(10,11),(11,12),(0,13),(13,14),(14,15),(15,16),
                     (0,17),(17,18),(18,19),(19,20),(5,9),(9,13),(13,17)]:
            cv2.line(frame, pts[a], pts[b], (200, 80, 0), 2)


# ── overlay ───────────────────────────────────────────────────────────────────

OBSTACLE_CM = 20   # matches the Arduino's OBSTACLE_CM


def draw_overlay(frame, bbox, labels, stopped, asl_letter, status=None):
    if bbox is not None:
        x1, y1, x2, y2 = map(int, bbox)
        color = (0, 0, 255) if stopped else (0, 255, 0)
        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)

    for (lx1, ly1, lx2, ly2, name, conf) in labels:
        cv2.rectangle(frame, (lx1, ly1), (lx2, ly2), (255, 165, 0), 2)
        cv2.putText(frame, f"{name} {conf:.0%}", (lx1, ly1 - 6),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 165, 0), 2)

    status_text = "GESTURE STOP" if stopped else "FOLLOWING"
    cv2.putText(frame, status_text, (10, 32),
                cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 255), 2)

    if asl_letter:
        cv2.putText(frame, f"ASL: {asl_letter}", (10, 70),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.2, (255, 80, 200), 3)

    # ── telemetry returned from the ESP32/Arduino (closes the loop) ──
    if status is not None:
        dc = status.get("dc", 999)
        vbat = status.get("vbat", 0)
        cv2.putText(frame, f"ESP dist: {int(dc)}cm   vbat: {vbat:.1f}V",
                    (10, frame.shape[0] - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 2)
        if 0 < dc < OBSTACLE_CM:
            cv2.putText(frame, "!! OBSTACLE - ROBOT OVERRIDE STOP !!",
                        (10, frame.shape[0] - 45),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)


# ── main ──────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--webcam",     action="store_true")
    parser.add_argument("--source",     default=None,
                        help="camera source: a stream URL (e.g. http://169.254.x.x:8080/video) or device index")
    parser.add_argument("--serial",     default=None)
    parser.add_argument("--no-display", action="store_true")
    parser.add_argument("--objects",    action="store_true",
                        help="also run object-labeling model (heavy — laptop only, off on Pi)")
    args = parser.parse_args()

    # use OpenCV-style capture (cap.read + BGR) for webcam or any --source; else Pi camera
    use_cv = args.webcam or (args.source is not None)

    # Serial (optional)
    uart = None
    if args.serial:
        from uart import ESP32Serial
        uart = ESP32Serial(port=args.serial)
        print(f"Serial connected on {args.serial}")
    else:
        print("No serial — running vision only")

    # ASL model (optional — skipped if not trained yet)
    asl_model  = None
    landmarker = None
    if not HAS_MEDIAPIPE:
        print("MediaPipe not installed — ASL disabled, running person-following only.")
    elif os.path.exists(ASL_MODEL_FILE):
        download_hand_model()
        landmarker = create_landmarker()
        with open(ASL_MODEL_FILE, "rb") as f:
            asl_model = pickle.load(f)
        print("ASL model loaded.")
    else:
        print("No ASL model found — run asl_collect.py first to enable ASL.")

    cap     = open_camera(args.webcam, args.source)
    tracker = PoseTracker(enable_objects=args.objects)
    ctrl    = PhiloController()

    fps_t0, fps_n = time.time(), 0

    frame_count    = 0
    asl_last       = None
    asl_confirm    = 0
    asl_sent       = None
    asl_sent_time  = 0
    asl_display    = None   # letter shown on screen

    print("Running. Press Q to quit.")

    while True:
        frame = grab_frame(cap, use_cv)
        if frame is None:
            break

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB) if use_cv else frame
        frame_count += 1

        # FPS readout every ~3s
        fps_n += 1
        if time.time() - fps_t0 >= 3.0:
            print(f"FPS: {fps_n / (time.time() - fps_t0):.1f}")
            fps_t0, fps_n = time.time(), 0

        # ── person following + gesture stop ──
        bbox, kpts, labels = tracker.infer(rgb)
        kpt_ys  = tracker.gesture_keypoints(kpts)
        stopped = ctrl.update_gesture(kpt_ys)

        if uart:
            if stopped:
                uart.send_cmd(0, 0, 0)
            else:
                l, r, f = ctrl.compute_cmd(bbox, FRAME_W)
                uart.send_cmd(l, r, f)

        # ── ASL (every ASL_SKIP frames) ──
        if asl_model and frame_count % ASL_SKIP == 0:
            mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
            result = landmarker.detect(mp_img)

            if not args.no_display:
                draw_hand(frame if use_cv else cv2.cvtColor(frame, cv2.COLOR_RGB2BGR),
                          result)

            row = hand_features(result)
            if row:
                pred = asl_model.predict([row])[0]
                conf = max(asl_model.predict_proba([row])[0])
                print(f"[ASL debug] pred={pred.upper()} conf={conf:.2f}")  # remove later
                if conf > 0.5:
                    if pred == asl_last:
                        asl_confirm += 1
                    else:
                        asl_last    = pred
                        asl_confirm = 1

                    if asl_confirm >= ASL_CONFIRM:
                        now = time.time()
                        if pred != asl_sent or (now - asl_sent_time) > ASL_COOLDOWN:
                            asl_display   = pred.upper()
                            asl_sent      = pred
                            asl_sent_time = now
                            print(f"ASL → {pred.upper()}")
                            if uart:
                                # reuse serial connection to send ASL
                                uart.ser.write(f"ASL {pred.upper()}\n".encode())
            else:
                asl_last    = None
                asl_confirm = 0

        # ── display ──
        if not args.no_display:
            vis = frame if use_cv else cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            status = uart.get_status() if uart else None
            draw_overlay(vis, bbox, labels, stopped, asl_display, status)
            cv2.imshow("Philo", vis)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    if uart:
        uart.send_cmd(0, 0, 0)
        uart.close()
    if use_cv:
        cap.release()
    else:
        cap.stop()
    if landmarker:
        landmarker.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
