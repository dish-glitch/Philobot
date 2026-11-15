#pragma once
#include <Arduino.h>

// Robot arm (EEZYbotArm Mk2) on a PCA9685 servo board, I2C address 0x40,
// sharing the SDA/SCL bus with the OLED (0x3C) and IMU (0x68).
// Servo power comes from the board's own V+ terminal (6 V buck), never the ESP32.
//   ch 0 = base, 1 = shoulder, 2 = elbow, 3 = gripper   (any of the 16 channels work)
//
// arm_set() only sets a target. arm_update() (called every loop) walks each servo
// toward its target a step at a time, so moves are smooth. Every target is first
// clamped to that joint's measured safe range (see LIMITS in arm.cpp), so a bad
// command can't drive a joint into a hard stop and stall the servo.
bool arm_init();                      // false if no PCA9685 answers — arm commands are then ignored
void arm_set(int ch, int deg);        // target angle, clamped to the joint's limits
void arm_update();                    // call every loop()
