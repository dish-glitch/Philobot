// TEST 3 — MPU-6050, shown on the OLED (no Serial needed).
// I2C: MPU + OLED  SDA->A4  SCL->A5  VCC->5V  GND->GND  (MPU AD0->GND)
// Expect: PITCH / ROLL numbers that change as you tilt the breadboard.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_MPU6050 mpu;
bool oled_ok = false, mpu_ok = false;

void setup() {
  Wire.begin();
  oled_ok = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  mpu_ok  = mpu.begin();

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(mpu_ok ? "MPU found" : "MPU MISSING");
  oled.display();
  delay(800);
}

void loop() {
  float pitch = 0, roll = 0;
  if (mpu_ok) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    pitch = atan2(a.acceleration.y, a.acceleration.z) * 57.2958;
    roll  = atan2(-a.acceleration.x, a.acceleration.z) * 57.2958;
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(mpu_ok ? "MPU-6050 TILT TEST" : "MPU MISSING!");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 18);
  oled.print("P:"); oled.print(pitch, 0); oled.print((char)247);
  oled.setCursor(0, 42);
  oled.print("R:"); oled.print(roll, 0); oled.print((char)247);
  oled.setTextSize(1);
  oled.display();

  delay(120);
}
