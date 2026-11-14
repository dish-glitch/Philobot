"""
Drive Philo with a game controller — no camera, no YOLO.
Controller -> laptop (USB or Bluetooth) -> ESP32 (USB cable) -> motors.

  python drive.py --serial COM15                 # USB cable to the ESP32
  python drive.py --wifi                         # wireless: laptop joined to WiFi "Philo-Robot"
  python drive.py --serial COM15 --min-speed 150 --max-speed 255

Left stick: up/down = forward/back, left/right = turn.  B / Circle (button 1) = stop.
Works with Xbox and PlayStation-style controllers (any SDL gamepad).
No controller plugged in? It opens a small window instead: click it, then
W/A/S/D or the arrow keys drive, Space brakes.
Ctrl+C, Esc or closing the window quits — the robot is stopped on exit.
"""

import argparse
import http.client
import os
import time

# Keep reading the controller even when this terminal isn't the focused window.
os.environ.setdefault("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1")
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


class SerialLink:
    """USB cable: the CMD text protocol."""
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=0)
        time.sleep(2.5)                   # ESP32 resets when the port opens

    def send(self, l, r, f):
        self.ser.reset_input_buffer()     # ignore STATUS/ACK chatter from the ESP32
        self.ser.write(f"CMD {l} {r} {f}\n".encode())

    def close(self):
        self.ser.close()


class WifiLink:
    """WiFi: the same values as GET /cmd on the ESP32's access point.
    The ESP32 web server closes the socket after every reply, so each command
    uses a fresh connection (reusing one made every other request fail)."""
    HOST = "192.168.4.1"

    def __init__(self):
        self.ok = None
        print(f"Sending to http://{self.HOST} — make sure this laptop is on WiFi 'Philo-Robot'.")

    def send(self, l, r, f):
        conn = http.client.HTTPConnection(self.HOST, 80, timeout=0.3)
        try:
            conn.request("GET", f"/cmd?l={l}&r={r}&f={f}", headers={"Connection": "close"})
            conn.getresponse().read()
            if self.ok is not True:
                print("Connected to the robot.")
            self.ok = True
        except (OSError, http.client.HTTPException):
            if self.ok is not False:
                print("Can't reach the robot — is the laptop on WiFi 'Philo-Robot'? Retrying...")
            self.ok = False
        finally:
            conn.close()

    def close(self):
        pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--serial", help="ESP32 port over USB, e.g. COM15")
    ap.add_argument("--wifi", action="store_true",
                    help="send over WiFi instead (ESP32 flashed with philo_drv8833_wifi; "
                         "join the laptop to WiFi 'Philo-Robot' first)")
    ap.add_argument("--min-speed", type=int, default=120, help="slowest speed that still moves the motors")
    ap.add_argument("--max-speed", type=int, default=255)
    args = ap.parse_args()
    if not args.serial and not args.wifi:
        ap.error("give --serial COMx (USB) or --wifi")

    pygame.init()
    pygame.joystick.init()
    js = None
    if pygame.joystick.get_count() > 0:
        js = pygame.joystick.Joystick(0)
        js.init()
        print(f"Controller: {js.get_name()}")
    else:
        print("No controller found — using the keyboard. Click the 'Philo drive' window, "
              "then W/A/S/D or arrows drive, Space brakes, Esc quits.")
        pygame.display.set_mode((360, 120))
        pygame.display.set_caption("Philo drive — WASD / arrows, Space = brake")

    if args.wifi:
        link = WifiLink()
    else:
        link = SerialLink(args.serial)
    print("Ready." + (" Left stick drives, B/Circle stops, Ctrl+C quits." if js else ""))

    def axis(i):
        v = js.get_axis(i)
        return 0.0 if abs(v) < DEADZONE else v

    def keyboard():
        """Return (throttle, steer, brake) from W/A/S/D or the arrow keys."""
        k = pygame.key.get_pressed()
        throttle = (k[pygame.K_w] or k[pygame.K_UP]) - (k[pygame.K_s] or k[pygame.K_DOWN])
        steer    = (k[pygame.K_d] or k[pygame.K_RIGHT]) - (k[pygame.K_a] or k[pygame.K_LEFT])
        return float(throttle), float(steer), bool(k[pygame.K_SPACE])

    last_print = 0.0
    try:
        while True:
            for ev in pygame.event.get():
                if ev.type == pygame.QUIT or (ev.type == pygame.KEYDOWN and ev.key == pygame.K_ESCAPE):
                    raise KeyboardInterrupt
            if js:
                throttle, steer = -axis(1), axis(0)   # stick up = forward
                brake = js.get_numbuttons() > 1 and js.get_button(1)
            else:
                throttle, steer, brake = keyboard()
            if brake:
                cmd = (0, 0, 2)            # brake
            else:
                l = max(-1.0, min(1.0, throttle + steer))
                r = max(-1.0, min(1.0, throttle - steer))
                cmd = (scale(l, args.min_speed, args.max_speed),
                       scale(r, args.min_speed, args.max_speed), 0)
            link.send(*cmd)
            if time.time() - last_print > 0.5:    # live readout: is the stick being seen?
                last_print = time.time()
                print(f"\rstick fwd {throttle:+.2f} turn {steer:+.2f}  ->  CMD {cmd[0]:4d} {cmd[1]:4d} {cmd[2]}   ",
                      end="", flush=True)
            time.sleep(1 / SEND_HZ)
    except KeyboardInterrupt:
        pass
    finally:
        link.send(0, 0, 1)
        link.close()
        print("Stopped.")


if __name__ == "__main__":
    main()
