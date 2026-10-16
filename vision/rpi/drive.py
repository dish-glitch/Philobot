"""
Drive Philo with a game controller — no camera, no YOLO.
Controller -> laptop (USB or Bluetooth) -> ESP32 (USB cable) -> motors.

  python drive.py --serial COM15
  python drive.py --serial COM15 --min-speed 150 --max-speed 255

Left stick: up/down = forward/back, left/right = turn.  B / Circle (button 1) = stop.
Works with Xbox and PlayStation-style controllers (any SDL gamepad).
Ctrl+C or close the window to quit — the robot is stopped on exit.
"""

import argparse
import time

import pygame
import serial

DEADZONE = 0.15
SEND_HZ  = 20          # well inside the ESP32's 500 ms watchdog


def scale(v, min_speed, max_speed):
    """Stick value -1..1 -> wheel speed, skipping the band where the motors only hum."""
    if v == 0:
        return 0
    s = min_speed + abs(v) * (max_speed - min_speed)
    return int(s if v > 0 else -s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--serial", required=True, help="ESP32 port, e.g. COM15")
    ap.add_argument("--min-speed", type=int, default=120, help="slowest speed that still moves the motors")
    ap.add_argument("--max-speed", type=int, default=255)
    args = ap.parse_args()

    pygame.init()
    pygame.joystick.init()
    if pygame.joystick.get_count() == 0:
        raise SystemExit("No controller found. Plug it in (USB) or pair it over Bluetooth, then rerun.")
    js = pygame.joystick.Joystick(0)
    js.init()
    print(f"Controller: {js.get_name()}")

    ser = serial.Serial(args.serial, 115200, timeout=0)
    time.sleep(2.5)                       # ESP32 resets when the port opens
    print("Ready. Left stick drives, B/Circle stops, Ctrl+C quits.")

    def axis(i):
        v = js.get_axis(i)
        return 0.0 if abs(v) < DEADZONE else v

    try:
        while True:
            pygame.event.pump()
            ser.reset_input_buffer()      # ignore STATUS/ACK chatter from the ESP32
            if js.get_numbuttons() > 1 and js.get_button(1):
                ser.write(b"CMD 0 0 2\n")  # brake
            else:
                throttle = -axis(1)        # stick up = forward
                steer    = axis(0)
                l = max(-1.0, min(1.0, throttle + steer))
                r = max(-1.0, min(1.0, throttle - steer))
                ser.write(f"CMD {scale(l, args.min_speed, args.max_speed)} "
                          f"{scale(r, args.min_speed, args.max_speed)} 0\n".encode())
            time.sleep(1 / SEND_HZ)
    except KeyboardInterrupt:
        pass
    finally:
        ser.write(b"CMD 0 0 1\n")
        ser.close()
        print("Stopped.")


if __name__ == "__main__":
    main()
