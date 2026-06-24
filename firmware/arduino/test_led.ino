// TEST 5 — LED only. Confirms the breadboard LED is wired right.
// LED: long leg (+) -> 220ohm -> D6,  short leg (-) -> GND
// Expect: the LED blinks on/off once per second.

#define LED_EXT 6

void setup() {
  pinMode(LED_EXT, OUTPUT);
}

void loop() {
  digitalWrite(LED_EXT, HIGH);
  delay(500);
  digitalWrite(LED_EXT, LOW);
  delay(500);
}
