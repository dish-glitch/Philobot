// I2C scanner — confirms which devices are wired to the Arduino's I2C bus.
// Expected for Philo bench test: 0x3C (OLED) and 0x68 (MPU-6050).
// Open Serial Monitor at 115200 baud after uploading.

#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("\nI2C scanner ready.");
}

void loop() {
  byte count = 0;
  Serial.println("Scanning...");

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      if (addr == 0x3C || addr == 0x3D) Serial.print("  <- OLED");
      if (addr == 0x68)                 Serial.print("  <- MPU-6050");
      Serial.println();
      count++;
    }
  }

  if (count == 0) Serial.println("  No devices found — check SDA/SCL/power wiring.");
  Serial.println();
  delay(2000);
}
