#pragma once

// ── Motor control ────────────────────────────────────────────────────────────
// U6 (front) and U7 (rear) share the same control lines.
// A-channel drives left wheels, B-channel drives right wheels.
#define PIN_PWM_L   25   // IO25  → U6/U7 PWMA
#define PIN_L_IN1   26   // IO26  → U6/U7 AIN1
#define PIN_L_IN2   27   // IO27  → U6/U7 AIN2

#define PIN_PWM_R   32   // IO32  → U6/U7 PWMB
#define PIN_R_IN1   33   // IO33  → U6/U7 BIN1
#define PIN_R_IN2   14   // IO14  → U6/U7 BIN2

// ── Encoders ─────────────────────────────────────────────────────────────────
// GPIO34/35 are input-only; no internal pull-up/down available.
#define PIN_ENC_L   34   // IO34
#define PIN_ENC_R   35   // IO35

// ── Ultrasonic (HC-SR04) ─────────────────────────────────────────────────────
// ECHO signals are 5 V; voltage dividers (R3/R7, R1/R2, R8/R9) drop to 3.3 V.
#define PIN_TRIG_L  19   // IO19
#define PIN_ECHO_L   4   // IO4

#define PIN_TRIG_C   5   // IO5
#define PIN_ECHO_C  18   // IO18

#define PIN_TRIG_R  13   // IO13
#define PIN_ECHO_R  23   // IO23

// ── I²C (MPU-6050 @ 0x68, OLEDs via J_MAST) ─────────────────────────────────
#define PIN_SDA     21   // IO21
#define PIN_SCL     22   // IO22

// ── UART to Raspberry Pi (J6) ─────────────────────────────────────────────────
#define PIN_ESP_TX  17   // IO17  → Pi RX
#define PIN_ESP_RX  16   // IO16  → Pi TX

// ── Analog ───────────────────────────────────────────────────────────────────
// Divider: R5=10 kΩ from VBAT, R6=3.3 kΩ to GND
// Vsense = Vbat × 3.3/13.3  →  Vbat = Vsense × 13.3/3.3
#define PIN_VBAT    36   // SENSOR_VP / ADC1_CH0
#define VBAT_RATIO  (13.3f / 3.3f)
#define ADC_BITS    12
#define ADC_FULL    4095.0f
#define ADC_VREF    3.3f

// ── Misc ──────────────────────────────────────────────────────────────────────
#define PIN_LED      2   // IO2  – status LED via R11 (100 Ω)
#define PIN_BOOT     0   // IO0  – BOOT/SW2, pulled high via R12 (10 kΩ)

// ── LEDC PWM ──────────────────────────────────────────────────────────────────
#define PWM_CH_L     0
#define PWM_CH_R     1
#define PWM_FREQ     20000   // 20 kHz – inaudible
#define PWM_BITS     8       // duty 0–255
