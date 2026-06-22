#include "imu.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// GY-521 module: AD0 pin tied to GND via U1 pin 7 → I²C address 0x68
static Adafruit_MPU6050 mpu;

bool imu_init() {
    if (!mpu.begin(0x68)) return false;
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    return true;
}

bool imu_read(ImuData &out) {
    sensors_event_t a, g, t;
    if (!mpu.getEvent(&a, &g, &t)) return false;
    out.ax = a.acceleration.x;
    out.ay = a.acceleration.y;
    out.az = a.acceleration.z;
    out.gx = g.gyro.x;
    out.gy = g.gyro.y;
    out.gz = g.gyro.z;
    out.temp_c = t.temperature;
    return true;
}
