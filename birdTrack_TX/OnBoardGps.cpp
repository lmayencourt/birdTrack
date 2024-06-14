/* SPDX-License-Identifier: MIT
 *
 * This files uses part of code from:
 * https://github.com/HelTecAutomation/CubeCell-Arduino/tree/master/libraries/OnBoardGPS/examples/printGPSInfo
 *
 * Copyright (c) 2024 Louis Mayencourt
 */
#include "OnBoardGps.h"

#include "OnBoardDisplay.h"

// Maximum validaty age for GPS position
#define VALID_AGE_THRESOLD 1000
//when gps waked, if in UPDATE_TIMEOUT, gps not fixed then into low power mode
#define UPDATE_TIMEOUT 1000
//once fixed, CONTINUE_TIME later into low power mode
#define CONTINUE_TIME 10000

Air530ZClass GPS;

bool gps_config() {
  // GPS.setmode(MODE_GPS);
  // GPS + GLONASS provided a valid fix in a few min.
  GPS.setmode(MODE_GPS_GLONASS);
  GPS.begin();
}

void gpsStart() {
  GPS.begin();
}

bool gps_is_time_valid() {
  return GPS.time.isValid();
}

bool gps_is_position_valid() {
  if (GPS.location.age() < VALID_AGE_THRESOLD) {
    return true;
  }

  return false;
}

void gps_update(uint32_t timeout) {
  //GPS.begin();

  uint32_t start_time = millis();
  while( (millis()-start_time) < timeout )
  {
    while (GPS.available() > 0)
    {
      GPS.encode(GPS.read());
    }
  }

  //GPS.end();
}

// Return true if a GPS signal is found
bool gps_is_locked() {
  bool gps_locked = true;

  uint32_t start_time = millis();
  while( (millis()-start_time) < UPDATE_TIMEOUT )
  {
    while (GPS.available() > 0)
    {
      GPS.encode(GPS.read());
    }
    // GPS fixed in a second
    if (GPS.location.age() < 1000) {
      break;
    }
  }

  //if gps fixed,  CONTINUE_TIME later stop GPS into low power mode, and every 1 second update gps, print and display gps info
  if(GPS.location.age() < 1000)
  {
    start_time = millis();
    uint32_t printinfo = 0;
    while( (millis()-start_time) < CONTINUE_TIME )
    {
      while (GPS.available() > 0)
      {
        GPS.encode(GPS.read());
      }

      if( (millis()-start_time) > printinfo )
      {
        printinfo += 1000;
        displayInfo();
      }
    }
  }
  else
  {
    gps_locked = false;
  }

  return gps_locked;
}

bool gps_get_date_time(GpsDateTime* time) {
  time->hour = GPS.time.hour();
  time->minute = GPS.time.minute();
  time->second = GPS.time.second();

  time->year = GPS.date.year();
  time->month = GPS.date.month();
  time->day = GPS.date.day();

  return true;
}

bool gps_get_info(GpsInfo* info) {
  info->latitude = GPS.location.lat();
  info->longitude = GPS.location.lng();

  info->altitude = GPS.altitude.meters();

  info->speed = GPS.speed.kmph();

  info->satellites_nbr = GPS.satellites.value();

  return true;
}

float gpsGetLatitude() {
  return GPS.location.lat();
}

float gpsGetLongitude() {
  return GPS.location.lng();
}

float gpsGetAltitude() {
  return GPS.altitude.meters();
}

unsigned int gpsGetSatellitesNbr() {
  return GPS.satellites.value();
}

unsigned int gpsGetAge() {
  return GPS.location.age();
}

void gps_idle() {
  GPS.end();
}