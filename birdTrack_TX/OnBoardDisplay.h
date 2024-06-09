/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

#ifndef ON_BOARD_DISPLAY
#define ON_BOARD_DISPLAY

// The display driver and Wire.h libraries are defined in LoraWan_App
#include "LoRaWan_APP.h"
// #include "Arduino.h"

// OLED Screen
#include "HT_SSD1306Wire.h"
extern SSD1306Wire display;

// Function prototypes
void display_on();
void displayInfo();
uint8_t batteryMvToPercent(uint16_t voltage);

#endif // ON_BOARD_DISPLAY
