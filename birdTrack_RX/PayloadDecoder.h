/* SPDX-License-Identifier: MIT
 *
 * Provides the functions to decode the custom LoRa payloads used to
 * communicate between the RX and TX node.
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

#ifndef PAYLOAD_DECODER
#define PAYLOAD_DECODER

#include <stdint.h>
#include "Arduino.h"

// Available commands
#define CMD_FULL_POSITION_UPDATE 0x00

typedef struct DecodedPayload {
  // data header
  uint8_t command;

  // position fields
  float latitude;
  float longitude;
  float altitude;
} DecodedPayload;

bool decode_payload(uint8_t* data_frame, size_t data_size, DecodedPayload payload);

#endif // PAYLOAD_DECODER
