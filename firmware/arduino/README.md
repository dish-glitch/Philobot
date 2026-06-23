# Arduino Uno — bench-test firmware

`arduino_philo.ino` is a stand-in for the ESP32 used while developing the vision
stack on a laptop. It reads the same serial protocol the Pi sends and:

- **`CMD <left> <right> <flags>`** — lights the onboard LED (pin 13) when
  following, turns it off on gesture stop.
- **`ASL <LETTER>`** — shows the letter on a 128x64 SSD1306 OLED.

## Wiring

```
OLED VCC -> 5V      OLED SDA -> A4
OLED GND -> GND     OLED SCL -> A5

LED long leg -> 220Ω -> pin 13     LED short leg -> GND
```

## Libraries (Arduino IDE → Library Manager)

- Adafruit SSD1306
- Adafruit GFX (installed automatically with SSD1306)

Upload at 115200 baud, then point the vision stack at the Uno's COM port:
`python main.py --webcam --serial COM8`
