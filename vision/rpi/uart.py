import serial
import threading


class ESP32Serial:
    def __init__(self, port="/dev/serial0", baud=115200):
        self.ser = serial.Serial(port, baud, timeout=0.1)
        self.latest_status = None
        self.ack_count = 0    # CMDs the ESP32 confirmed (bench firmware echoes "ACK l r f")
        self.last_sent = None
        self.holding = False  # gripper closed on something (our close command or the ESP32's AUTO GRAB)
        self._lock = threading.Lock()
        t = threading.Thread(target=self._read_loop, daemon=True)
        t.start()

    def send_cmd(self, left: int, right: int, flags: int = 0):
        msg = f"CMD {left} {right} {flags}\n"
        self.last_sent = (left, right, flags)
        with self._lock:
            self.ser.write(msg.encode())

    def send_servo(self, ch: int, deg: int):
        if ch == 3:
            self.holding = deg > 60       # anything past ~open counts as holding
        with self._lock:
            self.ser.write(f"SERVO {ch} {deg}\n".encode())

    def _read_loop(self):
        while True:
            try:
                line = self.ser.readline().decode(errors="ignore").strip()
                if line.startswith("ACK"):
                    self.ack_count += 1
                elif line == "AUTO GRAB":
                    self.holding = True
                    print("Auto-grab: got it")
                elif line.startswith("STATUS"):
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
