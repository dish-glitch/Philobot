#pragma once
#include <Arduino.h>

void ultrasonic_init();

// Returns distance in cm. Returns -1 if no echo within range.
float ultrasonic_left();
float ultrasonic_center();
float ultrasonic_right();
