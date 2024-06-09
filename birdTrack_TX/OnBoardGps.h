/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

#ifndef ON_BOARD_GPS
#define ON_BOARD_GPS

#include "GPS_Air530.h"
#include "GPS_Air530Z.h"

typedef struct GpsInfo {
  float latitude;
  float longitude;
  float altitude;
  
  float speed;

  int satellites_nbr;
} GpsInfo;

typedef struct GpsDateTime {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  uint16_t year;
  uint8_t month;
  uint8_t day;
} GpsTime;

bool gps_config();

// Return true if GPS time is valid
bool gps_is_time_valid();
// Return true if GPS position is valid
bool gps_is_position_valid();

void gps_update(uint32_t timeout);
bool gps_is_locked();

bool gps_get_date_time(GpsTime* GpsDateTime);
bool gps_get_info(GpsInfo* info);

float gpsGetLatitude();
float gpsGetLongitude();
float gpsGetAltitude();

void gps_idle();

#endif // ON_BOARD_GPS
