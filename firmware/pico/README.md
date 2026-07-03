# Raspberry Pi Pico — bench-test firmware (MicroPython)

Experimental stand-in for the ESP32. Reads the Pi's serial protocol and drives an
LED (and OLED, in `pico_main.py`).

- `pico_led_test.py` — minimal LED-only test for the `CMD` protocol.
- `pico_main.py` — LED + SSD1306 OLED, handles `CMD` and `ASL` messages.

## Known issue

Reading USB serial (`sys.stdin`) in standalone mode (running as `main.py` on boot,
MicroPico disconnected) is unreliable — see issue tracker. The **Arduino Uno is the
current bench stand-in**. The Pico is expected to work properly on the final robot
where the Pi 5 talks to it over the **hardware UART pins** (not USB).

## Setup

1. Flash MicroPython (`.uf2` from micropython.org) by holding BOOTSEL and copying
   the file onto the RPI-RP2 drive.
2. For the OLED: `mpremote mip install ssd1306`.

## Wiring

```
OLED VCC -> 3V3 (pin 36)    OLED SDA -> GP0 (pin 1)
OLED GND -> GND (pin 38)    OLED SCL -> GP1 (pin 2)

LED long leg -> 220Ω -> GP15 (pin 20)    LED short leg -> GND (pin 23)
```
