#include "ultrasonic.h"
#include "pins.h"

// 10 ms timeout → ~1.7 m max range (plenty for obstacle avoidance)
#define US_TIMEOUT_US  10000UL
#define SOUND_CM_US    0.01715f   // (34300 cm/s) / 2 / 1e6

void ultrasonic_init() {
    pinMode(PIN_TRIG_L, OUTPUT);
    pinMode(PIN_TRIG_C, OUTPUT);
    pinMode(PIN_TRIG_R, OUTPUT);
    pinMode(PIN_ECHO_L, INPUT);
    pinMode(PIN_ECHO_C, INPUT);
    pinMode(PIN_ECHO_R, INPUT);
    digitalWrite(PIN_TRIG_L, LOW);
    digitalWrite(PIN_TRIG_C, LOW);
    digitalWrite(PIN_TRIG_R, LOW);
}

static float ping(uint8_t trig, uint8_t echo) {
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    unsigned long t = pulseIn(echo, HIGH, US_TIMEOUT_US);
    return (t == 0) ? -1.0f : t * SOUND_CM_US;
}

float ultrasonic_left()   { return ping(PIN_TRIG_L, PIN_ECHO_L); }
float ultrasonic_center() { return ping(PIN_TRIG_C, PIN_ECHO_C); }
float ultrasonic_right()  { return ping(PIN_TRIG_R, PIN_ECHO_R); }
