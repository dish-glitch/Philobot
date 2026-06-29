import numpy as np
from ultralytics import YOLO

# COCO keypoint indices
LEFT_SHOULDER  = 5
RIGHT_SHOULDER = 6
LEFT_WRIST     = 9
RIGHT_WRIST    = 10


class PoseTracker:
    def __init__(self, model_path="yolov8n-pose.pt", imgsz=256, conf=0.5,
                 enable_objects=False):
        self.pose_model = YOLO(model_path)
        self.imgsz = imgsz
        self.conf  = conf
        # Object-labeling model is OPTIONAL and heavy — off by default so the Pi
        # only runs the lightweight pose model (huge speedup). Enable on a laptop.
        self.enable_objects = enable_objects
        self.detect_model = YOLO("yolov8s.pt") if enable_objects else None
        self.detect_conf = 0.35

    def infer(self, frame):
        """
        Returns:
          bbox   — [x1,y1,x2,y2] of largest detected person, or None
          kpts   — [17,2] keypoints for that person, or None
          labels — list of (x1,y1,x2,y2,name,conf) for non-person detections (empty if objects disabled)
        """
        # Pose model → person box + keypoints
        pose_results = self.pose_model(frame, imgsz=self.imgsz, conf=self.conf, verbose=False)
        person_bbox = None
        person_kpts = None

        if pose_results and pose_results[0].boxes is not None:
            boxes   = pose_results[0].boxes.xyxy.cpu().numpy()
            kpts    = pose_results[0].keypoints.xy.cpu().numpy() if pose_results[0].keypoints is not None else None
            if len(boxes):
                areas = [(b[2]-b[0])*(b[3]-b[1]) for b in boxes]
                best  = int(np.argmax(areas))
                person_bbox = boxes[best]
                if kpts is not None and best < len(kpts):
                    person_kpts = kpts[best]

        # Object detection — only if explicitly enabled (skipped on the Pi)
        labels = []
        if not self.enable_objects:
            return person_bbox, person_kpts, labels

        det_results = self.detect_model(frame, imgsz=640, conf=self.detect_conf, verbose=False)

        if det_results and det_results[0].boxes is not None:
            boxes   = det_results[0].boxes.xyxy.cpu().numpy()
            confs   = det_results[0].boxes.conf.cpu().numpy()
            cls_ids = det_results[0].boxes.cls.cpu().numpy().astype(int)
            names   = det_results[0].names
            for i, c in enumerate(cls_ids):
                if names[c] == "person":
                    continue
                x1, y1, x2, y2 = map(int, boxes[i])
                # Suppress objects whose center falls inside the person box —
                # almost always a misidentified hand/arm
                if person_bbox is not None:
                    cx = (x1 + x2) / 2
                    cy = (y1 + y2) / 2
                    px1, py1, px2, py2 = person_bbox
                    if px1 <= cx <= px2 and py1 <= cy <= py2:
                        continue
                labels.append((x1, y1, x2, y2, names[c], float(confs[i])))

        return person_bbox, person_kpts, labels

    def gesture_keypoints(self, kpts):
        """Return (lw_y, rw_y, ls_y, rs_y) or None if any keypoint is missing."""
        if kpts is None or len(kpts) < 11:
            return None
        pts = [kpts[LEFT_WRIST], kpts[RIGHT_WRIST], kpts[LEFT_SHOULDER], kpts[RIGHT_SHOULDER]]
        if any(p[0] == 0 and p[1] == 0 for p in pts):
            return None
        return pts[0][1], pts[1][1], pts[2][1], pts[3][1]
