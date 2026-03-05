#ifndef LCDFORMAT_H
#define LCDFORMAT_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

void scrollTicker (LiquidCrystal_I2C &lcd, 
				   const char* msg, 
				   int row, 
				   int delayMs, 
			       int passes);

void paddedDisplay (LiquidCrystal_I2C &lcd,
					const char* str, 
					int row);

#endif