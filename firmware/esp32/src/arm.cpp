#include "arm.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PCA9685_ADDR   0x40
#define SERVO_MIN_US    500   // MG996R / MG90S full range is roughly 500-2500 us
#define SERVO_MAX_US   2500
#define N_CH             16
#define STEP_MS          10   // one degree every 10 ms ≈ 100 °/s
#define GRIPPER_CH        3
#define RELAX_MS        500   // gripper parked open this long -> signal off (0 A)
#define BACKOFF_DEG       4   // after a gripper move, ease back this far against the travel
                              // direction so gears/arms aren't left pre-loaded (cuts holding
                              // current to ~0). Raise if it still hums after moves, 0 = off.

// Measured safe ranges with the arm assembled. Update these if a joint is re-measured.
struct Limit { int lo, hi; };
static const Limit LIMITS[N_CH] = {
    {10, 170},   // 0 base
    {20, 120},   // 1 shoulder (home 20)
    {0,  180},   // 2 elbow — not re-measured since the links went on; narrow this once known
    {50, 140},   // 3 gripper (parallel gripper, MG90S). 50 = open, 140 = closed; 40 and 150 hit the end stops
    // 4..15 unused: full range
    {0,180},{0,180},{0,180},{0,180},{0,180},{0,180},
    {0,180},{0,180},{0,180},{0,180},{0,180},{0,180},
};

static Adafruit_PWMServoDriver pca(PCA9685_ADDR);
static bool ready = false;
static int  cur[N_CH];      // last angle written, -1 = never written
static int  target[N_CH];
static bool relaxed[N_CH];
static int  move_dir   = 0;      // gripper: +1/-1 direction of the last commanded move
static bool backed_off = true;   // gripper: back-off already done for that move

static void write_deg(int ch, int deg) {
    pca.writeMicroseconds(ch, map(deg, 0, 180, SERVO_MIN_US, SERVO_MAX_US));
    cur[ch] = deg;
    relaxed[ch] = false;
}

bool arm_init() {
    for (int i = 0; i < N_CH; i++) { cur[i] = target[i] = -1; relaxed[i] = false; }
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
    // A relaxed servo hasn't moved (nothing drives it), so re-enable it where it was.
    if (ch == GRIPPER_CH && cur[ch] >= 0 && deg != cur[ch]) {
        move_dir   = deg > cur[ch] ? 1 : -1;
        backed_off = false;
    }
    if (cur[ch] < 0) write_deg(ch, deg);
    else if (relaxed[ch]) write_deg(ch, cur[ch]);
}

void arm_update() {
    static uint32_t last_ms = 0, parked_since = 0;
    if (!ready || millis() - last_ms < STEP_MS) return;
    last_ms = millis();
    for (int ch = 0; ch < N_CH; ch++) {
        if (target[ch] < 0 || cur[ch] == target[ch]) continue;
        write_deg(ch, cur[ch] + (target[ch] > cur[ch] ? 1 : -1));
    }
    // Gripper only: when parked fully open it isn't holding anything, so cut the
    // signal and let the servo go idle instead of fighting friction at ~0.05-0.2 A.
    // Shoulder/elbow must keep holding against gravity, so they are never relaxed.
    const int g = GRIPPER_CH;
    if (!backed_off && target[g] >= 0 && cur[g] == target[g]) {
        // Arrived: ease back a few degrees the other way so the mechanism isn't left
        // pre-loaded. (This doesn't help when an object blocks the jaws short of the
        // target — a grab should command an angle just past contact, not full close.)
        target[g]  = constrain(cur[g] - move_dir * BACKOFF_DEG, LIMITS[g].lo, LIMITS[g].hi);
        backed_off = true;
    }
    if (backed_off && cur[g] <= LIMITS[g].lo + BACKOFF_DEG && target[g] == cur[g] && !relaxed[g]) {
        if (!parked_since) parked_since = millis();
        if (millis() - parked_since >= RELAX_MS) {
            pca.setPWM(g, 0, 0);       // no pulses: servo stops driving
            relaxed[g] = true;
        }
    } else {
        parked_since = 0;
    }
}
