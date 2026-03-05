#include "lcd_format.h"
#include <string.h>

// Minimal RAM use: small frame bufer of PAD + 1 bytes, allocated on stack

void scrollTicker(LiquidCrystal_I2C &lcd,
    const char* msg,
    int row,
    int delayMs,
    int passes)
{
    constexpr int PAD = 16;                                 // window width, same as LCD columns

    if (msg == nullptr || passes <= 0) return;         // basic sanity: require message and at least 1 pass
    if (row < 0 || row > 1) return;                    // validate 2-line LCD rows

    int msgLen = strlen(msg);
    int total = PAD + msgLen + PAD;

    // Build a 16+1 NUL-terminated frame

    char frame[PAD + 1];
    frame[PAD] = '\0';

	// Scroll the message across the LCD, one character at a time, for the specified number of passes
	// Outer loops control the number of passes and the position of the message
    // Inner loop builds the frame for each position

    for (int p = 0; p < passes; p++) {
        for (int i = 0; i <= total - PAD; i++) {

            for (int c = 0; c < PAD; c++) {
                int pos = i + c;

                if (pos < PAD) {
                    frame[c] = ' ';                    // leading padding
                }
                else if (pos < PAD + msgLen) {
                    frame[c] = msg[pos - PAD];        // message
                }
                else {
                    frame[c] = ' ';                   // trailing padding
                }
            }

            lcd.setCursor(0, row);
            lcd.print(frame);
            delay(delayMs);
        }
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// Print exactly 16 chars on row
// ─────────────────────────────────────────────────────────────────────────────

void paddedDisplay (LiquidCrystal_I2C &lcd, const char* str, int row) {
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    if (str == nullptr) {
        // Nothing to copy, print blank (padded) line instead of crashing
        lcd.setCursor(0, row);
        lcd.print(buf);
        return;
    }

    int len = strlen(str);
    if (len > 16) len = 16;
    memcpy(buf, str, len);
    lcd.setCursor(0, row);
    lcd.print(buf);
}