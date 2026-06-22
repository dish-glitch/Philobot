#include "encoders.h"
#include "pins.h"

static volatile int32_t cnt_l = 0;
static volatile int32_t cnt_r = 0;

static void IRAM_ATTR isr_l() { cnt_l++; }
static void IRAM_ATTR isr_r() { cnt_r++; }

void encoders_init() {
    // GPIO34/35 are input-only with no internal pull resistors
    pinMode(PIN_ENC_L, INPUT);
    pinMode(PIN_ENC_R, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), isr_l, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), isr_r, RISING);
}

int32_t encoder_left()  { return cnt_l; }
int32_t encoder_right() { return cnt_r; }

void encoders_reset() {
    portDISABLE_INTERRUPTS();
    cnt_l = cnt_r = 0;
    portENABLE_INTERRUPTS();
}
