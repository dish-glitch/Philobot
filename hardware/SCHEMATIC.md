# Philo — Schematic Build Plan (KiCad)

**Purpose:** This is the schematic, written as text. Every component, every net, every connection. When you sit down in KiCad, you are transcribing this — not designing on the fly. Build it sheet by sheet, run ERC, then move to layout.

Reference designators match [BOM.md](BOM.md).

---

## How to Build This in KiCad (workflow)

1. **Create project:** `philo_pcb` → opens schematic editor (Eeschema).
2. **Use hierarchical sheets** (one per section below) or one flat sheet if you prefer. Six logical blocks: Power, MCU, Motor Drivers, Sensors, Companion (I2C mast), Interface.
3. **Place symbols** from the BOM. For modules (ESP32, D24V50F5, MPU-6050) use the matching module symbol/footprint — Espressif provides the ESP32 one; the D24V50F5 and MPU-6050 use a generic header footprint matching the module's pin layout/pitch.
4. **Wire per the net lists below.** Use named net labels (VBAT, +5V, +3V3, GND, etc.) instead of drawing every wire — much cleaner and less error-prone.
5. **Annotate** (assign reference designators), then **ERC** — fix every error and every meaningful warning.
6. **Assign footprints**, generate netlist, move to PCB layout (follow the layout checklist in hardware/README).

> Note: I can't drive the KiCad GUI for you (it's a desktop EDA tool). But this spec is complete enough that capture is mechanical. Build a sheet, ERC it, then ping me with anything that doesn't reconcile.

---

## Global Power Nets

Use these named nets everywhere instead of long wires:

- **VBAT** — raw battery, 6.4–8.4V (after fuse + reverse protection)
- **+5V** — D24V50F5 buck output (5A)
- **+3V3** — AMS1117 output
- **GND** — single net; the ground plane handles the star-ground topology in layout

---

## SHEET 1 — Power Input & Regulation

### Input protection chain
```
J1.+ (XT30)  →  F1 (7A polyfuse)  →  Q1 DRAIN (P-FET reverse protection) ;  Q1 SOURCE  →  VBAT
J1.-         →  GND
Q1 gate      →  GND through R (10k)
C9  (10µF ceramic)  : VBAT → GND   (at J1, fast transient)
C10 (100µF elec)    : VBAT → GND   (bulk)
```
**Reverse-polarity P-FET orientation (get this right — it is the #1 place this circuit is wired wrong):**
- **Drain → battery side** (incoming +, after the fuse)
- **Source → load side** (VBAT net)
- **Gate → GND via 10k**

Why: a P-FET body diode conducts drain→source. For protection it must conduct battery→load (so the board powers up) and block on reversal. That requires drain=battery, source=load. If you swap them, the board works normally but the body diode dumps reversed voltage into the Pi/ESP32 when the battery is plugged in backwards — i.e., it fails exactly when you need it. In normal operation source sits at ~VBAT, gate at 0V, so Vgs ≈ -7.4V (fully on); at 8.4V full charge Vgs ≈ -8.4V, within the AOD4185 ±20V gate rating (no Zener clamp needed at 2S). Verify the G/D/S pin order against the actual P-FET datasheet before placing the footprint.

### 5V rail — Pololu D24V50F5 (U2, 5V/5A module)
```
U2.VIN   ← VBAT
U2.GND   → GND
U2.VOUT  → +5V
U2.EN    → leave floating (internal pull-up = enabled) or tie to VIN
U2.PG    → optional (power-good open-drain; can drive an LED or leave unconnected)
C7 (1000µF low-ESR) : +5V → GND   (within 5mm of U2 VOUT — Pi inrush)
C8 (100nF)          : +5V → GND
```
Fixed 5.0V output — no trimpot to set. Match the footprint to the D24V50F5 pin layout (VIN, GND, VOUT, plus EN/PG). ~2A of headroom over the ~2.77A load.

### 3.3V rail — AMS1117-3.3 (U3)
```
U3.IN  ← +5V          (NOT VBAT — locked thermal decision)
U3.OUT → +3V3
U3.GND → GND
C3 (10µF) + C5 (100nF) : +5V (U3 in) → GND
C4 (10µF) + C6 (100nF) : +3V3 (U3 out) → GND
```

### Battery sense divider
```
VBAT → R12 (10k) → node "VBAT_SENSE" → R13 (3.3k) → GND
C18 (100nF) : VBAT_SENSE → GND     (filter)
VBAT_SENSE → U1.GPIO36
```

---

## SHEET 2 — MCU (ESP32-WROOM-32, U1)

### Power & boot
```
U1.3V3   ← +3V3
U1.GND   ← GND (all GND pins + thermal pad)
C1 (10µF) + C2 (100nF) : +3V3 → GND, close to U1.3V3
U1.EN    ← R9 (10k) to +3V3 ; C17 (1µF) EN→GND ; SW2 (RESET) EN→GND
U1.GPIO0 ← R11 (10k) to +3V3 ; SW1 (BOOT) GPIO0→GND
```

### Programming header (J_PROG, 6-pin)
```
J_PROG.1 = +3V3
J_PROG.2 = GND
J_PROG.3 = U1.EN
J_PROG.4 = U1.GPIO0
J_PROG.5 = U1.TXD0 (GPIO1)
J_PROG.6 = U1.RXD0 (GPIO3)
```
Manual flashing: hold BOOT, tap RESET, release BOOT. (Optional: add the 2-transistor DTR/RTS auto-reset circuit later — not required.)

### Status LED
```
U1.GPIO2 → R10 (100Ω) → D1 (LED) → GND
```
GPIO2 is a strapping pin (must not be held high at boot). LED to GND with series resistor is fine — it does not force the pin high. Do not add a pull-up here.

### ESP32 pin map (locked — see PRE_BUILD_LOCK A4)
| Function | GPIO | Net name |
|---|---|---|
| Left PWM | 25 | PWM_L |
| Left IN1 | 26 | L_IN1 |
| Left IN2 | 27 | L_IN2 |
| Right PWM | 32 | PWM_R |
| Right IN1 | 33 | R_IN1 |
| Right IN2 | 14 | R_IN2 |
| Left encoder | 34 | ENC_L |
| Right encoder | 35 | ENC_R |
| US center trig | 5 | TRIG_C |
| US center echo | 18 | ECHO_C |
| US left trig | 19 | TRIG_L |
| US left echo | 4 | ECHO_L |
| US right trig | 13 | TRIG_R |
| US right echo | 23 | ECHO_R |
| I2C SDA | 21 | SDA |
| I2C SCL | 22 | SCL |
| UART TX | 17 | ESP_TX |
| UART RX | 16 | ESP_RX |
| Battery ADC | 36 | VBAT_SENSE |
| Status LED | 2 | LED |

---

## SHEET 3 — Motor Drivers (U4, U5)

**Architecture: 4 motors, 4 channels, 2 chips. Each motor = one channel. Left pair shares control signals; right pair shares control signals.**

### Common to both TB6612FNG
```
U4.VM, U5.VM   ← VBAT
U4.VCC, U5.VCC ← +3V3 ; C13/C14 (100nF) each VCC → GND
U4.STBY, U5.STBY ← +3V3   (tie high — drivers always enabled)
U4.GND, U5.GND ← GND (+ exposed pad to GND pour)
C11 (470µF) : VBAT → GND at U4.VM (within 3mm)
C12 (470µF) : VBAT → GND at U5.VM (within 3mm)
```

### Control signal fan-out (the key wiring)
```
PWM_L (GPIO25) → U4.PWMA  AND  U5.PWMA      (both left motors)
L_IN1 (GPIO26) → U4.AIN1  AND  U5.AIN1
L_IN2 (GPIO27) → U4.AIN2  AND  U5.AIN2

PWM_R (GPIO32) → U4.PWMB  AND  U5.PWMB      (both right motors)
R_IN1 (GPIO33) → U4.BIN1  AND  U5.BIN1
R_IN2 (GPIO14) → U4.BIN2  AND  U5.BIN2
```
Each ESP32 output drives two high-impedance CMOS inputs — fine.

### Motor outputs
```
U4.AO1, U4.AO2 → J_ML_F   (left-front motor)
U5.AO1, U5.AO2 → J_ML_R   (left-rear motor)
U4.BO1, U4.BO2 → J_MR_F   (right-front motor)
U5.BO1, U5.BO2 → J_MR_R   (right-rear motor)
```
Mark polarity on silkscreen at each motor connector. If a motor runs backward, swap its two wires — never fix in firmware.

### Encoders (one per side)
```
J_ENC_L: +3V3, GND, out → ENC_L (GPIO34) ; C15 (100nF) ENC_L → GND
J_ENC_R: +3V3, GND, out → ENC_R (GPIO35) ; C16 (100nF) ENC_R → GND
```
Only the front motor's encoder per side is read (GPIO34/35 are input-only, fine for counting). Add 10k pull-up to +3V3 on each encoder line if the encoder output is open-collector (check the specific JGA25-370 encoder variant).

---

## SHEET 4 — Sensors

### Ultrasonic ×3 (HC-SR04) — echo MUST be divided
For each sensor (center/left/right), the echo divider:
```
J_US.Echo (5V) → R_series (1k) → node ECHO_x → R_gnd (2k) → GND
node ECHO_x → ESP32 echo pin
J_US.Trig ← ESP32 trig pin   (3.3V direct, no divider)
J_US.VCC  ← +5V
J_US.GND  ← GND
```
| Sensor | Trig net (pin) | Echo net (pin) | Divider R |
|---|---|---|---|
| Center (J_US1) | TRIG_C (5) | ECHO_C (18) | R1 1k / R4 2k |
| Left (J_US2) | TRIG_L (19) | ECHO_L (4) | R2 1k / R5 2k |
| Right (J_US3) | TRIG_R (13) | ECHO_R (23) | R3 1k / R6 2k |

### IMU (MPU-6050, U6)
```
U6.VCC ← +3V3
U6.GND ← GND
U6.SDA ↔ SDA (GPIO21)
U6.SCL ↔ SCL (GPIO22)
U6.AD0 → GND   (address 0x68)
```

### I2C pull-ups (shared by IMU + both OLEDs)
```
R7 (4.7k): SDA → +3V3
R8 (4.7k): SCL → +3V3
```
One pull-up pair serves the whole bus — do not add more on the OLED modules (remove/ignore their onboard pull-ups if present, or accept the parallel value; check it stays near 2–4.7k effective).

---

## SHEET 5 — Companion (I2C Mast Header)

Both OLED eyes live on the mast and share the I2C bus via one 4-wire header.
```
J_MAST.1 = +3V3
J_MAST.2 = GND
J_MAST.3 = SDA
J_MAST.4 = SCL
```
OLED1 set to 0x3C, OLED2 set to 0x3D (address pad on each module). Driven by ESP32 core 1 (firmware). No additional PCB components.

> Audio (speaker) is NOT on this PCB — it lives on the Pi (USB). Nothing to wire here.

---

## SHEET 6 — Interface to Raspberry Pi

```
J_PI_PWR.1 = +5V   → Pi GPIO pin 2 (5V)
J_PI_PWR.2 = GND   → Pi GPIO pin 6 (GND)

J_UART.1 = ESP_TX (GPIO17) → Pi GPIO pin 10 (RXD)
J_UART.2 = ESP_RX (GPIO16) ← Pi GPIO pin 8 (TXD)
J_UART.3 = GND
```
UART is 3.3V on both ends (ESP32 and Pi) — no level shifting needed. Do not power the Pi over USB-C; use this 5V header.

---

## Net List Summary (master reference)

| Net | Connects |
|---|---|
| VBAT | F1/Q1 out, U2.IN, U4.VM, U5.VM, C9, C10, C11, C12, R12 |
| +5V | U2.OUT, U3.IN, J_PI_PWR.1, J_US VCC ×3, C7, C8 |
| +3V3 | U3.OUT, U1.3V3, U4.VCC, U5.VCC, STBY ×2, U6.VCC, J_MAST.1, pull-ups, C1/C3/C4 |
| GND | everything; ground plane |
| PWM_L / L_IN1 / L_IN2 | GPIO25/26/27 → U4 & U5 channel A inputs |
| PWM_R / R_IN1 / R_IN2 | GPIO32/33/14 → U4 & U5 channel B inputs |
| SDA / SCL | GPIO21/22 ↔ U6, J_MAST, R7/R8 |
| ECHO_C/L/R | GPIO18/4/23 (after dividers) |
| TRIG_C/L/R | GPIO5/19/13 |
| ENC_L / ENC_R | GPIO34/35 |
| VBAT_SENSE | GPIO36 (via R12/R13) |
| ESP_TX / ESP_RX | GPIO17/16 ↔ J_UART |
| LED | GPIO2 → R10 → D1 → GND |

---

## Pre-Layout ERC Checklist

- [ ] Every power pin connected to a power net (no floating VCC)
- [ ] All GND pins + IC exposed pads on GND
- [ ] No net with only one connection (ERC "single pin net")
- [ ] STBY pins tied to +3V3 (not floating — drivers disabled if floating)
- [ ] EN has pull-up + cap; GPIO0 has pull-up
- [ ] Both I2C lines have exactly one pull-up pair
- [ ] All 3 echo lines pass through a divider before the ESP32 pin
- [ ] AMS1117 input net is +5V, not VBAT
- [ ] No GPIO double-assigned (cross-check against pin map table)
- [ ] ERC reports zero errors before moving to layout
