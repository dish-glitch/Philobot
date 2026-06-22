#pragma once
#include <Arduino.h>

struct ImuData {
    float ax, ay, az;  // acceleration m/s²
    float gx, gy, gz;  // angular rate rad/s
    float temp_c;
};

bool imu_init();
bool imu_read(ImuData &out);
