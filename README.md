# MQ6-Sensor-Calibration
Arduino Uno calibration sketch for the MQ6 gas sensor. Derives Ro in clean air, computes live LPG/butane PPM via log-log regression, and displays results on a 16×2 I²C LCD. Includes a reusable LCD formatting utility library.
The project separates LCD display utilities into a reusable library module (`lcd_format.h` / `lcd_format.cpp`), keeping the main sketch focused on sensor logic.
---

## Table of Contents

- [Features](#features)
- [File Structure](#file-structure)
- [Hardware Requirements](#hardware-requirements)
- [Wiring](#wiring)
- [Library Dependencies](#library-dependencies)
- [Configuration](#configuration)
- [Calibration Procedure](#calibration-procedure)
- [How It Works](#how-it-works)
- [License](#license)

---

## Features

- Averages 50 ADC readings in clean air to derive a stable **Ro** (baseline resistance)
- Applies MQ6 log-log regression curve to convert Rs/Ro ratio into **PPM**
- Scrolling ticker and padded display utilities for clean LCD output
- Modular design — LCD helpers are fully reusable across other projects

---

## File Structure

```
project-root/
├── MQ6_Calibration.ino   # Main sketch: sensor logic, setup, loop
├── lcd_format.h           # LCD utility declarations
├── lcd_format.cpp         # LCD utility implementations (scroll ticker, padded display)
├── README.md
└── LICENSE
```

> All three source files must live in the **same sketch folder** for the Arduino IDE to compile them together correctly.

---

## Hardware Requirements

| Component | Specification |
|---|---|
| Microcontroller | Arduino Uno R3 |
| Gas Sensor | MQ6 (LPG / butane / propane) |
| LCD Display | 16×2 character LCD with I²C backpack (PCF8574) |
| Load Resistor (RL) | 20 kΩ (must match `RL_VALUE` in sketch) |
| Power Supply | 5 V DC |

> **Warm-up note:** The MQ6 requires **24–48 hours** of continuous power for the first-ever use before calibration results are reliable. For subsequent power cycles, allow at least **3 minutes** of warm-up before running the sketch.

---

## Wiring

### MQ6 Sensor → Arduino Uno

```
MQ6 Pin    Arduino Pin    Notes
─────────────────────────────────────────────────────
VCC        5V             Sensor heater supply
GND        GND
AOUT       A0             Analog signal — matches MQ_PIN macro
DOUT       (not used)     Digital threshold output — leave unconnected
```

> The MQ6 module's on-board load resistor varies by manufacturer. Measure yours with a multimeter and update `RL_VALUE` in the sketch accordingly. An incorrect RL value will skew all resistance and PPM calculations. And if yo built a custom module like me, be sure to use aa fixed 20 kilohm resistor for RL.

### 16×2 I²C LCD → Arduino Uno

```
LCD Pin    Arduino Pin    Notes
──────────────────────────────────────────────────────
VCC        5V
GND        GND
SDA        A4             I²C data line
SCL        A5             I²C clock line
```

> The default I²C address assumed in this sketch is **0x27**. If your LCD does not initialise, try **0x3F**. You can scan for the correct address by running an I²C scanner sketch before deploying this project.

---

## Library Dependencies

Install both libraries via the **Arduino IDE Library Manager** (`Sketch → Include Library → Manage Libraries`):

### 1. LiquidCrystal_I2C
- **Author:** Frank de Brabander
- **Search term:** `LiquidCrystal I2C`
- **Tested version:** 1.1.2

### 2. Wire (built-in)
- Bundled with the Arduino IDE — no installation required.

### 3. math.h (built-in)
- Part of the AVR-GCC standard library — no installation required.

---

## Configuration

All tuneable constants are at the top of `MQ6_Calibration.ino`:

```cpp
// ── Hardware ─────────────────────────────────────
#define MQ_PIN          A0      // Analog pin connected to MQ6 AOUT
#define RL_VALUE        20.0    // Load resistance in kΩ — measure yours
#define CLEAN_AIR_FACTOR 9.5   // Rs/Ro ratio in clean air (MQ6 datasheet)

// ── Sampling ─────────────────────────────────────
const int CALIBRATION_SAMPLES   = 50;   // Number of readings during calibration
const int CALIBRATION_INTERVAL  = 500;  // ms between calibration samples
const int WORK_SAMPLES          = 25;   // Readings averaged per PPM result
const int WORK_INTERVAL         = 100;  // ms between working samples

// ── Log-curve Coefficients (MQ6, LPG) ────────────
const double a = -0.358;   // Slope of log-log regression line
const double b =  1.077;   // Y-intercept of log-log regression line
```

> The constants `a` and `b` are derived from the MQ6 sensitivity curve in the datasheet. They are specific to **LPG detection**. For other gases (propane, iso-butane), extract the corresponding slope and intercept from the datasheet's log-log graph and update these values. In my experience, you've got to do some extrapolation.

---

## Calibration Procedure

The calibration runs automatically on power-up. Follow these steps for accurate results:

1. **Place the sensor in clean air.** Outdoors or in a well-ventilated room away from gas appliances, kitchens, or garages.

2. **Power the Arduino.** The LCD will display a scrolling `Calibrating Sensor...` message, then switch to `Sampling air...`.

3. **Wait ~25 seconds.** The sketch collects 50 readings at 500 ms intervals. Do not expose the sensor to any gas source during this window.

4. **Ro is displayed.** Once complete, the LCD shows the calculated baseline resistance (`Ro = X.XX kOhm`). Note this value — a healthy MQ6 in clean air typically reads **Ro between 2 kΩ and 20 kΩ** depending on the module's load resistor.

5. **Live PPM readings begin.** The sketch immediately starts displaying gas concentration. The loop updates every ~2.5 seconds (25 samples × 100 ms).

### Interpreting Ro

| Ro Value | Likely Cause |
|---|---|
| < 1 kΩ | Sensor exposed to gas during calibration, or RL_VALUE wrong |
| 2 – 20 kΩ | Normal — calibration successful |
| > 50 kΩ | Sensor not warmed up, or faulty heater circuit |

---

## How It Works

### Resistance Calculation

The raw ADC value is converted to sensor resistance using a voltage divider formula:

```
Rs = RL × (ADC_MAX - raw_adc) / raw_adc
```

Where `ADC_MAX = 1023` (10-bit Arduino Uno ADC) and `RL` is the load resistance in kΩ.

### Baseline (Ro) Calculation

During calibration, the sketch averages 50 Rs readings in clean air, then divides by the `CLEAN_AIR_FACTOR` (9.5 for the MQ6) to obtain Ro — the expected sensor resistance at a known baseline concentration.

```
Ro = average(Rs) / CLEAN_AIR_FACTOR
```

### PPM Calculation

Gas concentration is derived from the MQ6 log-log sensitivity curve:

```
log10(ppm) = ( log10(Rs / Ro) - b ) / a
ppm        = 10 ^ ( (log10(Rs/Ro) - b) / a )
```

### LCD Utilities (`lcd_format`)

`scrollTicker()` — Scrolls a message string across a single LCD row a configurable number of times, with padding on both sides so the message enters and exits cleanly.

`paddedDisplay()` — Writes a string to a row and right-pads with spaces to exactly 16 characters, preventing ghost characters from previous longer strings.

---
