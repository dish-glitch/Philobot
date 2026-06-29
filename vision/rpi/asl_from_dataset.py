"""
Train ASL model from the Kaggle ASL Alphabet dataset.
Usage: python asl_from_dataset.py --dataset "X:\Philo training"
Saves asl_model.pkl in the same folder — drop-in replacement for the hand-collected model.
"""

import argparse
import csv
import os
import pickle

from sklearn.neural_network import MLPClassifier

SAMPLES_PER_CLASS = 500   # per letter — 13,000 total, ~5 min extraction
OUTPUT_MODEL      = "asl_model.pkl"
FEATURES_CSV      = "asl_features.csv"   # version-independent extracted features


def _load_extraction_deps():
    """Heavy imports (cv2 + mediapipe) only needed when extracting from images."""
    global cv2, mp, urllib, landmarks_to_features
    import cv2
    import urllib.request
    import mediapipe as mp
    from asl_features import landmarks_to_features
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


def save_features_csv(X, y, path):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        for label, row in zip(y, X):
            w.writerow([label] + list(row))
    print(f"Saved features to {path}  ({len(X)} samples)")


def load_features_csv(path):
    X, y = [], []
    with open(path) as f:
        for parts in csv.reader(f):
            if not parts:
                continue
            y.append(parts[0])
            X.append([float(v) for v in parts[1:]])
    return X, y


def train_and_save(X, y):
    print(f"Training neural network on {len(X):,} samples...")
    clf = MLPClassifier(hidden_layer_sizes=(256, 128), max_iter=500,
                        random_state=42, verbose=True)
    clf.fit(X, y)
    with open(OUTPUT_MODEL, "wb") as f:
        pickle.dump(clf, f)
    print(f"\nDone. Model saved to {OUTPUT_MODEL} (built with THIS environment's library versions)")


def extract_from_dataset(dataset_root, samples):
    _load_extraction_deps()
    download_model()
    landmarker = create_landmarker()

    train_dir = find_train_dir(dataset_root)
    if not train_dir:
        print(f"ERROR: Could not find A-Z letter folders in {dataset_root}")
        return None, None

    print(f"Dataset found at: {train_dir}\n")
    letters = sorted([
        d for d in os.listdir(train_dir)
        if d.upper() in VALID_LETTERS and os.path.isdir(os.path.join(train_dir, d))
    ])
    print(f"{len(letters)} letters: {', '.join(l.upper() for l in letters)}")
    print(f"Processing {samples} images per letter...\n")

    X, y = [], []
    for i, letter in enumerate(letters):
        folder = os.path.join(train_dir, letter)
        images = [f for f in os.listdir(folder)
                  if f.lower().endswith((".jpg", ".jpeg", ".png"))][:samples]
        ok = 0
        for img_file in images:
            features = extract_features(os.path.join(folder, img_file), landmarker)
            if features:
                X.append(features); y.append(letter.upper()); ok += 1
        print(f"  [{i+1:2d}/{len(letters)}] {letter.upper()}: {ok} samples")

    landmarker.close()
    print(f"\nExtraction complete: {len(X):,} samples")
    return X, y


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset", help="Extract from image dataset, save features CSV, and train")
    parser.add_argument("--csv",     help="Train from a pre-extracted features CSV (no images needed)")
    parser.add_argument("--samples", type=int, default=SAMPLES_PER_CLASS)
    args = parser.parse_args()

    if args.csv:
        # Train-only: load version-independent features and build the model in THIS env.
        print(f"Loading features from {args.csv} ...")
        X, y = load_features_csv(args.csv)
        print(f"Loaded {len(X):,} samples.")
        train_and_save(X, y)
        return

    if not args.dataset:
        print("Give --dataset <path> (extract images + train) OR --csv <file> (train from features).")
        return

    X, y = extract_from_dataset(args.dataset, args.samples)
    if not X:
        return
    save_features_csv(X, y, FEATURES_CSV)   # portable features — copy this to another machine to retrain
    train_and_save(X, y)


if __name__ == "__main__":
    main()
