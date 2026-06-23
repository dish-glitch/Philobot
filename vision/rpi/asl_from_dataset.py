"""
Train ASL model from the Kaggle ASL Alphabet dataset.
Usage: python asl_from_dataset.py --dataset "X:\Philo training"
Saves asl_model.pkl in the same folder — drop-in replacement for the hand-collected model.
"""

import argparse
import math
import os
import pickle
import urllib.request
import cv2
import mediapipe as mp
from sklearn.neural_network import MLPClassifier

from asl_features import landmarks_to_features

SAMPLES_PER_CLASS = 500   # per letter — 13,000 total, ~5 min extraction
OUTPUT_MODEL      = "asl_model.pkl"
MODEL_PATH        = "hand_landmarker.task"
MODEL_URL         = (
    "https://storage.googleapis.com/mediapipe-models/"
    "hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
)

VALID_LETTERS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ")


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
        min_hand_detection_confidence=0.5,
    )
    return mp.tasks.vision.HandLandmarker.create_from_options(options)


def extract_features(img_path, landmarker):
    img = cv2.imread(img_path)
    if img is None:
        return None
    rgb    = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
    result = landmarker.detect(mp_img)
    if not result.hand_landmarks:
        return None
    return landmarks_to_features(result.hand_landmarks[0])


def find_train_dir(root):
    """Handle different zip extraction structures."""
    candidates = [
        root,
        os.path.join(root, "asl_alphabet_train"),
        os.path.join(root, "asl_alphabet_train", "asl_alphabet_train"),
        os.path.join(root, "archive", "asl_alphabet_train"),
    ]
    for path in candidates:
        if os.path.isdir(path):
            subdirs = os.listdir(path)
            if any(d.upper() in VALID_LETTERS for d in subdirs):
                return path
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset", required=True, help="Path to extracted dataset root")
    parser.add_argument("--samples", type=int, default=SAMPLES_PER_CLASS)
    args = parser.parse_args()

    download_model()
    landmarker = create_landmarker()

    train_dir = find_train_dir(args.dataset)
    if not train_dir:
        print(f"ERROR: Could not find A-Z letter folders in {args.dataset}")
        print("Make sure the zip was fully extracted and try again.")
        return

    print(f"Dataset found at: {train_dir}\n")

    letters = sorted([
        d for d in os.listdir(train_dir)
        if d.upper() in VALID_LETTERS and os.path.isdir(os.path.join(train_dir, d))
    ])

    print(f"{len(letters)} letters found: {', '.join(l.upper() for l in letters)}")
    print(f"Processing {args.samples} images per letter ({args.samples * len(letters):,} total)...\n")

    X, y = [], []
    total_skipped = 0

    for i, letter in enumerate(letters):
        folder = os.path.join(train_dir, letter)
        images = [f for f in os.listdir(folder) if f.lower().endswith((".jpg", ".jpeg", ".png"))]
        images = images[:args.samples]

        ok = 0
        for img_file in images:
            features = extract_features(os.path.join(folder, img_file), landmarker)
            if features:
                X.append(features)
                y.append(letter.upper())
                ok += 1
            else:
                total_skipped += 1

        print(f"  [{i+1:2d}/{len(letters)}] {letter.upper()}: {ok} samples  "
              f"({'skipped ' + str(len(images)-ok) if ok < len(images) else 'all ok'})")

    landmarker.close()

    print(f"\nExtraction complete: {len(X):,} samples  ({total_skipped} images had no detectable hand)")
    print("Training neural network classifier...")

    clf = MLPClassifier(
        hidden_layer_sizes=(256, 128),
        max_iter=500,
        random_state=42,
        verbose=True,
    )
    clf.fit(X, y)

    with open(OUTPUT_MODEL, "wb") as f:
        pickle.dump(clf, f)

    print(f"\nDone. Model saved to {OUTPUT_MODEL}")
    print("Run: python main.py --webcam")


if __name__ == "__main__":
    main()
