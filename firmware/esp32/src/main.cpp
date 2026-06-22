#include <Arduino.h>
#include <Wire.h>

#include "pins.h"
#include "motors.h"
#include "encoders.h"
#include "ultrasonic.h"
#include "imu.h"
#include "display.h"
#include "pi_comm.h"

static float read_vbat() {
    uint32_t raw = analogRead(PIN_VBAT);
    float vadc = (raw / ADC_FULL) * ADC_VREF;
    return vadc * VBAT_RATIO;
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);   // LED on during init

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

    // Process any incoming command from Pi
    PiCmd cmd;
    if (pi_comm_recv(cmd)) {
        if      (cmd.flags & 0x02) motors_brake();
        else if (cmd.flags & 0x01) motors_stop();
        else                       motor_set(cmd.left, cmd.right);
    }

    // Publish status at ~10 Hz
    uint32_t now = millis();
    if (now - last_status_ms >= 100) {
        last_status_ms = now;

        PhiloStatus st{};
        st.vbat = read_vbat();
        st.dl   = ultrasonic_left();
        st.dc   = ultrasonic_center();
        st.dr   = ultrasonic_right();
        st.el   = encoder_left();
        st.er   = encoder_right();

        ImuData imu{};
        if (imu_read(imu)) {
            st.ax = imu.ax;
            st.ay = imu.ay;
            st.az = imu.az;
            st.gz = imu.gz;
        }

        pi_comm_send(st);
        display_status(st.vbat, st.dl, st.dc, st.dr, st.el, st.er);
    }
}
