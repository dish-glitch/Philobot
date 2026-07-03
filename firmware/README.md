# Firmware — Philo ESP32

**Owner: [@dish-glitch](https://github.com/dish-glitch)**

---

## Overview

The ESP32 firmware handles everything physical — spinning motors, reading encoders, polling ultrasonic sensors, and listening for commands from the Raspberry Pi. It does not know about people or cameras. It just moves wheels and avoids walls.

Built with PlatformIO. Framework: Arduino (simpler than ESP-IDF for this use case, sufficient for our timing requirements).

---

## Responsibilities

| Task | How | Status |
|---|---|---|
| Receive direction + speed commands | UART from Raspberry Pi at 115200 baud | ✅ implemented |
| Drive motors | PWM via ESP32 LEDC peripheral, 20kHz frequency | ✅ implemented |
| Send telemetry to Pi | `STATUS` line at 10 Hz (vbat, distances, encoders, IMU) | ✅ implemented |
| Control speed closed-loop | PID per motor side using encoder pulse counting | ⬜ next — encoders count, PID not wired in yet |
| Read ultrasonic sensors | Timed pulse on trigger, measure echo pulse width, sequential only | ⚠️ reads work; 60ms anti-crosstalk gap not added yet |
| Override Pi commands on obstacle | Priority layer — safety above navigation | ⬜ next — validated on Arduino stand-in, not yet ported |
| Detect tip-over | Read MPU-6050 pitch angle over I2C | ⬜ next — IMU is read and reported, cutoff not wired in |
| Watchdog | Stop motors if no command received in 500ms | ⬜ **next — NOT yet implemented; do not drive the robot unattended until this lands** |
| ASL letter display | `ASL <letter>` line -> big letter on OLED | ⬜ next — works on Arduino stand-in, ESP32 parser ignores `ASL` lines |

> **Honest status note:** the modules all compile and the command/telemetry loop is real,
> but the safety layer (watchdog, obstacle override, tilt cutoff, boot-time UART flush,
> I2C bus recovery) is still bench-rig-only or unimplemented. These are the first items
> to land once the PCB arrives — they are what makes the robot safe to run untethered.

---

## Serial Protocol (UART — Raspberry Pi to ESP32)

**Baud rate:** 115200
**Format:** `CMD <left> <right> <flags>\n` — **numeric differential drive** (as implemented in [`src/pi_comm.cpp`](src/pi_comm.cpp))

| Field | Range | Meaning |
|---|---|---|
| `left` | -255..255 | left-side wheel speed (negative = reverse, 0 = coast) |
| `right` | -255..255 | right-side wheel speed |
| `flags` | bitfield | bit 0 = coast stop, bit 1 = hard brake, 0 = normal drive |

Independent left/right speeds let the Pi steer **proportionally** (curve smoothly toward the person) instead of issuing discrete F/B/L/R/S turns. `motor_set(left, right)` in [`src/motors.h`](src/motors.h) drives the two sides directly.

**Examples:**
```
CMD 180 180 0\n   -> straight ahead at ~70% speed
CMD 180 110 0\n   -> curve right (right wheels slower)
CMD 0 0 1\n       -> stop (coast)
CMD 0 0 2\n       -> stop (hard brake)
```

The ESP32 parses each line as it arrives. If a line is malformed or (once implemented) the watchdog timer expires (no command for 500ms), motors stop immediately.

> **Historical note:** earlier drafts of this doc used a `CMD:<direction>,<speed>` format (`CMD:F,180`). That was replaced by the numeric differential-drive format above, which is what the firmware and the Pi-side code actually implement.

---

## Motor Control

**4 motors, 4 channels, 2 chips, controlled as 2 sides:**
- Each motor has its OWN H-bridge channel (left-front, left-rear, right-front, right-rear = 4 channels across 2 TB6612FNG chips). Do NOT parallel two motors onto one channel — two JGA25-370s stall at 3-4.4A combined, over the TB6612FNG 3.2A peak rating.
- The two left motors share the same control signals (PWM_L/IN1/IN2 on GPIO 25/26/27), so they always move identically. The two right motors share the same control signals (GPIO 32/33/14). Electrically separate channels, logically one "side." See [hardware SCHEMATIC.md](../hardware/SCHEMATIC.md) Sheet 3 for the exact fan-out.

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

The gesture detection runs entirely on the Raspberry Pi (see vision README). The ESP32 does not know what a gesture is. When the Pi detects a raised hand, it sends `CMD 0 0 1\n` (coast stop). When the hand is lowered, it resumes sending normal drive commands.

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

Without this, the robot may execute stale commands (e.g., `CMD 255 255 0` from before the crash) immediately on recovery.

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

## Companion Display — OLED Eyes  — ⚠️ PLANNED

> **Status:** Not implemented yet. The current [`display.cpp`](src/display.cpp) drives **one** SSD1306 as a **telemetry readout** (`display_status()` shows battery, ultrasonic distances, encoder counts). The animated dual-eye / mood system below — including the `MOOD:` UART extension — is the design target. Build it on top of the current display module; until then the Pi should not depend on `MOOD:` packets being understood.

Philo is a companion robot, so the goal is to give it a face: two SSD1306 OLED displays (128x64, I2C) acting as animated eyes. The ESP32 drives them because it is already the I2C master and it knows the robot's real-time state (moving, stopped, obstacle, tilt, low battery).

**Hardware:** Two OLEDs on the existing I2C bus, at addresses 0x3C and 0x3D (set by the address-select jumper/resistor on each module). The MPU-6050 is already on this bus at 0x68, so the bus now carries three devices. The existing 4.7k pull-ups are adequate for three devices on a short bus.

### CRITICAL: Run the display on core 1, not in the control loop

A full 128x64 OLED refresh over I2C at 400kHz takes ~23ms. Two displays is ~46ms. If you render eyes inside the main control loop, you destroy PID timing and the robot drives badly.

The ESP32 is dual-core. Pin the display rendering to **core 1** as a separate FreeRTOS task. The control loop (motors, PID, sensors, UART, watchdog) stays on **core 0** where it runs uninterrupted.

```cpp
// Shared state — written by core 0, read by core 1
volatile RobotMood current_mood = MOOD_NEUTRAL;

void display_task(void *param) {
    init_oled(0x3C);  // left eye
    init_oled(0x3D);  // right eye
    for (;;) {
        render_eyes(current_mood);   // draw both eyes for current mood
        vTaskDelay(pdMS_TO_TICKS(100));  // ~10 FPS, plenty for eye animation
    }
}

void setup() {
    // ... control setup on core 0 ...
    xTaskCreatePinnedToCore(display_task, "eyes", 4096, NULL, 1, NULL, 1);  // core 1
}
```

Eye rendering reads `current_mood` but never writes shared control state. Core 0 sets the mood flag; core 1 just draws. No mutex needed for a single volatile enum.

### Mood states

The ESP32 derives most expressions from its own state. The Pi sends optional hints for vision-driven moods over UART.

| Mood | Trigger (who sets it) | Eye expression |
|---|---|---|
| NEUTRAL | Following normally (core 0) | Open, looking forward |
| HAPPY | Pi sends `MOOD:FOUND` when target acquired | Curved/smiling eyes |
| SEARCHING | Pi sends `MOOD:LOST` when no person detected | Eyes glance side to side |
| WAITING | Pi sends `MOOD:WAIT` on raised-hand gesture | Half-closed, calm |
| ALERT | Obstacle detected by ultrasonic (core 0) | Wide eyes |
| DIZZY | IMU tilt > 30 degrees (core 0) | Spiral / X eyes |
| SLEEPY | Low battery < 6.4V (core 0) | Drooping eyes |

### MOOD protocol extension

When implemented, the Pi will send mood hints over the existing UART link using a second command prefix, parsed alongside `CMD`:

```
MOOD:FOUND\n     -> set current_mood = HAPPY
MOOD:LOST\n      -> set current_mood = SEARCHING
MOOD:WAIT\n      -> set current_mood = WAITING
```

`MOOD:` packets only set the display flag — they never move motors. State set by core 0 (ALERT, DIZZY, SLEEPY) takes priority over Pi mood hints, because safety-relevant state should always show on the face.

---

## Firmware Structure (PlatformIO)

```
firmware/esp32/
+-- platformio.ini          # board = esp32dev, framework = arduino
+-- src/
|   +-- main.cpp            # setup() and loop()
|   +-- motors.cpp / .h     # PWM, PID, encoder ISR
|   +-- ultrasonic.cpp / .h # HC-SR04 sequential polling and priority logic
|   +-- imu.cpp / .h        # MPU-6050 I2C read, tilt check
|   +-- pi_comm.cpp / .h    # UART parse (CMD numeric), STATUS send; MOOD parse planned
|   +-- display.cpp / .h    # OLED eyes, core-1 FreeRTOS task, mood rendering
+-- test/                   # unit tests if needed
```

---

## Pin Assignment (final — verified from KiCad netlist)

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

*Pin assignments are final — verified from KiCad netlist. Do not change without updating both the schematic and this table.*

*Previous revision had a conflict: GPIO 21 was assigned to both ultrasonic front-left echo AND I2C SDA, and GPIO 22 was assigned to both front-right trigger AND I2C SCL. This table resolves both conflicts. Ultrasonic echo uses GPIO 4/18/23; ultrasonic triggers use GPIO 5/13/19; I2C uses GPIO 21/22 as intended.*

---

## Timeline

| Status | Milestone |
|---|---|
| **DONE** | All firmware modules compile — motors, encoders, ultrasonic, IMU, OLED, Pi UART |
| **DONE** | UART command parsing — robot responds to `CMD <left> <right> <flags>` |
| **DONE** | Gesture stop tested on Arduino stand-in |
| **DONE** | ASL letter display over UART tested on Arduino stand-in |
| Next | PCB arrives — verify all modules on real ESP32 hardware |
| Next | Encoders reading, PID loop running, robot drives straight |
| Next | Ultrasonic obstacle avoidance working (sequential firing confirmed) |
| Next | IMU tilt detection, full integration with Pi |
| Next | PID tuning, edge cases, demo readiness |
