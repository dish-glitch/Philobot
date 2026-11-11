#pragma once
#include <Arduino.h>

// Pi → ESP32:
//   "CMD <left> <right> <flags>\n"   flags bit 0 = coast stop, bit 1 = hard brake
//   "ASL <letter>\n"                 show the letter big on the OLED for 2 s
//   "SERVO <ch> <deg>\n"             move arm servo <ch> (PCA9685 channel) to <deg> 0..180
struct PiCmd {
    int  left;
    int  right;
    uint8_t flags;
};

enum PiMsgType : uint8_t {
    PI_MSG_NONE = 0,   // nothing complete yet, or an unrecognized line
    PI_MSG_CMD,
    PI_MSG_ASL,
    PI_MSG_SERVO,      // read the values with pi_comm_servo()
};

// ESP32 → Pi: "STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>\n"
struct PhiloStatus {
    float   vbat;
    float   dl, dc, dr;
    int32_t el, er;
    float   ax, ay, az, gz;
};

void      pi_comm_init(uint32_t baud = 115200);
PiMsgType pi_comm_recv(PiCmd &cmd, char &asl_letter);
void      pi_comm_servo(int &ch, int &deg);   // last SERVO message
void      pi_comm_send(const PhiloStatus &s);
