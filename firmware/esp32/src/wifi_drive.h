#pragma once
#include <Arduino.h>

// Phone control over WiFi (build flag WIFI_DRIVE).
// The ESP32 runs its own WiFi network; a phone joins it and opens
// http://192.168.4.1 for an on-screen joystick.
void wifi_drive_init();

// Call every loop. Returns true when the phone sent a new command.
bool wifi_drive_poll(int &left, int &right, uint8_t &flags);
