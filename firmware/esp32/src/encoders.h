#pragma once
#include <Arduino.h>

void encoders_init();

int32_t encoder_left();
int32_t encoder_right();

void encoders_reset();
