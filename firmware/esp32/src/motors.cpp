#include "motors.h"
#include "pins.h"

void motors_init() {
    pinMode(PIN_L_IN1, OUTPUT);
    pinMode(PIN_L_IN2, OUTPUT);
    pinMode(PIN_R_IN1, OUTPUT);
    pinMode(PIN_R_IN2, OUTPUT);

    ledcAttach(PIN_PWM_L, PWM_FREQ, PWM_BITS);
    ledcAttach(PIN_PWM_R, PWM_FREQ, PWM_BITS);

    motors_stop();
}

static void drive_side(uint8_t in1, uint8_t in2, uint8_t pwm_pin, int speed) {
    speed = constrain(speed, -255, 255);
    if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(pwm_pin, (uint8_t)speed);
    } else if (speed < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(pwm_pin, (uint8_t)(-speed));
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(pwm_pin, 0);
    }
}

void motor_set(int left_speed, int right_speed) {
    drive_side(PIN_L_IN1, PIN_L_IN2, PIN_PWM_L, left_speed);
    drive_side(PIN_R_IN1, PIN_R_IN2, PIN_PWM_R, right_speed);
}

void motors_stop() {
    motor_set(0, 0);
}

void motors_brake() {
    digitalWrite(PIN_L_IN1, HIGH);
    digitalWrite(PIN_L_IN2, HIGH);
    ledcWrite(PIN_PWM_L, 255);

    digitalWrite(PIN_R_IN1, HIGH);
    digitalWrite(PIN_R_IN2, HIGH);
    ledcWrite(PIN_PWM_R, 255);
}
