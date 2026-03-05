/**************************************************************************************************************************
 * MQ6 Sensor Calibration Program

 * The program comprises three main functions:
        1. computeResistance: Converts the raw ADC value from the sensor into a resistance value.
        2. calibrateSensor: Calibrates the sensor by taking multiple readings in clean air to determine Ro.
        3. computePPM: Computes the gas concentration in PPM based on the current sensor resistance and the calibrated Ro.

 * Tweak constants a and b for each specific gas sensor module, as they differ
 * Code works with AVR-cores; requires adjustments for other architectures (e.g., ESP32) due to differences in ADC 
   resolution and timing.
 * Code written by Edmond Francis on Feb 27, 2026
 **************************************************************************************************************************/

#include <math.h>
#include "lcd_format.h"

 // ── Hardware Macros ─────────────────────────────────────────────────────

#define MQ_PIN A0
#define RL_VALUE 20.0          // recommended Load resistance in kΩ
#define CLEAN_AIR_FACTOR 9.5   // Rs/Ro in clean air (MQ6 datasheet)

// ── Sampling Constants ───────────────────────────────────────────────────

const int CALIBRATION_SAMPLES = 50;
const int CALIBRATION_INTERVAL = 500;   // ms between calibration samples
const int WORK_SAMPLES = 25;           // samples under normal operation
const int WORK_INTERVAL = 100;

// ── MQ6 Log-curve coefficients ───────────────────────────────────────────

const double a = -0.358;            // slope  of log-log regression
const double b = 1.077;            // intercept on log(y) axis

// Initialize LCD
LiquidCrystal_I2C lcd (0x27, 16, 2);

double Ro = 0.0;

// ── Forward Declarations ─────────────────────────────────────────────────

double computeResistance(int raw_adc);
double calibrateSensor();
double readSensor();
double computePPM();

void setup() {
    Serial.begin(9600);
	lcd.init();
    lcd.backlight();

    const char* prepMsg = " Calibrating Sensor. Keep in clean air! ";
    lcd.clear();                                     // make sure row1 is empty
    paddedDisplay(lcd, "", 1);
    scrollTicker(lcd, prepMsg, 0, 170, 1);          // first pass
    delay(500);
    scrollTicker(lcd, prepMsg, 0, 170, 1);

    // ─── Static sampling message for whole calibration window ──────────────────

    lcd.clear();
    paddedDisplay(lcd, " Sampling air... ", 0);     // top row only
    Ro = calibrateSensor();                        // blocking calibration (uses delay)

    // ─── Show success marquee + Ro value (bottom static), hold 2s ──────────────

    char roVal[12];
    dtostrf(Ro, 5, 2, roVal); // double → formatted to ASCII string, NUL-terminated
    char roLine[17];
    snprintf(roLine, sizeof(roLine), " Ro = %s kOhm ", roVal);
    lcd.clear();
    paddedDisplay(lcd, roLine, 1);                                        // bottom static
    scrollTicker(lcd, " *** Calibration SUCCESSFUL! *** ", 0, 140, 2);   // top marquee
    delay(2000);

    // ─── Clear and display steady gas concentration UI ────────────────────────

    double ppmNow = computePPM();
    int ippm = (int)round(ppmNow);
    char ppmLine[17];
    snprintf(ppmLine, sizeof(ppmLine), " %d ppm. ", ippm);
    lcd.clear();
    paddedDisplay(lcd, "The gas conc. is : ", 0);
    paddedDisplay(lcd, ppmLine, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  loop: update the same two-line UI: top fixed label, bottom shows Y ppm.
// ─────────────────────────────────────────────────────────────────────────────

void loop() {
    double ppm = computePPM();
    int    ippm = (int)round(ppm);                 // round to nearest integer for display

    char ppmLine[17];
    snprintf(ppmLine, sizeof(ppmLine), " %d ppm. ", ippm);

    paddedDisplay(lcd, "The gas conc. is : ", 0);
    paddedDisplay(lcd, ppmLine, 1);

    delay(1000);
}

// ─────────────────────────────────────────────────────────────────────────────
//  computeResistance
//  Input : Raw 10-bit ADC value
//  Output: Sensor resistance in kΩ
// ─────────────────────────────────────────────────────────────────────────────

double computeResistance(int raw_adc) {
    if (raw_adc <= 0) return RL_VALUE * 1023.0;
    return (double)RL_VALUE * (1023.0 - raw_adc) / (double)raw_adc;
}

// ─────────────────────────────────────────────────────────────────────────────
//  calibrateSensor — blocks CALIBRATION_SAMPLES * CALIBRATION_INTERVAL ms
//  Returns Ro in kΩ
// ─────────────────────────────────────────────────────────────────────────────

double calibrateSensor() {
    double sum = 0.0;
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        sum += computeResistance(analogRead(MQ_PIN));
        delay(CALIBRATION_INTERVAL);
    }
    return (sum / CALIBRATION_SAMPLES) / CLEAN_AIR_FACTOR;
}

// ─────────────────────────────────────────────────────────────────────────────
//  readSensor — returns average Rs over WORK_SAMPLES readings
// ─────────────────────────────────────────────────────────────────────────────

double readSensor() {
    double sum = 0.0;
    for (int i = 0; i < WORK_SAMPLES; i++) {
        sum += computeResistance(analogRead(MQ_PIN));
        delay(WORK_INTERVAL);
    }
    return sum / WORK_SAMPLES;
}

// ─────────────────────────────────────────────────────────────────────────────
//  computePPM — log10(ppm) = (log10(Rs/Ro) - b) / a
// ─────────────────────────────────────────────────────────────────────────────

double computePPM() {
    double Rs = readSensor();
    if (Ro <= 0.0) return 0.0;
    double ratio = Rs / Ro;
    if (ratio <= 0.0) return 0.0;
    return pow(10.0, (log10(ratio) - b) / a);
}