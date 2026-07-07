# Arduino Uno — bench-test firmware

Sketches used to develop and validate Philo on a breadboard before the real
ESP32 PCB exists. The Uno stands in for the ESP32.

## `arduino_philo.ino` — the ESP32 stand-in (receiver + safety layer)

Reads the laptop's serial protocol, drives the dashboard, and runs an obstacle
safety override on its own ultrasonic sensor, serving as the ESP32 while the PCB is unavailable.

- **`CMD <left> <right> <flags>`**
  - follow LED (D6) on when moving, off when stopped
  - turn LEDs: D7 lights when steering left, D8 when steering right
  - OLED shows `FOLLOW`/`STOPPED`/`OBSTACLE`, direction, and live distance
- **`ASL <LETTER>`** — flashes the big letter on the OLED for 2s
- **Obstacle override** — reads its own HC-SR04; if something is closer than
  `OBSTACLE_CM` (20cm) it forces STOP regardless of the camera command. Safety
  does not depend on the laptop or the link staying alive.
- **Telemetry** — sends `STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>`
  back at 10Hz (single ultrasonic reported as `dc`). The laptop shows the
  distance + an override warning in the video window, so data flows both ways.

### Wiring
```
OLED  SDA->A4  SCL->A5  VCC->5V  GND->GND   (0x3C)
HC-SR04 :  TRIG->D2  ECHO->D3
Follow LED : long leg->220Ω->D6, short->GND
LEFT  LED  : long leg->220Ω->D7, short->GND
RIGHT LED  : long leg->220Ω->D8, short->GND
```

Upload at 115200, close the Arduino Serial Monitor, then run the vision stack
pointed at the Uno's port: `python main.py --webcam --serial COM8`

> The MPU-6050 tip-over override is not in this sketch yet — reading the MPU over
> I2C with no timeout can hang the loop if the connection is flaky. Add it back
> once the bus is rock-solid.

## Bench test sketches (sensor bring-up, run individually)

| Sketch | Tests |
|---|---|
| `i2c_scan.ino` | Lists I2C devices — confirms OLED (0x3C) + MPU-6050 (0x68) |
| `test_oled.ino` | OLED only — clean text + counter |
| `test_mpu_oled.ino` | MPU-6050 tilt (pitch/roll) shown on OLED |
| `test_ultrasonic_oled.ino` | One HC-SR04 distance shown on OLED |
| `test_led.ino` | LED blink on D6 |
| `sensor_min.ino` | OLED + MPU + 1 ultrasonic + LED combined |
| `sensor_hub.ino` | OLED + MPU + 2 ultrasonics + LED combined |

## Libraries (Arduino IDE → Library Manager)

- Adafruit SSD1306, Adafruit GFX
- Adafruit MPU6050, Adafruit Unified Sensor (for the MPU sketches)

> Note: HC-SR04 echo connects straight to the 5V Uno here. On the real 3.3V
> ESP32 board the echo line goes through a voltage divider (see hardware BOM).
