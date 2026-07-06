"""
Measure Philo's real vision performance on THIS machine (no Arduino needed).

    ~/philo312/bin/python benchmark.py --webcam --frames 300
    ~/philo312/bin/python benchmark.py --webcam --frames 300 --asl   # also time MediaPipe

Prints an FPS + per-stage latency report you can paste straight into RESULTS.md.
Methodology: warm up 10 frames, then time grab / YOLO-pose / (MediaPipe) / total
over --frames frames and report mean, median, min, max.
"""

import argparse
import statistics
import time

import cv2
from tracker import PoseTracker

# Optional MediaPipe timing (needs the 3.12 env)
try:
    import mediapipe as mp
    from main import create_landmarker, download_hand_model
    HAS_MP = True
except Exception:
    HAS_MP = False


def open_cam(webcam, source):
    if source is not None:
        return cv2.VideoCapture(int(source) if str(source).isdigit() else source)
    return cv2.VideoCapture(0)


def stats(a):
    return statistics.mean(a), statistics.median(a), min(a), max(a)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--webcam", action="store_true")
    ap.add_argument("--source", default=None)
    ap.add_argument("--frames", type=int, default=300)
    ap.add_argument("--asl", action="store_true", help="also time MediaPipe hand landmarks")
    args = ap.parse_args()

    cap = open_cam(args.webcam, args.source)
    if not cap.isOpened():
        raise SystemExit("Could not open camera.")

    tracker = PoseTracker(enable_objects=False)
    landmarker = None
    if args.asl:
        if not HAS_MP:
            raise SystemExit("--asl requested but MediaPipe isn't available in this env.")
        download_hand_model()
        landmarker = create_landmarker()

    print("Warming up...")
    for _ in range(10):
        cap.read()

    grab_t, pose_t, mp_t, loop_t = [], [], [], []
    print(f"Timing {args.frames} frames...")
    n = 0
    while n < args.frames:
        t0 = time.perf_counter()
        ok, frame = cap.read()
        if not ok:
            continue
        t1 = time.perf_counter()

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        tracker.infer(rgb)
        t2 = time.perf_counter()

        if landmarker is not None:
            mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
            landmarker.detect(mp_img)
        t3 = time.perf_counter()

        grab_t.append((t1 - t0) * 1000)
        pose_t.append((t2 - t1) * 1000)
        mp_t.append((t3 - t2) * 1000)
        loop_t.append((t3 - t0) * 1000)
        n += 1

    cap.release()

    fps = 1000.0 / statistics.mean(loop_t)
    print(f"\n=== RESULT ({n} frames) ===")
    print(f"Sustained FPS (mean): {fps:.1f}")
    print(f"{'stage':14s} {'mean_ms':>8s} {'median':>8s} {'min':>7s} {'max':>7s}")
    rows = [("grab", grab_t), ("yolo-pose", pose_t)]
    if landmarker is not None:
        rows.append(("mediapipe", mp_t))
    rows.append(("loop total", loop_t))
    for name, arr in rows:
        m, md, lo, hi = stats(arr)
        print(f"{name:14s} {m:8.1f} {md:8.1f} {lo:7.1f} {hi:7.1f}")


if __name__ == "__main__":
    main()
