"""
ASL data collection + training.
Press a letter key (a-z) to record that sign. Hold it steady.
Press Q when done — trains automatically.
Features come from asl_features.py, the same code main.py uses at runtime.
"""

import cv2
import csv
import os
import pickle
import urllib.request
import mediapipe as mp
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import SVC

from asl_features import landmarks_to_features

SAMPLES_PER_LETTER = 100  # more samples = better accuracy
DATA_FILE  = "asl_data.csv"
MODEL_FILE = "asl_model.pkl"
MODEL_PATH = "hand_landmarker.task"
MODEL_URL  = (
    "https://storage.googleapis.com/mediapipe-models/"
    "hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
)

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


def get_features(result):
    # Must match inference (main.py) exactly — use the shared feature code.
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
            cv2.circle(frame, pt, 5, (0, 255, 0), -1)
        for a, b in [(0,1),(1,2),(2,3),(3,4),(0,5),(5,6),(6,7),(7,8),
                     (0,9),(9,10),(10,11),(11,12),(0,13),(13,14),(14,15),(15,16),
                     (0,17),(17,18),(18,19),(19,20),(5,9),(9,13),(13,17)]:
            cv2.line(frame, pts[a], pts[b], (0, 200, 0), 2)


def main():
    download_model()

    cap        = cv2.VideoCapture(0)
    landmarker = create_landmarker()
    collected  = {}
    active     = None

    print("Press a letter key to start recording. Press Q to train & quit.")

    while True:
        ok, frame = cap.read()
        if not ok:
            break

        # Detect on the UNflipped frame, like main.py does, so left/right match;
        # mirror only the picture shown on screen.
        rgb    = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
        result = landmarker.detect(mp_img)

        draw_hand(frame, result)
        frame = cv2.flip(frame, 1)

        if active:
            row = get_features(result)
            if row:
                collected.setdefault(active, [])
                if len(collected[active]) < SAMPLES_PER_LETTER:
                    collected[active].append(row)
                if len(collected[active]) >= SAMPLES_PER_LETTER:
                    print(f"  {active.upper()} done.")
                    active = None

        if active:
            count = len(collected.get(active, []))
            cv2.putText(frame, f"Recording {active.upper()}: {count}/{SAMPLES_PER_LETTER}",
                        (10, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        else:
            done = ", ".join(k.upper() for k in collected) or "none"
            cv2.putText(frame, f"Done: {done}", (10, 35),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, "Press a-z to record  |  Q to train & quit",
                        (10, 65), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (200, 200, 200), 1)

        cv2.imshow("ASL Collect", frame)
        key = cv2.waitKey(30) & 0xFF
        if key == ord('q'):
            break
        elif 97 <= key <= 122:
            active = chr(key)
            collected.pop(active, None)
            print(f"Recording: {active.upper()} — hold sign steady")

    cap.release()
    cv2.destroyAllWindows()
    landmarker.close()

    if len(collected) < 2:
        print("Need at least 2 different letters to train. Nothing saved.")
        return

    with open(DATA_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        for letter, rows in collected.items():
            for row in rows:
                writer.writerow([letter] + row)

    print(f"\nTraining on {sum(len(v) for v in collected.values())} samples...")
    X, y = [], []
    for letter, rows in collected.items():
        for row in rows:
            X.append(row)
            y.append(letter)

    clf = SVC(kernel="rbf", probability=True, C=10, gamma="scale")
    clf.fit(X, y)

    with open(MODEL_FILE, "wb") as f:
        pickle.dump(clf, f)

    print(f"Done. Now run: python main.py --webcam")


if __name__ == "__main__":
    main()
