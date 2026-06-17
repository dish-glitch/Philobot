# Firmware — Philo ESP32

**Owner: [@dish-glitch](https://github.com/dish-glitch)**

---

## Overview

The ESP32 firmware handles everything physical — spinning motors, reading encoders, polling ultrasonic sensors, and listening for commands from the Raspberry Pi. It does not know about people or cameras. It just moves wheels and avoids walls.

Built with PlatformIO. Framework: Arduino (simpler than ESP-IDF for this use case, sufficient for our timing requirements).

---

## Responsibilities

| Task | How |
|---|---|
| Receive direction + speed commands | UART from Raspberry Pi at 115200 baud |
| Drive motors | PWM via ESP32 LEDC peripheral, 20kHz frequency |
| Control speed closed-loop | PID per motor side using encoder pulse counting |
| Read ultrasonic sensors | Timed pulse on trigger, measure echo pulse width |
| Override Pi commands on obstacle | Priority layer — safety above navigation |
| Detect tip-over | Read MPU-6050 pitch angle over I2C |
| Watchdog | Stop motors if no command received in 500ms |

---

## Serial Protocol (UART — Raspberry Pi to ESP32)

**Baud rate:** 115200  
**Format:** `CMD:<direction>,<speed>\n`

| Direction Code | Meaning |
|---|---|
| `F` | Forward |
| `B` | Backward |
| `L` | Turn left |
| `R` | Turn right |
| `S` | Stop |

**Speed:** 0–255 (maps to PWM duty cycle, 0 = stopped, 255 = full speed)

**Examples:**
```
CMD:F,180\n    → drive forward at ~70% speed
CMD:L,120\n    → turn left at ~47% speed
CMD:S,0\n      → stop
```

The ESP32 parses each line as it arrives. If a line is malformed or the watchdog timer expires (no command for 500ms), motors stop immediately.

---

## Motor Control

**4 motors, 2 sides:**
- Left side: left-front + left-rear motors wired in parallel to one TB6612FNG channel
- Right side: right-front + right-rear motors wired in parallel to the other channel

**PWM:** ESP32 LEDC peripheral, 20kHz. Human ears hear up to ~20kHz — running PWM at exactly 20kHz puts motor whine right at the edge of hearing. In practice it is nearly silent compared to lower frequencies.

**PID speed control per side:**
Encoders count pulses per unit time. The PID loop compares actual speed (encoder pulses/sec) to the target speed from the Pi command, and adjusts PWM duty cycle to close the gap.

```
error = target_speed - measured_speed
output = Kp * error + Ki * integral + Kd * derivative
clamp output to 0-255
write to PWM
```

Tune Kp first (increase until robot responds without oscillating), then Ki (removes steady-state error), Kd last (dampens overshoot). Start with Kp=1.0, Ki=0.1, Kd=0.05 and adjust from there.

Each side has its own PID instance. This is what keeps the robot driving straight instead of drifting — if one motor is slightly faster than the other, the PID corrects it automatically.

---

## Ultrasonic Sensor Logic

Three HC-SR04 sensors: front-left, front-center, front-right.

**Reading a sensor (one sensor at a time, polled in sequence):**
```
1. Pull trigger pin HIGH for 10 microseconds
2. Pull trigger pin LOW
3. Measure how long echo pin stays HIGH
4. distance_cm = echo_duration_us / 58.0
```

Poll all three sensors every 50ms (20 times per second). Store the last reading for each.

**Priority override (runs before applying Pi command):**
```
if front_center < 25cm:
    execute ESCAPE sequence (back up 300ms, spin 90 degrees)
elif front_left < 30cm:
    reduce left motor speed by 40%, boost right
elif front_right < 30cm:
    reduce right motor speed by 40%, boost left
else:
    apply Pi command normally
```

The escape sequence is a fixed-time maneuver — it does not check sensors during execution. Simple and predictable.

---

## Gesture Control Response

The gesture detection runs entirely on the Raspberry Pi (see vision README). The ESP32 does not know what a gesture is. When the Pi detects a raised hand, it sends `CMD:S,0\n` (stop). When the hand is lowered, it resumes sending directional commands.

The ESP32 just follows orders. The watchdog timer is important here — if the Pi is in WAITING state and sends nothing, the ESP32 must not keep driving from the last command. Set watchdog to 500ms: if no command arrives within 500ms, stop motors.

---

## IMU — Tip-Over Detection

MPU-6050 over I2C. Read pitch angle every 100ms. If pitch exceeds ±30 degrees (robot tipping forward or backward), cut all motor power immediately and hold until pitch returns to safe range.

This protects the PCB and camera if the robot drives off a ledge or tips on a ramp.

---

## Firmware Structure (PlatformIO)

```
firmware/esp32/
├── platformio.ini          # board = esp32dev, framework = arduino
├── src/
│   ├── main.cpp            # setup() and loop()
│   ├── motor.cpp / .h      # PWM, PID, encoder ISR
│   ├── ultrasonic.cpp / .h # HC-SR04 polling and priority logic
│   ├── imu.cpp / .h        # MPU-6050 I2C read, tilt check
│   └── serial_cmd.cpp / .h # UART parse, watchdog timer
└── test/                   # unit tests if needed
```

---

## Pin Assignment (draft — confirm against KiCad schematic)

| Function | ESP32 GPIO |
|---|---|
| Left motor PWM | GPIO 25 |
| Left motor direction A | GPIO 26 |
| Left motor direction B | GPIO 27 |
| Right motor PWM | GPIO 32 |
| Right motor direction A | GPIO 33 |
| Right motor direction B | GPIO 14 |
| Left encoder | GPIO 34 |
| Right encoder | GPIO 35 |
| Ultrasonic front-center trigger | GPIO 5 |
| Ultrasonic front-center echo | GPIO 18 |
| Ultrasonic front-left trigger | GPIO 19 |
| Ultrasonic front-left echo | GPIO 21 |
| Ultrasonic front-right trigger | GPIO 22 |
| Ultrasonic front-right echo | GPIO 23 |
| IMU SDA | GPIO 21 |
| IMU SCL | GPIO 22 |
| UART RX (from Pi TX) | GPIO 16 |
| UART TX (to Pi RX) | GPIO 17 |

*Note: finalize pin assignments in KiCad before writing firmware. Changing pins after PCB is ordered means hardware rework.*

---

## Timeline

| Week | Target |
|---|---|
| 4-5 | PCB arrives — write basic motor spin, confirm over serial terminal |
| 5 | UART command parsing working, robot responds to F/B/L/R/S |
| 6 | Encoders reading, PID loop running, robot drives straight |
| 6-7 | Ultrasonic obstacle avoidance working |
| 7 | IMU tilt detection, watchdog, full integration with Pi |
| 8 | Tuning PID, edge cases handled |
