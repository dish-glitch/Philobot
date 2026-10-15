#include "motors.h"
#include "pins.h"

#ifdef DRV8833
// DRV8833 has no PWM pin: PWM goes on the IN pins themselves.
//   forward: IN1 = PWM, IN2 = 0     reverse: IN1 = 0, IN2 = PWM
//   coast:   IN1 = IN2 = 0          brake:   IN1 = IN2 = 255
// nSLEEP/STBY must be tied to 3.3V on the module (PIN_PWM_L/R are unused).
static const uint8_t CH_L1 = 0, CH_L2 = 1, CH_R1 = 2, CH_R2 = 3;

void motors_init() {
    ledcSetup(CH_L1, PWM_FREQ, PWM_BITS); ledcAttachPin(PIN_L_IN1, CH_L1);
    ledcSetup(CH_L2, PWM_FREQ, PWM_BITS); ledcAttachPin(PIN_L_IN2, CH_L2);
    ledcSetup(CH_R1, PWM_FREQ, PWM_BITS); ledcAttachPin(PIN_R_IN1, CH_R1);
    ledcSetup(CH_R2, PWM_FREQ, PWM_BITS); ledcAttachPin(PIN_R_IN2, CH_R2);
    motors_stop();
}

static void drive_side(uint8_t ch1, uint8_t ch2, int speed) {
    speed = constrain(speed, -255, 255);
    ledcWrite(ch1, speed > 0 ? speed : 0);
    ledcWrite(ch2, speed < 0 ? -speed : 0);
}

void motor_set(int left_speed, int right_speed) {
    drive_side(CH_L1, CH_L2, left_speed);
    drive_side(CH_R1, CH_R2, right_speed);
}

void motors_stop() {
    motor_set(0, 0);
}

void motors_brake() {
    ledcWrite(CH_L1, 255); ledcWrite(CH_L2, 255);
    ledcWrite(CH_R1, 255); ledcWrite(CH_R2, 255);
}
#else
void motors_init() {
    pinMode(PIN_L_IN1, OUTPUT);
    pinMode(PIN_L_IN2, OUTPUT);
    pinMode(PIN_R_IN1, OUTPUT);
    pinMode(PIN_R_IN2, OUTPUT);

    ledcSetup(PWM_CH_L, PWM_FREQ, PWM_BITS);
    ledcAttachPin(PIN_PWM_L, PWM_CH_L);
    ledcSetup(PWM_CH_R, PWM_FREQ, PWM_BITS);
    ledcAttachPin(PIN_PWM_R, PWM_CH_R);

    motors_stop();
}

static void drive_side(uint8_t in1, uint8_t in2, uint8_t ch, int speed) {
    speed = constrain(speed, -255, 255);
    if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(ch, (uint8_t)speed);
    } else if (speed < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(ch, (uint8_t)(-speed));
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(ch, 0);
    }
}

void motor_set(int left_speed, int right_speed) {
    drive_side(PIN_L_IN1, PIN_L_IN2, PWM_CH_L, left_speed);
    drive_side(PIN_R_IN1, PIN_R_IN2, PWM_CH_R, right_speed);
}

void motors_stop() {
    motor_set(0, 0);
}

void motors_brake() {
    digitalWrite(PIN_L_IN1, HIGH);
    digitalWrite(PIN_L_IN2, HIGH);
    ledcWrite(PWM_CH_L, 255);

    digitalWrite(PIN_R_IN1, HIGH);
    digitalWrite(PIN_R_IN2, HIGH);
    ledcWrite(PWM_CH_R, 255);
}
#endif  // DRV8833
