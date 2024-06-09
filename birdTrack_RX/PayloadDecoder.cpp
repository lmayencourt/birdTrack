/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

#include "PayloadDecoder.h"

#define HEADER_SIZE 2
#define HEADER_UID_IDX 0
#define HEADER_CMD_IDX 1

// Looks somehow like EG for "Eagle"
#define PAYLOAD_UID 0xE8

#define LATITUDE_IDX 0x00
#define LONGITUDE_IDX LATITUDE_IDX+4
#define ALTITUDE_IDX LONGITUDE_IDX+4

// Private functions
bool verify_header(uint8_t* data_frame, size_t data_size);

bool decode_payload(uint8_t* data_frame, size_t data_size, DecodedPayload payload) {
  if (verify_header(data_frame, data_size)) {
    // Full header not present in data frame
    return false;
  }

  uint8_t cmd = data_frame[HEADER_CMD_IDX];

  switch (cmd) {
    case CMD_FULL_POSITION_UPDATE:
      if (data_size != 14) {
        return false;
      }
      memcpy(&payload.latitude, &data_frame[HEADER_SIZE + LATITUDE_IDX], sizeof(payload.latitude));
      memcpy(&payload.longitude, &data_frame[HEADER_SIZE + LONGITUDE_IDX], sizeof(payload.longitude));
      memcpy(&payload.altitude, &data_frame[HEADER_SIZE + ALTITUDE_IDX], sizeof(payload.altitude));
      break;
    default:
      return false;
  }

  return true;
}

bool verify_header(uint8_t* data_frame, size_t data_size) {
  if (data_size < HEADER_SIZE) {
    // Full header not present in data frame
    return false;
  }

  if (data_frame[HEADER_UID_IDX] != PAYLOAD_UID) {
    return false;
  }

  return true;
}