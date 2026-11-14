"""
Move the arm servos by hand to test each joint and find good angles.

  python arm_test.py --serial COM15      # ESP32 on the USB cable
  python arm_test.py --wifi              # laptop joined to WiFi "Philo-Robot"

Type:  <channel> <degrees>   e.g.  3 40    (gripper to 40 degrees)
       q                     quit
Channels (PCA9685): 0 = base, 1 = shoulder, 2 = elbow, 3 = gripper.
Start every new joint at 90 and move in small steps until you know its limits —
a servo pushed past what the arm allows will stall, get hot and can strip.
"""

import argparse
import http.client
import time

import serial

NAMES = {0: "base", 1: "shoulder", 2: "elbow", 3: "gripper"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--serial", help="ESP32 port over USB, e.g. COM15")
    ap.add_argument("--wifi", action="store_true", help="send over WiFi 'Philo-Robot' instead")
    args = ap.parse_args()
    if not args.serial and not args.wifi:
        ap.error("give --serial COMx (USB) or --wifi")

    ser = None
    if args.serial:
        ser = serial.Serial(args.serial, 115200, timeout=0)
        time.sleep(2.5)                   # ESP32 resets when the port opens

    def send(ch, deg):
        if ser:
            ser.write(f"SERVO {ch} {deg}\n".encode())
            return True
        conn = http.client.HTTPConnection("192.168.4.1", 80, timeout=1)
        try:
            conn.request("GET", f"/servo?ch={ch}&deg={deg}", headers={"Connection": "close"})
            conn.getresponse().read()
            return True
        except (OSError, http.client.HTTPException):
            print("  can't reach the robot — is the laptop on WiFi 'Philo-Robot'?")
            return False
        finally:
            conn.close()

    print(__doc__.split("Type:")[1].split("Start")[0].rstrip())
    while True:
        try:
            line = input("> ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            break
        if line in ("q", "quit", "exit"):
            break
        try:
            ch, deg = (int(x) for x in line.split())
        except ValueError:
            print("  type two numbers: <channel> <degrees>, e.g. 3 40")
            continue
        deg = max(0, min(180, deg))
        if send(ch, deg):
            print(f"  {NAMES.get(ch, 'channel ' + str(ch))} -> {deg} deg")
    if ser:
        ser.close()


if __name__ == "__main__":
    main()
