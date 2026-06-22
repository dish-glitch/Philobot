#pragma once
#include <Arduino.h>

// Pi → ESP32: "CMD <left> <right> <flags>\n"
//   flags bit 0 = coast stop, bit 1 = hard brake
struct PiCmd {
    int  left;
    int  right;
    uint8_t flags;
};

// ESP32 → Pi: "STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>\n"
struct PhiloStatus {
    float   vbat;
    float   dl, dc, dr;
    int32_t el, er;
    float   ax, ay, az, gz;
};

void      pi_comm_init(uint32_t baud = 115200);
bool      pi_comm_recv(PiCmd &cmd);
void      pi_comm_send(const PhiloStatus &s);
