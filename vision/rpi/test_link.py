"""
Pi -> Arduino link test (no camera needed).
Auto-finds the Arduino and runs it through a follow / turn / stop sequence so you
can watch the LEDs + OLED react. Proves the Pi is driving the board.

Run on the Pi:
    ~/philo-venv/bin/python vision/rpi/test_link.py
"""

import glob
import sys
import time

import serial

ports = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")
if not ports:
    sys.exit("No Arduino found (no /dev/ttyACM* or /dev/ttyUSB*). Is it plugged in?")

port = ports[0]
print(f"Connecting to {port} ...")
s = serial.Serial(port, 115200, timeout=1)
time.sleep(2)  # Arduino resets when the port opens — wait for it to boot


def cmd(left, right, label):
    s.write(f"CMD {left} {right} 0\n".encode())
    print(f"  -> {label}")
    time.sleep(2.5)


print("\nWatch the breadboard LEDs + OLED:\n")
cmd(200, 200, "FOLLOW straight   (follow LED D6 ON, eyes look center)")
cmd(200, 110, "turn RIGHT        (right LED D8 ON, eyes look right)")
cmd(110, 200, "turn LEFT         (left LED D7 ON, eyes look left)")

s.write(b"CMD 0 0 1\n")
print("  -> STOP            (all LEDs OFF)")
time.sleep(0.5)
s.close()
print("\nDone. If the LEDs followed along, the Pi is driving the Arduino. ")
