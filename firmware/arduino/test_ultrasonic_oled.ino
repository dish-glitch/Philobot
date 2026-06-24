// TEST 4 — HC-SR04 ultrasonic, shown on the OLED (no Serial needed).
// HC-SR04:  TRIG D2  ECHO D3  VCC->5V  GND->GND
// OLED on I2C as usual (SDA A4, SCL A5).
// Expect: a distance in cm that drops as you move your hand toward the sensor.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define TRIG 2
#define ECHO 3

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oled_ok = false;

long readDistance() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long dur = pulseIn(ECHO, HIGH, 30000UL);   // 30ms timeout ~= 5m
  if (dur == 0) return -1;                    // no echo
  return dur / 58;                            // us -> cm
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  Wire.begin();
  oled_ok = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  long d = readDistance();

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("ULTRASONIC TEST");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setTextSize(3);
  oled.setCursor(0, 22);
  if (d < 0) oled.print("--");
  else       oled.print(d);
  oled.setTextSize(2);
  oled.print(" cm");
  oled.setTextSize(1);
  oled.display();

  delay(100);
}
