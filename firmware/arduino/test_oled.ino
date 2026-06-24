// TEST 2 — OLED only. Nothing else connected matters.
// I2C: SDA->A4  SCL->A5  VCC->5V  GND->GND   (addr 0x3C)
// Expect: clean text + a counter ticking up, and a moving box.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oled_ok = false;
int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nOLED test start");

  Wire.begin();
  oled_ok = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println(oled_ok ? "OLED begin OK" : "OLED begin FAILED");
}

void loop() {
  if (!oled_ok) { Serial.println("no OLED"); delay(1000); return; }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("PHILO OLED TEST");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 18);
  oled.print("Count: ");
  oled.print(counter);

  // a box that slides across so you can see refresh working
  int x = (counter * 8) % 120;
  oled.drawRect(x, 44, 8, 8, SSD1306_WHITE);

  oled.display();

  Serial.print("frame "); Serial.println(counter);
  counter++;
  delay(300);
}
