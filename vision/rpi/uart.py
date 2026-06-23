import serial
import threading


class ESP32Serial:
    def __init__(self, port="/dev/serial0", baud=115200):
        self.ser = serial.Serial(port, baud, timeout=0.1)
        self.latest_status = None
        self._lock = threading.Lock()
        t = threading.Thread(target=self._read_loop, daemon=True)
        t.start()

    def send_cmd(self, left: int, right: int, flags: int = 0):
        msg = f"CMD {left} {right} {flags}\n"
        with self._lock:
            self.ser.write(msg.encode())

    def _read_loop(self):
        while True:
            try:
                line = self.ser.readline().decode().strip()
                if line.startswith("STATUS"):
                    parts = line.split()
                    if len(parts) == 11:
                        keys = ["vbat", "dl", "dc", "dr", "el", "er", "ax", "ay", "az", "gz"]
                        self.latest_status = dict(zip(keys, map(float, parts[1:])))
            except Exception:
                pass

    def get_status(self):
        return self.latest_status

    def close(self):
        self.ser.close()
