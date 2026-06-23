import sys
import select
import machine
import ssd1306

# LED — goes HIGH when following, LOW when gesture-stopped
led = machine.Pin(15, machine.Pin.OUT)

# OLED
i2c  = machine.I2C(0, sda=machine.Pin(0), scl=machine.Pin(1), freq=400000)
oled = ssd1306.SSD1306_I2C(128, 64, i2c)
oled.fill(0)
oled.text("Philo Ready", 10, 28)
oled.show()

# Non-blocking serial read
poll = select.poll()
poll.register(sys.stdin, select.POLLIN)

while True:
    if poll.poll(10):
        line = sys.stdin.readline().strip()

        if line.startswith("CMD"):
            parts = line.split()
            if len(parts) >= 3:
                left  = int(parts[1])
                right = int(parts[2])
                led.value(0 if (left == 0 and right == 0) else 1)

        elif line.startswith("ASL"):
            letter = line[4:].strip()
            oled.fill(0)
            oled.text("Sign detected:", 0, 0)
            # Big letter in the center
            oled.text(letter, 56, 28)
            oled.show()
