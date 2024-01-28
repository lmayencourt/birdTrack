/* SPDX-License-Identifier: MIT
 *
 * This files uses part of code from:
 * https://github.com/HelTecAutomation/CubeCell-Arduino/tree/master/libraries/OnBoardGPS/examples/printGPSInfo
 *
 * Copyright (c) 2023 Louis Mayencourt
 */

#include "GPS_Air530.h"
#include "GPS_Air530Z.h"

//if GPS module is Air530, use this
//Air530Class GPS;

//if GPS module is Air530Z, use this
Air530ZClass GPS;

// OLED Screen
#include <Wire.h>
#include "HT_SSD1306Wire.h"
SSD1306Wire  display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr , freq , SDA, SCL, resolution , rst

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup()
{
  Serial.begin(115200);
  GPS.setmode(MODE_GPS_GLONASS);
  GPS.begin();

  Serial.println(F("A simple demonstration of Air530 module"));
  Serial.println();

  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);

  displayInfo();
}

void loop(){
  uint32_t starttime = millis();
  while( (millis()-starttime) < 1000 )
  {
    while (GPS.available() > 0)
    {
      GPS.encode(GPS.read());
    }
  }
  displayInfo();
  if (millis() > 5000 && GPS.charsProcessed() < 10)
  {
    Serial.println("No GPS detected: check wiring.");
    while(true);
  }
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

void displayInfo()
{
  Serial.print("Date/Time: ");
  if (GPS.date.isValid())
  {
    Serial.printf("%d/%02d/%02d",GPS.date.year(),GPS.date.day(),GPS.date.month());
  }
  else
  {
    Serial.print("INVALID");
  }

  if (GPS.time.isValid())
  {
    Serial.printf(" %02d:%02d:%02d.%02d",GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.time.centisecond());
  }
  else
  {
    Serial.print(" INVALID");
  }
  Serial.println();
  
  Serial.print("LAT: ");
  Serial.print(GPS.location.lat(),6);
  Serial.print(", LON: ");
  Serial.print(GPS.location.lng(),6);
  Serial.print(", ALT: ");
  Serial.print(GPS.altitude.meters());

  Serial.println(); 
  
  Serial.print("SATS: ");
  Serial.print(GPS.satellites.value());
  Serial.print(", HDOP: ");
  Serial.print(GPS.hdop.hdop());
  Serial.print(", AGE: ");
  Serial.print(GPS.location.age());
  Serial.print(", COURSE: ");
  Serial.print(GPS.course.deg());
  Serial.print(", SPEED: ");
  Serial.println(GPS.speed.kmph());
  Serial.println();

  display.clear();

  display.drawString(0, 0, "SATS: " + String(GPS.satellites.value()));
  uint16_t battery_mv = getBatteryVoltage();
  display.drawString(50, 0, "batt:" + String(batteryMvToPercent(battery_mv)) + "% (" + String(float(battery_mv)/1000) + "mV");
  display.drawString(0, 10, "LAT:" + String(GPS.location.lng()));
  display.drawString(0, 20, "LON:" + String(GPS.location.lat()));
  display.drawString(0, 30, "ALT:" + String(GPS.altitude.meters()) + "m");
  display.drawString(0, 40, "SPEED:" + String(GPS.speed.kmph()) + "km/h");
  display.drawString(0, 50, "COURSE:" + String(GPS.course.deg()));

  display.display();
}
