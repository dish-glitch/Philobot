#include "arm.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PCA9685_ADDR   0x40
#define SERVO_MIN_US    500   // MG996R / MG90S full range is roughly 500-2500 us
#define SERVO_MAX_US   2500
#define N_CH             16
#define STEP_MS          10   // one degree every 10 ms ≈ 100 °/s

// Measured safe ranges with the arm assembled. Update these if a joint is re-measured.
struct Limit { int lo, hi; };
static const Limit LIMITS[N_CH] = {
    {10, 170},   // 0 base
    {20, 120},   // 1 shoulder (home 20)
    {0,  180},   // 2 elbow — not re-measured since the links went on; narrow this once known
    {40, 160},   // 3 gripper (parallel gripper, MG90S)
    // 4..15 unused: full range
    {0,180},{0,180},{0,180},{0,180},{0,180},{0,180},
    {0,180},{0,180},{0,180},{0,180},{0,180},{0,180},
};

static Adafruit_PWMServoDriver pca(PCA9685_ADDR);
static bool ready = false;
static int  cur[N_CH];      // last angle written, -1 = never written
static int  target[N_CH];

static void write_deg(int ch, int deg) {
    pca.writeMicroseconds(ch, map(deg, 0, 180, SERVO_MIN_US, SERVO_MAX_US));
    cur[ch] = deg;
}

bool arm_init() {
    for (int i = 0; i < N_CH; i++) cur[i] = target[i] = -1;
    Wire.beginTransmission(PCA9685_ADDR);          // probe first: begin() can't report a missing board
    if (Wire.endTransmission() != 0) return false;
    pca.begin();
    pca.setOscillatorFrequency(27000000);
    pca.setPWMFreq(50);                            // standard hobby servo frame
    ready = true;
    return true;
}

void arm_set(int ch, int deg) {
    if (!ready || ch < 0 || ch >= N_CH) return;
    deg = constrain(deg, LIMITS[ch].lo, LIMITS[ch].hi);
    target[ch] = deg;
    // The servo's real position is unknown until the first write, so jump straight there.
    if (cur[ch] < 0) write_deg(ch, deg);
}

void arm_update() {
    static uint32_t last_ms = 0;
    if (!ready || millis() - last_ms < STEP_MS) return;
    last_ms = millis();
    for (int ch = 0; ch < N_CH; ch++) {
        if (target[ch] < 0 || cur[ch] == target[ch]) continue;
        write_deg(ch, cur[ch] + (target[ch] > cur[ch] ? 1 : -1));
    }
}
