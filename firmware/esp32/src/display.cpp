#include "display.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define OLED_ADDR  0x3C
#define OLED_W     128
#define OLED_H      64

static Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
static bool ready = false;

bool display_init() {
    ready = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    if (ready) {
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.println("Philo booting up...");
        oled.display();
    }
    return ready;
}

void display_status(float vbat, float dl, float dc, float dr,
                    int32_t el, int32_t er) {
    if (!ready) return;
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.printf("Bat: %.2f V\n", vbat);
    oled.printf("L:%4.0f C:%4.0f R:%4.0f\n", dl, dc, dr);
    oled.printf("EL:%6ld ER:%6ld\n", (long)el, (long)er);
    oled.display();
}

void display_clear() {
    if (!ready) return;
    oled.clearDisplay();
    oled.display();
}
