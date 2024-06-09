/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */
#include "OnBoardDisplay.h"

#include "Arduino.h"
#include "OnBoardGps.h"

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

// Display the GPS info on the oled screen
void displayInfo()
{
  // Serial.print("Date/Time: ");
  // if (GPS.date.isValid())
  // {
  //   Serial.printf("%d/%02d/%02d",GPS.date.year(),GPS.date.day(),GPS.date.month());
  // }
  // else
  // {
  //   Serial.print("INVALID");
  // }

  // if (GPS.time.isValid())
  // {
  //   Serial.printf(" %02d:%02d:%02d.%02d",GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.time.centisecond());
  // }
  // else
  // {
  //   Serial.print(" INVALID");
  // }
  // Serial.println();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  GpsInfo gps_info;
  gps_get_info(&gps_info);

  Serial.print("LAT: ");
  Serial.print(gps_info.latitude,6);
  Serial.print(", LON: ");
  Serial.print(gps_info.longitude,6);
  Serial.print(", ALT: ");
  Serial.print(gps_info.altitude);

  Serial.println(); 
  
  Serial.print("SATS: ");
  Serial.print(gps_info.satellites_nbr);
  // Serial.print(", HDOP: ");
  // Serial.print(GPS.hdop.hdop());
  // Serial.print(", AGE: ");
  // Serial.print(GPS.location.age());
  // Serial.print(", COURSE: ");
  // Serial.print(GPS.course.deg());
  Serial.print(", SPEED: ");
  Serial.println(gps_info.speed);
  Serial.println();

  display.clear();

  display.drawString(0, 0, "SATS: " + String(gps_info.satellites_nbr));
  uint16_t battery_mv = getBatteryVoltage();
  // display.drawString(50, 0, "batt:" + String(batteryMvToPercent(battery_mv)) + "% (" + String(float(battery_mv)/1000) + "mV");
  display.drawString(0, 10, "LON:" + String(gps_info.longitude));
  display.drawString(0, 20, "LAT:" + String(gps_info.latitude));
  display.drawString(0, 30, "ALT:" + String(gps_info.altitude) + "m");
  display.drawString(0, 40, "SPEED:" + String(gps_info.speed) + "km/h");
  // display.drawString(0, 50, "COURSE:" + String(GPS.course.deg()));

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