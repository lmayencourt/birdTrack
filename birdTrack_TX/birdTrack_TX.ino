/* SPDX-License-Identifier: MIT
 *
 * This files uses part of code from:
 * https://github.com/HelTecAutomation/CubeCell-Arduino/tree/master/libraries/OnBoardGPS/examples/printGPSInfo
 *
 * Arduino board needed is CubeCell-GPS（HTCC-AB02S
 *
 * Copyright (c) 2024 Louis Mayencourt
 */

// LoRa radio
#include "LoRaWan_APP.h"
#include "Arduino.h"

#include "OnBoardDisplay.h"
#include "OnBoardGPS.h"
#include "PayloadEncoder.h"

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

// #define RF_FREQUENCY                               915000000 // Hz for US
#define RF_FREQUENCY                                866300000 // Hz for EU

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       9         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

float txNumber;
bool lora_tx_done=true;

// Legal 1% duty cycle
uint64_t last_tx = 0;
uint64_t tx_start_time;
uint64_t tx_end_time;
uint64_t minimum_pause;

enum BirdTrackingDeviceState {
  BTD_STATE_GPS_SEARCHING,
  BTD_STATE_GPS_TIME,
  BTD_STATE_GPS_LOCKED,
  BTD_STATE_LORA_SEND,
  BTD_STATE_SLEEP,
};

BirdTrackingDeviceState device_state;

void setup()
{
  Serial.begin(115200);
  Serial.println("Bird Tracking device");
  Serial.println("Starting...");

  display_on();
  display.init();
  display.setFont(ArialMT_Plain_10);

  // displayInfo();

  gps_config();

  txNumber=0;
  lora_tx_done = false;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

  // For testing, start in Lora directly
  // device_state = BTD_STATE_GPS_SEARCHING;
  device_state = BTD_STATE_LORA_SEND;

  Serial.println("Setup done, enter main loop");
}

void loop(){
  char str[30];
  int index = 0;

  switch(device_state) {
    case BTD_STATE_GPS_SEARCHING:
      Serial.println("GPS Searching...");
      GpsDateTime datetime;
      gps_get_date_time(&datetime);
      display.clear();  
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_16);
      display.drawString(64, 32-16/2, "GPS seearching...");

      if (gps_is_time_valid()) {
        display.setFont(ArialMT_Plain_10);
        index = sprintf(str,"%02d-%02d-%02d",datetime.year,datetime.day,datetime.month);
        str[index] = 0;
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 50, str);
        
        index = sprintf(str,"%02d:%02d:%02d",datetime.hour,datetime.minute,datetime.second);
        str[index] = 0;
        display.drawString(60, 50, str);
        Serial.println(str);
      } else {
        Serial.println("No GPS time yet...");
        display.drawString(60, 0, "No GPS time yet..");
      }

      display.display();

      //delay(1000);
      // displayInfo();

      if (gps_is_position_valid()) {
        device_state = BTD_STATE_GPS_LOCKED;
      } else {
        // display.setTextAlignment(TEXT_ALIGN_CENTER);
        // display.setFont(ArialMT_Plain_16);
        // display.drawString(64, 32-16/2, "No GPS signal");
        // Serial.println("No GPS signal");
        // display.display();
        gps_update(1000);
      }

      break;
    case BTD_STATE_GPS_LOCKED:
      Serial.println("GPS Locked...");
      display.clear();  
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_16);
      display.drawString(64, 32-16/2, "GPS Locked...");
      display.display();
      delay(1000);
      display.clear();  
      displayInfo();
      display.display();
      delay(1000);
      gps_idle();
      device_state = BTD_STATE_LORA_SEND;
    break;
    case BTD_STATE_LORA_SEND:
        if(lora_tx_done == false)
        {
          delay(1000);
          GpsInfo gps_info;
          gps_get_info(&gps_info);
          // sprintf(txpacket, "lat:%f lon:%f alt:%f", gps_info.latitude, gps_info.longitude, gps_info.altitude);
          DecodedPayload payload;
          payload.command = CMD_FULL_POSITION_UPDATE;
          payload.latitude = gps_info.latitude;
          payload.longitude = gps_info.longitude;
          payload.altitude = gps_info.altitude;
          size_t payload_length = encode_payload(payload, (uint8_t*)&txpacket, sizeof(txpacket));
          if (payload_length != 14) {
            Serial.println("Error when building payload");
          }
          Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, payload_length);
          // turnOnRGB(COLOR_SEND,0); //change rgb color
          tx_start_time = millis();
          Radio.Send( (uint8_t *)txpacket, payload_length); //send the package out 

          // Serial.println("tx time:");
          // Serial.println()
        } else {
          // delay(minimum_pause);
          device_state = BTD_STATE_SLEEP;
        }
        Serial.printf("Tx time %i -> %i  delta %i\n", tx_start_time, tx_end_time, (tx_end_time - tx_start_time));
        Serial.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - tx_end_time)) / 1000) + 1);
      break;
    case BTD_STATE_SLEEP:
      Serial.println("Enter deep sleep...");
      delay(5000);
      device_state = BTD_STATE_LORA_SEND;
      break;
    default:
      Serial.printf("Error: Unhandled state &i", device_state);
      break;
  }

}

void OnTxDone( void )
{
  tx_end_time = millis();
  turnOffRGB();
  Serial.printf("TX done %i......%i\n", tx_start_time, tx_end_time);
  lora_tx_done = true;
  minimum_pause = (tx_end_time - tx_start_time) * 100;
  last_tx = millis();
}

void OnTxTimeout( void )
{
  turnOffRGB();
  Radio.Sleep( );
  Serial.println("TX Timeout......");
  lora_tx_done = true;
}
