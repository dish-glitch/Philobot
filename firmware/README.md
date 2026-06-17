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
| Read ultrasonic sensors | Timed pulse on trigger, measure echo pulse width, sequential only |
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

**Speed:** 0-255 (maps to PWM duty cycle, 0 = stopped, 255 = full speed)

**Examples:**
```
CMD:F,180\n    -> drive forward at ~70% speed
CMD:L,120\n    -> turn left at ~47% speed
CMD:S,0\n      -> stop
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

**Soft-start — required to protect TB6612FNG:**

On every new movement command (transitioning from stop to motion), ramp PWM from 0 to the target duty cycle over 200ms. Do not jump immediately to full duty cycle.

```cpp
void soft_start(uint8_t target_pwm, uint8_t motor_side) {
    uint8_t current = 0;
    while (current < target_pwm) {
        current = min(current + 5, target_pwm);
        set_motor_pwm(motor_side, current);
        delay(10);  // 200ms total for 0->255 ramp, shorter for lower targets
    }
}
```

Why: JGA25-370 motors starting from rest on carpet are near stall for 100-400ms. Without soft-start, TB6612FNG sees repeated stall-current bursts on every start. With soft-start, the current rises gradually as the motor accelerates, never hitting peak stall.

---

## Ultrasonic Sensor Logic

Three HC-SR04 sensors: front-left, front-center, front-right.

### CRITICAL: Fire sensors sequentially, never simultaneously

**DO NOT fire all three HC-SR04 sensors at the same time.** If multiple sensors fire simultaneously, each sensor may pick up the ultrasonic pulse from another sensor rather than its own reflection. This causes false short-range readings (phantom walls) that trigger the obstacle escape sequence repeatedly.

**Confirmed failure mode:** Sensor A fires a 40kHz pulse, Sensor B fires at the same time, Sensor B's echo pin goes high from Sensor A's pulse (wrong reflection), ESP32 reads 0.03m where there is no object.

**Fix: 60ms gap between each sensor fire.** Total scan cycle is 180ms for all three sensors.

```
Timing:
  t=0ms:    Fire front-center trigger
  t=60ms:   Read front-center echo, fire front-left trigger
  t=120ms:  Read front-left echo, fire front-right trigger
  t=180ms:  Read front-right echo — cycle complete, restart
```

**Reading one sensor:**
```
1. Pull trigger pin HIGH for 10 microseconds
2. Pull trigger pin LOW
3. Wait for echo pin to go HIGH (timeout 30ms if nothing detected)
4. Measure how long echo pin stays HIGH
5. distance_cm = echo_duration_us / 58.0
```

**Sensor invalid reading handling:**

HC-SR04 can return no echo (object too close, noise, wiring fault). Do not interpret timeout as 0cm (triggers escape loop) or as max range (treats blocked path as clear). Instead:

```cpp
#define SENSOR_MIN_CM 2
#define SENSOR_MAX_CM 400
#define SENSOR_INVALID_HOLDOFF_CYCLES 3

uint16_t last_valid[3] = {400, 400, 400};  // default to far/clear
uint8_t  invalid_count[3] = {0, 0, 0};

void update_sensor(int idx, uint16_t raw_cm) {
    if (raw_cm < SENSOR_MIN_CM || raw_cm > SENSOR_MAX_CM) {
        invalid_count[idx]++;
        if (invalid_count[idx] >= SENSOR_INVALID_HOLDOFF_CYCLES) {
            last_valid[idx] = 20;  // treat as obstacle after 3 bad readings
        }
        // else: hold last known valid reading
    } else {
        last_valid[idx] = raw_cm;
        invalid_count[idx] = 0;
    }
}
```

Use `last_valid[idx]` in the priority logic below, never raw readings.

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

**Ultrasonic side blind spots:** The HC-SR04 has a ~15 degree cone angle. Objects that approach from a 45-75 degree angle (between the sensor cones) may not be detected until very close. The angled sensor placement mitigates this but does not eliminate it. Do not position the robot such that furniture is approaching from the sides — the sensors only protect the front.

---

## Gesture Control Response

The gesture detection runs entirely on the Raspberry Pi (see vision README). The ESP32 does not know what a gesture is. When the Pi detects a raised hand, it sends `CMD:S,0\n` (stop). When the hand is lowered, it resumes sending directional commands.

The ESP32 just follows orders. The watchdog timer is important here — if the Pi is in WAITING state and sends nothing, the ESP32 must not keep driving from the last command. Set watchdog to 500ms: if no command arrives within 500ms, stop motors.

---

## IMU — Tip-Over Detection

MPU-6050 over I2C. Read pitch angle every 100ms. If pitch exceeds +/-30 degrees (robot tipping forward or backward), cut all motor power immediately and hold until pitch returns to safe range.

This protects the PCB and camera if the robot drives off a ledge or tips on a ramp.

**UART buffer flush on boot:**

When ESP32 resets (power cycle or watchdog), the Pi's serial write buffer may contain queued commands from before the reset. On boot, flush the UART receive buffer before entering the main loop — discard everything in the buffer accumulated while ESP32 was offline:

```cpp
void setup() {
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    delay(300);  // wait for Pi to potentially send stale data
    while (Serial2.available()) Serial2.read();  // flush
    i2c_bus_recovery();
    // ... rest of setup
}
```

Without this, the robot may execute stale commands (e.g., CMD:F,255 from before the crash) immediately on recovery.

**I2C bus recovery:** Run a 9-clock SCL recovery routine at firmware init before starting the main loop. This clears any stuck-SDA-low condition left over from a hard reset or Pi reboot mid-transaction:

```cpp
// I2C bus recovery — 9 SCL clocks with SDA high
void i2c_bus_recovery() {
    for (int i = 0; i < 9; i++) {
        digitalWrite(SCL_PIN, HIGH);
        delayMicroseconds(5);
        digitalWrite(SCL_PIN, LOW);
        delayMicroseconds(5);
    }
}
```

---

## Firmware Structure (PlatformIO)

```
firmware/esp32/
+-- platformio.ini          # board = esp32dev, framework = arduino
+-- src/
|   +-- main.cpp            # setup() and loop()
|   +-- motor.cpp / .h      # PWM, PID, encoder ISR
|   +-- ultrasonic.cpp / .h # HC-SR04 sequential polling and priority logic
|   +-- imu.cpp / .h        # MPU-6050 I2C read, tilt check
|   +-- serial_cmd.cpp / .h # UART parse, watchdog timer
+-- test/                   # unit tests if needed
```

---

## Pin Assignment (draft — confirm against KiCad schematic)

| Function | ESP32 GPIO | Notes |
|---|---|---|
| Left motor PWM | GPIO 25 | LEDC channel |
| Left motor direction A | GPIO 26 | TB6612FNG AIN1 |
| Left motor direction B | GPIO 27 | TB6612FNG AIN2 |
| Right motor PWM | GPIO 32 | LEDC channel |
| Right motor direction A | GPIO 33 | TB6612FNG BIN1 |
| Right motor direction B | GPIO 14 | TB6612FNG BIN2 |
| Left encoder | GPIO 34 | Input-only pin |
| Right encoder | GPIO 35 | Input-only pin |
| Ultrasonic front-center trigger | GPIO 5 | |
| Ultrasonic front-center echo | GPIO 18 | After 1k/2k voltage divider |
| Ultrasonic front-left trigger | GPIO 19 | |
| Ultrasonic front-left echo | GPIO 4 | After 1k/2k voltage divider |
| Ultrasonic front-right trigger | GPIO 13 | |
| Ultrasonic front-right echo | GPIO 23 | After 1k/2k voltage divider |
| IMU SDA | GPIO 21 | I2C with 4.7k pull-up to 3.3V |
| IMU SCL | GPIO 22 | I2C with 4.7k pull-up to 3.3V |
| UART RX (from Pi TX) | GPIO 16 | |
| UART TX (to Pi RX) | GPIO 17 | |
| Battery ADC | GPIO 36 | Input-only, 10k/3.3k divider |
| Status LED | GPIO 2 | 100 ohm series resistor |

*Note: finalize pin assignments in KiCad before writing firmware. Changing pins after PCB is ordered means hardware rework.*

*Previous revision had a conflict: GPIO 21 was assigned to both ultrasonic front-left echo AND I2C SDA, and GPIO 22 was assigned to both front-right trigger AND I2C SCL. This table resolves both conflicts. Ultrasonic echo uses GPIO 4/18/23; ultrasonic triggers use GPIO 5/13/19; I2C uses GPIO 21/22 as intended.*

---

## Timeline

| Week | Target |
|---|---|
| 4-5 | PCB arrives — write basic motor spin, confirm over serial terminal |
| 5 | UART command parsing working, robot responds to F/B/L/R/S |
| 6 | Encoders reading, PID loop running, robot drives straight |
| 6-7 | Ultrasonic obstacle avoidance working (sequential firing confirmed) |
| 7 | IMU tilt detection, watchdog, full integration with Pi |
| 8 | Tuning PID, edge cases handled |
