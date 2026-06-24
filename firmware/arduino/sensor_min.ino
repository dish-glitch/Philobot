// Philo combined bench test — Arduino Uno (clean, OLED output only)
// OLED + MPU-6050 (tilt) + HC-SR04 (distance) + LED on obstacle.
// This is just the proven single-part tests merged — no Serial needed.
//
// Wiring:
//   I2C: MPU + OLED   SDA->A4  SCL->A5   (MPU AD0->GND = addr 0x68)
//   HC-SR04:  TRIG D2  ECHO D3
//   LED: D6 (long leg->220ohm->D6, short leg->GND)
//   All VCC->5V, GND->GND

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define TRIG 2
#define ECHO 3
#define LED_EXT 6
#define NEAR_CM 20

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_MPU6050 mpu;
bool mpu_ok = false;

long readDistance() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long dur = pulseIn(ECHO, HIGH, 30000UL);
  if (dur == 0) return -1;
  return dur / 58;
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED_EXT, OUTPUT);

  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  mpu_ok = mpu.begin();
}

void loop() {
  long d = readDistance();

  float pitch = 0;
  if (mpu_ok) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    pitch = atan2(a.acceleration.y, a.acceleration.z) * 57.2958;
  }

  bool near = (d > 0 && d < NEAR_CM);
  digitalWrite(LED_EXT, near ? HIGH : LOW);

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("PHILO SENSOR TEST");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.print(d < 0 ? -1 : d); oled.print("cm");

  oled.setCursor(0, 40);
  oled.print(pitch, 0); oled.print((char)247);

  oled.setTextSize(1);
  oled.setCursor(70, 47);
  oled.print(near ? "OBSTACLE" : "clear");

  oled.display();
  delay(150);
}
