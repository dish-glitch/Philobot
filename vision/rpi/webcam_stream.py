"""
Laptop webcam -> MJPEG stream (run this ON THE LAPTOP).

Streams the laptop's webcam over the network so the Pi can use it as its camera
across the direct Ethernet link. Uses only OpenCV + the Python standard library.

Run on the laptop:
    python webcam_stream.py

Then on the Pi (over the direct link), point main.py at it:
    ~/philo-venv/bin/python main.py --source http://169.254.52.140:8080/video --serial /dev/ttyACM0

(Replace 169.254.52.140 with the laptop's Ethernet/link-local IP if different.)
Open http://localhost:8080/video in a laptop browser to preview the stream.
"""

import cv2
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PORT  = 8080
CAM   = 0            # laptop webcam index
WIDTH, HEIGHT = 640, 480
JPEG_QUALITY = 70

cap = cv2.VideoCapture(CAM)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)
if not cap.isOpened():
    raise SystemExit("Could not open the laptop webcam (index %d)." % CAM)


class MJPEGHandler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass  # quiet

    def do_GET(self):
        if self.path not in ("/video", "/"):
            self.send_response(404)
            self.end_headers()
            return
        self.send_response(200)
        self.send_header("Content-Type",
                         "multipart/x-mixed-replace; boundary=frame")
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        try:
            while True:
                ok, frame = cap.read()
                if not ok:
                    continue
                ok, jpg = cv2.imencode(".jpg", frame,
                                       [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
                if not ok:
                    continue
                data = jpg.tobytes()
                self.wfile.write(b"--frame\r\n")
                self.wfile.write(b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(data)}\r\n\r\n".encode())
                self.wfile.write(data)
                self.wfile.write(b"\r\n")
        except (BrokenPipeError, ConnectionResetError):
            pass  # client (the Pi) disconnected — fine


if __name__ == "__main__":
    print(f"Streaming laptop webcam on http://0.0.0.0:{PORT}/video")
    print("Preview in a browser: http://localhost:%d/video" % PORT)
    print("Press Ctrl+C to stop.")
    ThreadingHTTPServer(("0.0.0.0", PORT), MJPEGHandler).serve_forever()
