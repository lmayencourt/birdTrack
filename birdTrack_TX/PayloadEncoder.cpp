#include <cstring>
/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

#include "PayloadEncoder.h"

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

size_t encode_payload(DecodedPayload payload, uint8_t* data_frame, size_t data_size) {
  if (data_size < HEADER_SIZE) {
    return 0;
  }

  // Build header
  data_frame[HEADER_UID_IDX] = PAYLOAD_UID;
  data_frame[HEADER_CMD_IDX] = CMD_FULL_POSITION_UPDATE;

  // Build full position update payload
  memcpy(&data_frame[HEADER_SIZE + LATITUDE_IDX], &payload.latitude, sizeof(payload.latitude));
  memcpy(&data_frame[HEADER_SIZE + LONGITUDE_IDX], &payload.longitude, sizeof(payload.longitude));
  memcpy(&data_frame[HEADER_SIZE + ALTITUDE_IDX], &payload.altitude, sizeof(payload.altitude));

  return HEADER_SIZE + 3*4;
}

bool decode_payload(uint8_t* data_frame, size_t data_size, DecodedPayload payload) {
  if (verify_header(data_frame, data_size)) {
    // Full header not present in data frame
    return false;
  }

  uint8_t cmd = data_frame[HEADER_CMD_IDX];

  switch (cmd) {
    case CMD_FULL_POSITION_UPDATE:
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