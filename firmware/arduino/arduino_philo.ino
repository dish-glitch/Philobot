#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LED = 13;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // OLED not found — continue without it
    while (false);
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(20, 28);
  oled.print("Philo Ready");
  oled.display();
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("CMD")) {
      int left = 0, right = 0, flags = 0;
      sscanf(line.c_str(), "CMD %d %d %d", &left, &right, &flags);
      digitalWrite(LED, (left == 0 && right == 0) ? LOW : HIGH);
    }
    else if (line.startsWith("ASL")) {
      String letter = line.substring(4);
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.print("Sign detected:");
      oled.setTextSize(4);
      oled.setCursor(44, 24);
      oled.print(letter);
      oled.display();
    }
  }
}
