#include "arm.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PCA9685_ADDR   0x40
#define SERVO_MIN_US    500   // MG996R / SG90 full range is roughly 500-2500 us
#define SERVO_MAX_US   2500

static Adafruit_PWMServoDriver pca(PCA9685_ADDR);
static bool ready = false;

bool arm_init() {
    Wire.beginTransmission(PCA9685_ADDR);          // probe first: begin() can't report a missing board
    if (Wire.endTransmission() != 0) return false;
    pca.begin();
    pca.setOscillatorFrequency(27000000);
    pca.setPWMFreq(50);                            // standard hobby servo frame
    ready = true;
    return true;
}

void arm_set(int ch, int deg) {
    if (!ready || ch < 0 || ch > 15) return;
    deg = constrain(deg, 0, 180);
    pca.writeMicroseconds(ch, map(deg, 0, 180, SERVO_MIN_US, SERVO_MAX_US));
}
