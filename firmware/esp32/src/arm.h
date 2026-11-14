#pragma once
#include <Arduino.h>

// Robot arm (EEZYbotArm Mk2) on a PCA9685 servo board, I2C address 0x40,
// sharing the SDA/SCL bus with the OLED (0x3C) and IMU (0x68).
// Servo power comes from the board's own V+ terminal (6 V buck), never the ESP32.
//   ch 0 = base, 1 = shoulder, 2 = elbow, 3 = gripper   (any of the 16 channels work)
bool arm_init();                      // false if no PCA9685 answers — arm commands are then ignored
void arm_set(int ch, int deg);        // 0..180 degrees
