// Philo bench sensor hub — Arduino Uno
// Reads MPU-6050 (tilt) + 2x HC-SR04 (distance), shows a live dashboard on the
// OLED and a full report on Serial (115200). LED lights when an obstacle is
// within 20cm — same idea as Philo's obstacle-avoidance priority.
//
// Wiring:
//   I2C: MPU + OLED   SDA->A4  SCL->A5   (MPU AD0->GND = addr 0x68)
//   HC-SR04 #1 (center): TRIG D2 ECHO D3
//   HC-SR04 #2 (left):   TRIG D4 ECHO D5
//   LED: D13 (built-in) + external LED on D6 (long leg->220ohm->D6, short leg->GND)
//   All VCC->5V, GND->GND

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define TRIG1 2
#define ECHO1 3
#define TRIG2 4
#define ECHO2 5
#define LED_PIN 13        // built-in LED
#define LED_EXT 6         // external LED on the breadboard (220ohm in series)
#define NEAR_CM 20        // "obstacle close" threshold

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_MPU6050 mpu;
bool mpu_ok = false;

// HC-SR04: distance in cm, or -1 if no echo / out of range
long readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 30000UL);   // 30ms timeout ~= 5m
  if (dur == 0) return -1;
  return dur / 58;                            // us -> cm
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_EXT, OUTPUT);

  Wire.begin();

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println("OLED not found at 0x3C");
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(10, 28);
  oled.print("Philo sensors...");
  oled.display();

  mpu_ok = mpu.begin();
  Serial.println(mpu_ok ? "MPU-6050 OK" : "MPU-6050 NOT found");

  Serial.println("Sensor hub ready.\n");
}

void loop() {
  long d1 = readDistance(TRIG1, ECHO1);
  long d2 = readDistance(TRIG2, ECHO2);

  float pitch = 0, roll = 0;
  if (mpu_ok) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    pitch = atan2(a.acceleration.y, a.acceleration.z) * 57.2958;
    roll  = atan2(-a.acceleration.x, a.acceleration.z) * 57.2958;
  }

  bool near = (d1 > 0 && d1 < NEAR_CM) || (d2 > 0 && d2 < NEAR_CM);
  digitalWrite(LED_PIN, near ? HIGH : LOW);
  digitalWrite(LED_EXT, near ? HIGH : LOW);

  // ---- Serial report ----
  Serial.print("dist1="); Serial.print(d1 < 0 ? -1 : d1); Serial.print("cm  ");
  Serial.print("dist2="); Serial.print(d2 < 0 ? -1 : d2); Serial.print("cm  ");
  Serial.print("pitch="); Serial.print(pitch, 0); Serial.print("  ");
  Serial.print("roll="); Serial.print(roll, 0); Serial.print("  ");
  Serial.println(near ? "[OBSTACLE]" : "");

  // ---- OLED dashboard ----
  oled.clearDisplay();
  oled.setCursor(0, 0);  oled.print("PHILO SENSOR TEST");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 16);
  oled.print("Dist1: "); oled.print(d1 < 0 ? -1 : d1); oled.print(" cm");
  oled.setCursor(0, 28);
  oled.print("Dist2: "); oled.print(d2 < 0 ? -1 : d2); oled.print(" cm");
  oled.setCursor(0, 40);
  oled.print("Tilt : "); oled.print(pitch, 0); oled.print((char)247);
  oled.setCursor(0, 52);
  oled.print(near ? "** OBSTACLE <20cm **" : "path clear");
  oled.display();

  delay(150);
}
