#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "pins.h"
#include "motors.h"
#include "encoders.h"
#include "ultrasonic.h"
#include "imu.h"
#include "display.h"
#include "pi_comm.h"

// ── Safety / timing parameters ───────────────────────────────────────────────
#define WATCHDOG_MS        500    // stop if no CMD from the Pi for this long
#define US_INTERVAL_MS      60    // one sensor per slot — 40 kHz crosstalk guard
#define OBSTACLE_STOP_CM    25    // center sensor: hard stop under this
#define OBSTACLE_VEER_CM    30    // side sensors: veer away under this
#define SENSOR_HOLDOFF       3    // invalid readings in a row → assume blocked
#define TILT_LIMIT_DEG      30    // tip-over cutoff
#define ASL_SHOW_MS       2000    // how long an ASL letter owns the OLED
#define STATUS_MS          100    // telemetry rate (10 Hz)

static float    dist_cm[3]     = {400, 400, 400};   // L, C, R — last valid
static uint8_t  invalid_cnt[3] = {0, 0, 0};
static uint8_t  us_idx         = 0;

static PiCmd    last_cmd       = {0, 0, 1};   // boot state: coast-stopped
static uint32_t last_cmd_ms    = 0;
static bool     tilted         = false;
static uint32_t asl_until_ms   = 0;

static float read_vbat() {
    uint32_t raw = analogRead(PIN_VBAT);
    float vadc = (raw / ADC_FULL) * ADC_VREF;
    return vadc * VBAT_RATIO;
}

// 9 SCL pulses with SDA released — frees a slave left holding SDA low after a
// reset mid-transaction. Must run BEFORE Wire.begin.
static void i2c_bus_recovery() {
    pinMode(PIN_SDA, INPUT_PULLUP);
    pinMode(PIN_SCL, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(PIN_SCL, LOW);
        delayMicroseconds(5);
    }
    digitalWrite(PIN_SCL, HIGH);
    pinMode(PIN_SCL, INPUT_PULLUP);
}

// Fire ONE sensor per 60 ms slot, round-robin. All three share 40 kHz — firing
// together makes each hear the others' pulses as phantom near-range echoes.
static void update_ultrasonic() {
    static uint32_t last_ping_ms = 0;
    uint32_t now = millis();
    if (now - last_ping_ms < US_INTERVAL_MS) return;
    last_ping_ms = now;

    float d = (us_idx == 0) ? ultrasonic_left()
            : (us_idx == 1) ? ultrasonic_center()
                            : ultrasonic_right();

    if (d < 2.0f || d > 400.0f) {
        // Timeout / out of range: hold the last valid reading briefly, then
        // fail safe by assuming an obstacle at 20 cm.
        if (invalid_cnt[us_idx] < SENSOR_HOLDOFF) invalid_cnt[us_idx]++;
        if (invalid_cnt[us_idx] >= SENSOR_HOLDOFF) dist_cm[us_idx] = 20.0f;
    } else {
        dist_cm[us_idx] = d;
        invalid_cnt[us_idx] = 0;
    }
    us_idx = (us_idx + 1) % 3;
}

// Priority: tilt > watchdog > brake/coast flags > obstacle override > Pi cmd
static void apply_drive() {
    if (tilted)                             { motors_stop(); return; }
    if (millis() - last_cmd_ms > WATCHDOG_MS) { motors_stop(); return; }

    if (last_cmd.flags & 0x02) { motors_brake(); return; }
    if (last_cmd.flags & 0x01) { motors_stop();  return; }

    int  l = last_cmd.left;
    int  r = last_cmd.right;
    bool forward = (l > 0 || r > 0);

    if (forward) {
        if (dist_cm[1] < OBSTACLE_STOP_CM) { motors_stop(); return; }
        // Veer AWAY from a side obstacle. The robot turns toward its slower
        // side, so slow the side OPPOSITE the obstacle.
        if      (dist_cm[0] < OBSTACLE_VEER_CM) r = (r * 3) / 5;  // left blocked → turn right
        else if (dist_cm[2] < OBSTACLE_VEER_CM) l = (l * 3) / 5;  // right blocked → turn left
    }
    motor_set(l, r);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);   // LED on during init

    i2c_bus_recovery();
    Wire.begin(PIN_SDA, PIN_SCL);

    motors_init();
    encoders_init();
    ultrasonic_init();

    analogReadResolution(ADC_BITS);
    analogSetAttenuation(ADC_11db);  // full-scale ≈ 3.3 V

    if (!imu_init())      Serial.println("IMU not found – check I2C wiring");
    if (!display_init())  Serial.println("OLED not found – check I2C wiring");

    pi_comm_init(115200);

    Serial.println("Philo ready");
    digitalWrite(PIN_LED, LOW);
}

void loop() {
    static uint32_t last_status_ms = 0;

    // 1. Messages from the Pi
    PiCmd cmd;
    char  letter = 0;
    switch (pi_comm_recv(cmd, letter)) {
        case PI_MSG_CMD:
            last_cmd    = cmd;
            last_cmd_ms = millis();
            break;
        case PI_MSG_ASL:
            display_asl(letter);
            asl_until_ms = millis() + ASL_SHOW_MS;
            break;
        default:
            break;
    }

    // 2. Sensors
    update_ultrasonic();

    // 3. Drive through the safety priority chain (every pass, so the
    //    watchdog fires even when the Pi goes completely silent)
    apply_drive();

    // 4. Telemetry + display at 10 Hz
    uint32_t now = millis();
    if (now - last_status_ms >= STATUS_MS) {
        last_status_ms = now;

        PhiloStatus st{};
        st.vbat = read_vbat();
        st.dl   = dist_cm[0];
        st.dc   = dist_cm[1];
        st.dr   = dist_cm[2];
        st.el   = encoder_left();
        st.er   = encoder_right();

        ImuData imu{};
        if (imu_read(imu)) {
            st.ax = imu.ax;
            st.ay = imu.ay;
            st.az = imu.az;
            st.gz = imu.gz;

            // Tip-over: angle between the gravity vector and the board Z axis
            // (MPU mounted flat). Clears automatically once righted.
            float mag = sqrtf(imu.ax * imu.ax + imu.ay * imu.ay + imu.az * imu.az);
            if (mag > 0.5f) {
                float tilt_deg = acosf(fabsf(imu.az) / mag) * 180.0f / PI;
                tilted = (tilt_deg > TILT_LIMIT_DEG);
            }
        }

        pi_comm_send(st);

        // An ASL letter owns the screen for 2 s, then telemetry resumes
        if ((int32_t)(now - asl_until_ms) >= 0)
            display_status(st.vbat, st.dl, st.dc, st.dr, st.el, st.er);
    }
}
