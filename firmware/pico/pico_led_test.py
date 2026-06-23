import sys
import machine
import time

led = machine.Pin(15, machine.Pin.OUT)
led.value(0)

time.sleep(1)  # let USB connection settle on boot
print("Pico LED ready")

while True:
    try:
        line = sys.stdin.readline()
        if not line:
            continue
        line = line.strip()
        if line.startswith("CMD"):
            parts = line.split()
            if len(parts) >= 3:
                left  = int(parts[1])
                right = int(parts[2])
                led.value(0 if (left == 0 and right == 0) else 1)
    except Exception:
        pass
