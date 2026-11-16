#pragma once
#include <Arduino.h>

// Robot arm (EEZYbotArm Mk2) on a PCA9685 servo board, I2C address 0x40,
// sharing the SDA/SCL bus with the OLED (0x3C) and IMU (0x68).
// Servo power comes from the board's own V+ terminal (6 V buck), never the ESP32.
//   ch 0 = base, 1 = shoulder, 2 = elbow, 3 = gripper   (any of the 16 channels work)
//
// arm_set() only sets a target. arm_update() (called every loop) walks each servo
// toward its target a step at a time, so moves are smooth. Every target is first
// clamped to that joint's measured safe range (see the J[] table in arm.cpp), so a
// bad command can't drive a joint into a hard stop and stall the servo.
//
// Auto-grab (needs the IR sensor, see PIN_GRIP_IR): send channel 99 through the
// normal SERVO path — "SERVO 99 <deg>" turns it on with <deg> as the grip angle,
// "SERVO 99 0" turns it off. While on, an object held between the open jaws makes
// the gripper close to <deg>. Opening it (SERVO 3 50) re-arms once the object is
// taken away. Reports "OBJ 1"/"OBJ 0" and "AUTO GRAB" on the serial link.
#define ARM_AUTO_CH 99
bool arm_init();                      // false if no PCA9685 answers — arm commands are then ignored
void arm_set(int ch, int deg);        // target angle, clamped to the joint's limits
void arm_update();                    // call every loop()
