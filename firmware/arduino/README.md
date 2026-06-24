# Arduino Uno — bench-test firmware

Sketches used to develop and validate Philo on a breadboard before the real
ESP32 PCB exists. The Uno stands in for the ESP32.

## `arduino_philo.ino` — the ESP32 stand-in (receiver)

Reads the same serial protocol the Pi/laptop sends and reacts:

- **`CMD <left> <right> <flags>`**
  - follow LED (D6) on when moving, off when stopped
  - turn LEDs: D7 lights when steering left, D8 when steering right
  - OLED shows `FOLLOW`/`STOPPED`, direction, and live L/R speeds
- **`ASL <LETTER>`** — flashes the big letter on the OLED for 2s

### Wiring
```
OLED  SDA->A4  SCL->A5  VCC->5V  GND->GND   (0x3C)
Follow LED : long leg->220Ω->D6, short->GND   (D13 built-in mirrors it)
LEFT  LED  : long leg->220Ω->D7, short->GND
RIGHT LED  : long leg->220Ω->D8, short->GND
```

Upload at 115200, close the Arduino Serial Monitor, then run the vision stack
pointed at the Uno's port: `python main.py --webcam --serial COM8`

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
