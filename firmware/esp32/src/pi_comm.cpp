#include "pi_comm.h"
#include "pins.h"

#include <stdio.h>
#include <string.h>

// UART1 – GPIO16 (RX from Pi), GPIO17 (TX to Pi)
static HardwareSerial PiSerial(1);

static char    rx_buf[64];
static uint8_t rx_len = 0;

void pi_comm_init(uint32_t baud) {
    PiSerial.begin(baud, SERIAL_8N1, PIN_ESP_RX, PIN_ESP_TX);
    // Discard anything queued while we were off/resetting (Pi boot messages,
    // stale commands from before a watchdog reset).
    delay(50);
    while (PiSerial.available()) PiSerial.read();
    rx_len = 0;
}

static PiMsgType parse_line(const char *line, PiCmd &cmd, char &asl_letter) {
    int l, r, f;
    if (sscanf(line, "CMD %d %d %d", &l, &r, &f) == 3) {
        cmd.left  = l;
        cmd.right = r;
        cmd.flags = (uint8_t)f;
        return PI_MSG_CMD;
    }
    if (strncmp(line, "ASL ", 4) == 0 && line[4] != '\0') {
        asl_letter = line[4];
        return PI_MSG_ASL;
    }
    return PI_MSG_NONE;   // Pi boot noise or malformed line — ignore
}

PiMsgType pi_comm_recv(PiCmd &cmd, char &asl_letter) {
    // Non-blocking: accumulate bytes and parse only when a full line is in.
    // (readStringUntil would stall the control loop up to 1 s on a partial
    // line, which would starve the watchdog and obstacle checks.)
    while (PiSerial.available()) {
        char ch = (char)PiSerial.read();
        if (ch == '\n' || ch == '\r') {
            if (rx_len == 0) continue;
            rx_buf[rx_len] = '\0';
            rx_len = 0;
            PiMsgType t = parse_line(rx_buf, cmd, asl_letter);
            if (t != PI_MSG_NONE) return t;
            continue;
        }
        if (rx_len < sizeof(rx_buf) - 1) rx_buf[rx_len++] = ch;
        else rx_len = 0;   // oversized garbage — drop and restart the line
    }
    return PI_MSG_NONE;
}

void pi_comm_send(const PhiloStatus &s) {
    PiSerial.printf(
        "STATUS %.2f %.1f %.1f %.1f %ld %ld %.3f %.3f %.3f %.3f\n",
        s.vbat, s.dl, s.dc, s.dr,
        (long)s.el, (long)s.er,
        s.ax, s.ay, s.az, s.gz);
}
