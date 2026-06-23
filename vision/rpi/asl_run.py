"""
ASL recognition → terminal (and optionally Arduino serial)
Usage:
  python asl_run.py
  python asl_run.py --serial COM3
"""

import argparse
import os
import pickle
import time
import urllib.request
import cv2
import mediapipe as mp

MODEL_PATH     = "hand_landmarker.task"
MODEL_URL      = (
    "https://storage.googleapis.com/mediapipe-models/"
    "hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
)
ASL_MODEL_FILE = "asl_model.pkl"
CONFIRM_FRAMES = 10
SEND_COOLDOWN  = 2.0


def download_model():
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


def get_landmarks(result):
    if not result.hand_landmarks:
        return None
    hand  = result.hand_landmarks[0]
    wrist = hand[0]
    row   = []
    for lm in hand:
        row.extend([lm.x - wrist.x, lm.y - wrist.y])
    return row


def draw_hand(frame, result):
    if not result.hand_landmarks:
        return
    h, w = frame.shape[:2]
    for hand in result.hand_landmarks:
        pts = [(int(lm.x * w), int(lm.y * h)) for lm in hand]
        for pt in pts:
            cv2.circle(frame, pt, 5, (0, 255, 0), -1)
        connections = [
            (0,1),(1,2),(2,3),(3,4),
            (0,5),(5,6),(6,7),(7,8),
            (0,9),(9,10),(10,11),(11,12),
            (0,13),(13,14),(14,15),(15,16),
            (0,17),(17,18),(18,19),(19,20),
            (5,9),(9,13),(13,17),
        ]
        for a, b in connections:
            cv2.line(frame, pts[a], pts[b], (0, 200, 0), 2)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial", default=None, help="Arduino COM port, e.g. COM3")
    args = parser.parse_args()

    download_model()

    with open(ASL_MODEL_FILE, "rb") as f:
        model = pickle.load(f)

    ser = None
    if args.serial:
        import serial
        ser = serial.Serial(args.serial, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {args.serial}")

    landmarker     = create_landmarker()
    cap            = cv2.VideoCapture(0)
    last_letter    = None
    confirm_count  = 0
    last_sent      = None
    last_send_time = 0

    print("Running — sign a letter at the camera. Press Q to quit.")

    while True:
        ok, frame = cap.read()
        if not ok:
            break

        frame  = cv2.flip(frame, 1)
        rgb    = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
        result = landmarker.detect(mp_img)

        draw_hand(frame, result)

        detected = None
        row = get_landmarks(result)
        if row:
            pred = model.predict([row])[0]
            conf = max(model.predict_proba([row])[0])
            if conf > 0.75:
                detected = pred
                cv2.putText(frame, f"{pred.upper()}  {conf:.0%}",
                            (10, 45), cv2.FONT_HERSHEY_SIMPLEX, 1.4, (0, 255, 0), 3)

        if detected == last_letter:
            confirm_count += 1
        else:
            last_letter   = detected
            confirm_count = 1

        if detected and confirm_count >= CONFIRM_FRAMES:
            now = time.time()
            if detected != last_sent or (now - last_send_time) > SEND_COOLDOWN:
                print(f"  → {detected.upper()}")
                if ser:
                    ser.write(f"ASL {detected.upper()}\n".encode())
                last_sent      = detected
                last_send_time = now

        cv2.imshow("Philo ASL", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    landmarker.close()
    if ser:
        ser.close()


if __name__ == "__main__":
    main()
