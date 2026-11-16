#include "arm.h"
#include "pins.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PCA9685_ADDR   0x40
#define SERVO_MIN_US    500   // MG996R / MG90S full range is roughly 500-2500 us
#define SERVO_MAX_US   2500
#define N_CH             16

// How a joint may go limp (PWM off, ~0 A) once it has been still for relax_ms.
enum Relax : uint8_t {
    RELAX_NEVER,   // must keep holding (shoulder/elbow carry the arm against gravity)
    RELAX_IDLE,    // anywhere, once idle (for a joint with no load and enough friction)
    RELAX_AT_LO,   // only when parked at its low limit (gripper fully open, holding nothing)
};

// Per-joint safety settings. Update lo/hi when a joint is re-measured.
//   lo, hi   : measured safe range — commands outside are clamped, so a joint can't be
//              driven into a hard stop and stall
//   step_ms  : ms per 1° step. Slower = smaller current spikes on the big MG996Rs
//   backoff  : after a move, ease back this many degrees against the travel direction so
//              gears/linkage aren't left pre-loaded (cuts holding current). 0 = off.
//              Kept 0 on gravity joints: backing off there just lets the arm sag.
struct Joint { int lo, hi; uint16_t step_ms; uint8_t backoff; Relax relax; uint16_t relax_ms; };
static const Joint J[N_CH] = {
    {10, 170, 15, 2, RELAX_NEVER,    0},  // 0 base     (MG996R) kept powered: the turntable spins
                                          //   freely on its balls, a limp base would drift
    {20, 120, 15, 0, RELAX_NEVER,    0},  // 1 shoulder (MG996R) home 20
    {45, 155, 15, 0, RELAX_NEVER,    0},  // 2 elbow    (MG996R) measured 40-160; 5° margin because
                                          //   its safe range shifts with the shoulder angle
    {50, 140, 10, 4, RELAX_AT_LO,  500},  // 3 gripper  (MG90S) 50 open, 140 closed; 40/150 hit end stops
    // 4..15 unused
    {0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},
    {0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},
    {0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},
    {0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},{0,180,10,0,RELAX_NEVER,0},
};

static Adafruit_PWMServoDriver pca(PCA9685_ADDR);
static bool     ready = false;
static int      cur[N_CH];          // last angle written, -1 = never written (servo limp)
static int      target[N_CH];
static bool     relaxed[N_CH];
static int8_t   move_dir[N_CH];     // +1/-1 direction of the last commanded move
static bool     backed_off[N_CH];   // back-off already done for that move
static uint32_t last_step[N_CH];
static uint32_t still_since[N_CH];  // when the joint last stopped moving, 0 = moving

// Auto-grab state (gripper = channel 3)
#define GRIP_CH          3
#define OBJ_DEBOUNCE_MS 100   // sensor must agree this long before we believe it
#define REARM_CLEAR_MS  800   // after a grab, jaws must be open and empty this long
static int      auto_deg   = 0;       // 0 = auto-grab off, else the grip angle
static bool     auto_armed = false;
static bool     obj        = false;   // debounced sensor state
static uint32_t obj_change = 0, clear_since = 0;

static void write_deg(int ch, int deg) {
    pca.writeMicroseconds(ch, map(deg, 0, 180, SERVO_MIN_US, SERVO_MAX_US));
    cur[ch] = deg;
    relaxed[ch] = false;
}

bool arm_init() {
    for (int i = 0; i < N_CH; i++) {
        cur[i] = target[i] = -1;
        relaxed[i] = false; move_dir[i] = 0; backed_off[i] = true;
        last_step[i] = still_since[i] = 0;
    }
#ifdef PIN_GRIP_IR
    pinMode(PIN_GRIP_IR, INPUT);
#endif
    Wire.beginTransmission(PCA9685_ADDR);          // probe first: begin() can't report a missing board
    if (Wire.endTransmission() != 0) return false;
    pca.begin();
    pca.setOscillatorFrequency(27000000);
    pca.setPWMFreq(50);                            // standard hobby servo frame
    ready = true;
    return true;
}

void arm_set(int ch, int deg) {
    if (ch == ARM_AUTO_CH) {
#ifdef PIN_GRIP_IR
        auto_deg   = deg > 0 ? constrain(deg, J[GRIP_CH].lo, J[GRIP_CH].hi) : 0;
        auto_armed = false;                        // wait for empty, open jaws first
        if (auto_deg) arm_set(GRIP_CH, J[GRIP_CH].lo);
        Serial.printf("AUTO %d\n", auto_deg);
#else
        Serial.println("AUTO unavailable: no gripper sensor in this build");
#endif
        return;
    }
    if (!ready || ch < 0 || ch >= N_CH) return;
    deg = constrain(deg, J[ch].lo, J[ch].hi);
    target[ch] = deg;
    still_since[ch] = 0;
    if (cur[ch] >= 0 && deg != cur[ch]) {
        move_dir[ch]   = deg > cur[ch] ? 1 : -1;
        backed_off[ch] = false;
    }
    // The servo's real position is unknown until the first write, so jump straight there.
    // A relaxed servo hasn't moved (nothing drives it), so re-enable it where it was.
    if (cur[ch] < 0) write_deg(ch, deg);
    else if (relaxed[ch]) write_deg(ch, cur[ch]);
}

static bool gripper_open() {
    const Joint &j = J[GRIP_CH];
    return target[GRIP_CH] >= 0 && target[GRIP_CH] <= j.lo + j.backoff &&
           cur[GRIP_CH]    <= j.lo + j.backoff;
}

static void auto_grab_update(uint32_t now) {
#ifdef PIN_GRIP_IR
    // Debounce the sensor and report changes, so it can be checked from the laptop.
    static bool raw_last = false;
    const bool raw = digitalRead(PIN_GRIP_IR) == GRIP_IR_ACTIVE;
    if (raw != raw_last) { raw_last = raw; obj_change = now; }
    if (raw != obj && now - obj_change >= OBJ_DEBOUNCE_MS) {
        obj = raw;
        Serial.printf("OBJ %d\n", obj ? 1 : 0);
    }
    if (!auto_deg) return;

    // Re-arm only once the jaws are open AND empty for a moment — otherwise it
    // would grab again the instant it opened, while the object is still there.
    if (!auto_armed) {
        if (gripper_open() && !obj) {
            if (!clear_since) clear_since = now;
            if (now - clear_since >= REARM_CLEAR_MS) auto_armed = true;
        } else {
            clear_since = 0;
        }
        return;
    }
    if (obj && gripper_open()) {
        arm_set(GRIP_CH, auto_deg);
        auto_armed  = false;
        clear_since = 0;
        Serial.println("AUTO GRAB");
    }
#else
    (void)now;
#endif
}

void arm_update() {
    if (!ready) return;
    const uint32_t now = millis();
    auto_grab_update(now);
    for (int ch = 0; ch < N_CH; ch++) {
        if (target[ch] < 0) continue;              // never commanded: leave it limp
        const Joint &j = J[ch];

        if (cur[ch] != target[ch]) {               // still travelling: one step at this joint's pace
            if (now - last_step[ch] >= j.step_ms) {
                last_step[ch] = now;
                write_deg(ch, cur[ch] + (target[ch] > cur[ch] ? 1 : -1));
            }
            continue;
        }
        if (!backed_off[ch]) {                     // just arrived: ease off the pre-load
            backed_off[ch] = true;
            if (j.backoff) {
                target[ch] = constrain(cur[ch] - move_dir[ch] * j.backoff, j.lo, j.hi);
                continue;
            }
        }
        if (relaxed[ch] || j.relax == RELAX_NEVER) continue;
        if (j.relax == RELAX_AT_LO && cur[ch] > j.lo + j.backoff) { still_since[ch] = 0; continue; }
        if (!still_since[ch]) still_since[ch] = now;
        if (now - still_since[ch] >= j.relax_ms) {
            pca.setPWM(ch, 0, 0);                  // no pulses: servo stops driving, ~0 A
            relaxed[ch] = true;
        }
    }
}
