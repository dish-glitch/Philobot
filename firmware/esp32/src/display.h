#pragma once
#include <Arduino.h>

// SSD1306 128×64 OLED connected to J_MAST (J4) I²C header.
// ADDR pin to GND → 0x3C. Change to 0x3D if ADDR is pulled high.
bool display_init();
void display_status(float vbat, float dl, float dc, float dr,
                    int32_t el, int32_t er);
void display_clear();
