/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */
#include "OnBoardDisplay.h"

#include "Arduino.h"
#include "ArduinoLog.h"

#include "OnBoardGps.h"

enum InfoScreen {
  INFOSCREEN_STATE = 0,
  INFOSCREEN_GPS,
  INFOSCREEN_LORA,
  INFOSCREEN_OFF,
  INFOSCREEN_MAX_IDX,
};

static InfoScreen info_screen_idx = INFOSCREEN_STATE;

// Turn on the OLed display
void display_on() {
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);
}

void display_off() {
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void displayNextScreen() {
  switch(info_screen_idx) {
    case INFOSCREEN_STATE: info_screen_idx = INFOSCREEN_GPS; break;
    case INFOSCREEN_GPS: info_screen_idx = INFOSCREEN_STATE; break;
    case INFOSCREEN_LORA: info_screen_idx = INFOSCREEN_OFF; break;
    case INFOSCREEN_OFF: info_screen_idx = INFOSCREEN_STATE; break;
    default: info_screen_idx = INFOSCREEN_STATE; break;
  }
}

// Display the GPS info on the oled screen
void displayInfo(char* current_state)
{
  Log.noticeln("Displaying state: %d", info_screen_idx);
  GpsInfo gps_info;
  gps_get_info(&gps_info);

  Log.notice("LAT: %f ", gps_info.latitude);
  Log.notice("LON: %f ", gps_info.longitude);
  Log.noticeln("ALT: %f", gps_info.altitude);

  Log.notice("SATS: %d", gps_info.satellites_nbr);
  Log.noticeln(", SPEED: %f", gps_info.speed);

  uint16_t battery_mv = getBatteryVoltage();

  display.clear();
  switch (info_screen_idx) {
    case INFOSCREEN_STATE:
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 0, current_state);
      break;
    case INFOSCREEN_GPS:
        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_LEFT);

        display.drawString(0, 0, "SATS: " + String(gps_info.satellites_nbr));
        display.drawString(50, 0, "batt:" + String(batteryMvToPercent(battery_mv)) + "% (" + String(float(battery_mv)/1000) + "mV");
        display.drawString(0, 10, "LON:" + String(gps_info.longitude));
        display.drawString(0, 20, "LAT:" + String(gps_info.latitude));
        display.drawString(0, 30, "ALT:" + String(gps_info.altitude) + "m");
        display.drawString(0, 40, "SPEED:" + String(gps_info.speed) + "km/h");
        // display.drawString(0, 50, "COURSE:" + String(GPS.course.deg()));
      break;
    case INFOSCREEN_LORA:
      break;
    // case INFOSCREEN_OFF:
    //   break;
    default:
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 0, "Error IDX");
      break;
  }

  display.display();
}

// Battery mV to percent, with values from
// https://stackoverflow.com/questions/56266857/how-do-i-convert-battery-voltage-into-battery-percentage-on-a-4-15v-li-ion-batte
uint8_t batteryMvToPercent(uint16_t voltage) {
  if (voltage >= 4200) {
    return 100;
  } else if (voltage >= 4100) {
    return 90;
  } else if (voltage >= 4000) {
    return 80;
  } else if (voltage >= 3950) {
    return 70;
  } else if (voltage >= 3900) {
    return 60;
  } else if (voltage >= 3850) {
    return 50;
  } else if (voltage >= 3800) {
    return 40;
  } else if (voltage >= 3750) {
    return 30;
  } else if (voltage >= 3700) {
    return 20;
  } else if (voltage >= 3650) {
    return 10;
  } else {
    return 0;
  }
}