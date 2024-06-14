/* SPDX-License-Identifier: MIT
 *
 * This files uses part of code from:
 * https://github.com/ropg/heltec_esp32_lora_v3/blob/main/examples/LoRa_rx_tx/LoRa_rx_tx.ino
 *
 * Arduino board needed is Heltec WiFi LoRa 32
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

/**
 * Send and receive LoRa-modulation packets with a sequence number, showing RSSI
 * and SNR for received packets on the little display.
 *
 * Note that while this send and received using LoRa modulation, it does not do
 * LoRaWAN. For that, see the LoRaWAN_TTN example.
 *
 * This works on the stick, but the output on the screen gets cut off.
*/

// Turns the 'PRG' button into the power button, long press is off 
#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

#include "PayloadDecoder.h"

// Pause between transmited packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#define PAUSE               300

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
#define FREQUENCY           866.3       // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH           125.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference. 
#define SPREADING_FACTOR    9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
#define TRANSMIT_POWER      0

// Ble advertising library
#include "ArduinoBLE.h"

BLEService myService("fff0");
BLEIntCharacteristic myCharacteristic("fff1", BLERead | BLEBroadcast);

// Advertising parameters should have a global scope. Do NOT define them in 'setup' or in 'loop'
const uint8_t completeRawAdvertisingData[] = {13, 0xff, 0xA1,0xA2,0xA3,0xA4,0xB1,0xB2,0xB3,0x0B4,0xC1,0xC2,0xC3,0xC4};

#define BLE_AD_LENGHT_IDX 0
#define BLE_AD_TYPE_IDX 1
#define BLE_AD_HEADER_SIZE 2

#define BLE_AD_MANUFACTURER_DATA 0x0ff
uint8_t position_advertising_data[BLE_AD_HEADER_SIZE+3*4];

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

void setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  if (!BLE.begin()) {
    Serial.println("failed to initialize BLE!");
    while (1);
  }

  myService.addCharacteristic(myCharacteristic);
  BLE.addService(myService);

  // Build advertising data packet
  BLEAdvertisingData advData;
  // If a packet has a raw data parameter, then all the other parameters of the packet will be ignored
  advData.setRawData(completeRawAdvertisingData, sizeof(completeRawAdvertisingData));
  // Copy set parameters in the actual advertising packet
  BLE.setAdvertisingData(advData);

  // Build scan response data packet
  BLEAdvertisingData scanData;
  scanData.setLocalName("Bird Track");
  // Copy set parameters in the actual scan response packet
  BLE.setScanResponseData(scanData);

  BLE.advertise();

  both.println("Starting main loop");
}

void loop() {
  heltec_loop();
  
  bool tx_legal = millis() > last_tx + minimum_pause;
  // Transmit a packet every PAUSE seconds or when the button is pressed
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    // In case of button click, tell user to wait
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }
    both.printf("TX [%s] ", String(counter).c_str());
    radio.clearDio1Action();
    heltec_led(50); // 50% brightness is plenty for this LED
    tx_time = millis();
    RADIOLIB(radio.transmit(String(counter++).c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }
    // Maximum 1% duty cycle
    minimum_pause = tx_time * 100;
    last_tx = millis();
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag) {
    uint8_t rx_data[30];
    size_t rx_length = 0;
    rxFlag = false;
    rx_length = 14;
    radio.readData(rx_data, rx_length);
    // rx_length = radio.receive(&rx_data);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      char payload_txt[30];
      for (int i=0; i<rx_length; i++) {
        sprintf(&payload_txt[2*i], "%02X", rx_data[i]);
      }
      both.printf("RX [%s] %u\n", payload_txt, rx_length);
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());

      DecodedPayload payload;
      bool result_ok = decode_payload(rx_data, rx_length, &payload);
      if (!result_ok) {
        both.println("Error: Failed to decode payload");
      }
      both.printf("lat:%f lon:%f alt:%f\n", payload.latitude, payload.longitude, payload.altitude);

      char adv_data[20];
      // Build advertising data packet
      BLE.stopAdvertise();
      BLEAdvertisingData advData;
      // If a packet has a raw data parameter, then all the other parameters of the packet will be ignored
      position_advertising_data[BLE_AD_LENGHT_IDX] = sizeof(position_advertising_data) - 1; // lenght byte not included in size
      position_advertising_data[BLE_AD_TYPE_IDX] = BLE_AD_MANUFACTURER_DATA;
      size_t data_offset = BLE_AD_HEADER_SIZE;
      memcpy(position_advertising_data + data_offset, &payload.latitude, sizeof(payload.latitude));
      data_offset+= sizeof(payload.latitude);
      memcpy(position_advertising_data + data_offset, &payload.longitude, sizeof(payload.longitude));
      data_offset+= sizeof(payload.longitude);
      memcpy(position_advertising_data + data_offset, &payload.altitude, sizeof(payload.altitude));
      advData.setRawData(position_advertising_data, sizeof(position_advertising_data));
      // Copy set parameters in the actual advertising packet
      BLE.setAdvertisingData(advData);
      BLE.advertise();

      for (int i=0; i<sizeof(position_advertising_data); i++) {
        sprintf(&payload_txt[2*i], "%02X", position_advertising_data[i]);
      }
      both.printf("Advertising [%s] %u\n", payload_txt, sizeof(position_advertising_data));
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx() {
  rxFlag = true;
}
