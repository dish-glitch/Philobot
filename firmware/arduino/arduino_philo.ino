// Philo bench receiver + obstacle safety layer — Arduino Uno as ESP32 stand-in.
//
// Receives from laptop vision stack:
//   CMD <left> <right> <flags>   -> drive state (follow LED + turn LEDs + OLED)
//   ASL <LETTER>                 -> OLED flashes the big letter for 2s
//
// Reads its OWN ultrasonic and OVERRIDES the command for safety (like the ESP32):
//   - obstacle closer than OBSTACLE_CM  -> force STOP, even while camera says follow
//
// Reports back to laptop at 10Hz:
//   STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>
//
// Wiring:
//   OLED on I2C  SDA->A4 SCL->A5  (0x3C)
//   HC-SR04  TRIG->D2  ECHO->D3
//   Follow LED D6, LEFT LED D7, RIGHT LED D8   (each long leg->220ohm->pin)
//
// (MPU-6050 tip-over override removed for now — add back once the I2C bus is
//  rock-solid; it can hang the loop if the MPU connection is flaky.)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define TRIG 2
#define ECHO 3
#define LED_FOLLOW 6
#define LED_LEFT   7
#define LED_RIGHT  8

#define OBSTACLE_CM 20
#define TURN_MARGIN 15
#define ASL_HOLD_MS 2000
#define STATUS_MS   100     // 10 Hz

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

int  cmdLeft = 0, cmdRight = 0;
bool cmdStopped = true;
unsigned long asl_until = 0;
unsigned long last_status = 0;

long readDistance() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long dur = pulseIn(ECHO, HIGH, 25000UL);
  if (dur == 0) return -1;
  return dur / 58;
}

void drawState(const char* state, int dist, const char* dir) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("PHILO");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 16);  oled.print(state);
  oled.setTextSize(1);
  oled.setCursor(0, 40);  oled.print("Dir: "); oled.print(dir);
  oled.setCursor(0, 52);  oled.print("Dist: ");
  if (dist < 0) oled.print("--"); else oled.print(dist);
  oled.print("cm");
  oled.display();
}

void drawASL(String letter) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("Sign detected:");
  oled.setTextSize(5);
  oled.setCursor(46, 22); oled.print(letter);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(LED_FOLLOW, OUTPUT);
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);

  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(10, 24); oled.print("Philo Ready");
  oled.display();
}

void loop() {
  // 1. handle pending serial lines
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("CMD")) {
      int l = 0, r = 0, f = 0;
      sscanf(line.c_str(), "CMD %d %d %d", &l, &r, &f);
      cmdLeft = l; cmdRight = r;
      cmdStopped = (l == 0 && r == 0);
    } else if (line.startsWith("ASL")) {
      String letter = line.substring(4); letter.trim();
      drawASL(letter);
      asl_until = millis() + ASL_HOLD_MS;
    }
  }

  // 2. ultrasonic + safety override + LEDs + OLED + telemetry at 10Hz
  unsigned long now = millis();
  if (now - last_status >= STATUS_MS) {
    last_status = now;

    long dist = readDistance();
    bool obstacle = (dist > 0 && dist < OBSTACLE_CM);

    bool stopped = cmdStopped || obstacle;     // safety override
    int  diff = cmdLeft - cmdRight;
    bool turnRight = (!stopped && diff >  TURN_MARGIN);
    bool turnLeft  = (!stopped && diff < -TURN_MARGIN);

    digitalWrite(LED_FOLLOW, stopped ? LOW : HIGH);
    digitalWrite(LED_RIGHT, turnRight ? HIGH : LOW);
    digitalWrite(LED_LEFT,  turnLeft  ? HIGH : LOW);

    if (now > asl_until) {
      const char* state = obstacle ? "OBSTACLE" : (stopped ? "STOPPED" : "FOLLOW");
      const char* dir = stopped ? "-" : (turnLeft ? "LEFT" : (turnRight ? "RIGHT" : "STRAIGHT"));
      drawState(state, dist, dir);
    }

    // telemetry back to laptop (single ultrasonic reported as center; others 999=clear)
    Serial.print("STATUS 7.40 999 ");
    Serial.print(dist < 0 ? 999 : dist);
    Serial.print(" 999 0 0 0 0 0 0\n");
  }
}
