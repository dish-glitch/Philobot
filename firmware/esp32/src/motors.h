#pragma once
#include <Arduino.h>

void motors_init();

// speed: -255 (full reverse) … +255 (full forward), 0 = coast
void motor_set(int left_speed, int right_speed);

void motors_stop();   // coast (IN1=IN2=0)
void motors_brake();  // short-brake (IN1=IN2=1, PWM=255)
