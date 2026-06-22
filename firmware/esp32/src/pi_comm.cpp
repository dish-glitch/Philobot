#include "pi_comm.h"
#include "pins.h"

// UART1 – GPIO16 (RX from Pi), GPIO17 (TX to Pi)
static HardwareSerial PiSerial(1);

void pi_comm_init(uint32_t baud) {
    PiSerial.begin(baud, SERIAL_8N1, PIN_ESP_RX, PIN_ESP_TX);
}

bool pi_comm_recv(PiCmd &cmd) {
    if (!PiSerial.available()) return false;
    String line = PiSerial.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("CMD")) return false;

    // "CMD <left> <right> <flags>"
    int a = line.indexOf(' ', 0) + 1;
    int b = line.indexOf(' ', a) + 1;
    int c = line.indexOf(' ', b) + 1;
    cmd.left  = line.substring(a, b - 1).toInt();
    cmd.right = line.substring(b, c - 1).toInt();
    cmd.flags = (uint8_t)line.substring(c).toInt();
    return true;
}

void pi_comm_send(const PhiloStatus &s) {
    PiSerial.printf(
        "STATUS %.2f %.1f %.1f %.1f %ld %ld %.3f %.3f %.3f %.3f\n",
        s.vbat, s.dl, s.dc, s.dr,
        (long)s.el, (long)s.er,
        s.ax, s.ay, s.az, s.gz);
}
