// Philo bench receiver — Arduino Uno as ESP32 stand-in.
// Listens to the laptop vision stack over USB serial and reacts:
//   CMD <left> <right> <flags>  -> follow LED on when moving, off when stopped
//                                  turn LEDs show left/right steering
//                                  OLED shows state + direction
//   ASL <LETTER>                -> OLED flashes the big letter for 2s
//
// Wiring:
//   OLED  SDA->A4 SCL->A5  VCC->5V GND->GND  (0x3C)
//   Follow LED : long leg->220ohm->D6,  short->GND   (D13 built-in mirrors it)
//   LEFT  LED  : long leg->220ohm->D7,  short->GND   (lights when turning left)
//   RIGHT LED  : long leg->220ohm->D8,  short->GND   (lights when turning right)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

#define LED_FOLLOW 6
#define LED_LEFT   7
#define LED_RIGHT  8
#define LED_BUILTIN_PIN 13
#define ASL_HOLD_MS 2000   // keep an ASL letter on screen this long
#define TURN_MARGIN 15     // L/R speed gap before we call it a turn

unsigned long asl_until = 0;

void showDrive(int left, int right, bool stopped, const char* dir) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("PHILO");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 18);
  oled.print(stopped ? "STOPPED" : "FOLLOW");
  if (!stopped) {
    oled.setTextSize(1);
    oled.setCursor(0, 38);
    oled.print("Dir: "); oled.print(dir);
  }
  oled.setTextSize(1);
  oled.setCursor(0, 52);
  oled.print("L:"); oled.print(left);
  oled.print("  R:"); oled.print(right);
  oled.display();
}

void showASL(String letter) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Sign detected:");
  oled.setTextSize(5);
  oled.setCursor(46, 22);
  oled.print(letter);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_FOLLOW, OUTPUT);
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT);

  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(10, 24);
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
      bool stopped = (left == 0 && right == 0);

      // follow LED
      digitalWrite(LED_FOLLOW, stopped ? LOW : HIGH);
      digitalWrite(LED_BUILTIN_PIN, stopped ? LOW : HIGH);

      // turn indicators:  left>right = turning right,  right>left = turning left
      int diff = left - right;
      bool turnRight = (!stopped && diff >  TURN_MARGIN);
      bool turnLeft  = (!stopped && diff < -TURN_MARGIN);
      digitalWrite(LED_RIGHT, turnRight ? HIGH : LOW);
      digitalWrite(LED_LEFT,  turnLeft  ? HIGH : LOW);

      const char* dir = stopped ? "-" : (turnLeft ? "LEFT" : (turnRight ? "RIGHT" : "STRAIGHT"));
      if (millis() > asl_until) showDrive(left, right, stopped, dir);
    }
    else if (line.startsWith("ASL")) {
      String letter = line.substring(4);
      letter.trim();
      showASL(letter);
      asl_until = millis() + ASL_HOLD_MS;
    }
  }
}
