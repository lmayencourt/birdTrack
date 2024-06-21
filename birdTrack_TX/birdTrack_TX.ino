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
#include "ArduinoLog.h"

#include "OnBoardDisplay.h"
#include "OnBoardGPS.h"
#include "PayloadEncoder.h"

// When true, GPS is deactivate between every loop
// in order to improve battery life.
#define GPS_LOW_POWER 0

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

char tx_packet[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

float txNumber;
bool lora_tx_done=true;

// Legal 1% duty cycle
uint32_t last_tx = 0;
uint32_t tx_start_time = 0;
uint32_t tx_end_time = 0;
uint32_t minimum_pause = 999999;

enum BirdTrackingDeviceState {
  BTD_STATE_GPS_START,
  BTD_STATE_GPS_SEARCHING,
  BTD_STATE_GPS_TIME,
  BTD_STATE_GPS_LOCKED,
  BTD_STATE_LORA_SEND,
  BTD_STATE_LORA_SEND_WAIT_DONE,
  BTD_STATE_SLEEP,
};

BirdTrackingDeviceState device_state;

#define SCREEN_ROLLOUT_DELAY 3500
static TimerEvent_t screen_rollout_timer;

void logPrefix(Print* _logOutput, int logLevel) {
  // Division constants
  const unsigned long MSECS_PER_SEC       = 1000;
  const unsigned long SECS_PER_MIN        = 60;
  const unsigned long SECS_PER_HOUR       = 3600;
  const unsigned long SECS_PER_DAY        = 86400;

  // Total time
  const unsigned long msecs               =  millis();
  const unsigned long secs                =  msecs / MSECS_PER_SEC;

  // Time in components
  const unsigned long MilliSeconds        =  msecs % MSECS_PER_SEC;
  const unsigned long Seconds             =  secs  % SECS_PER_MIN ;
  const unsigned long Minutes             = (secs  / SECS_PER_MIN) % SECS_PER_MIN;
  const unsigned long Hours               = (secs  % SECS_PER_DAY) / SECS_PER_HOUR;

  // Time as string
  char timestamp[20];
  sprintf(timestamp, "%02d:%02d:%02d.%03d ", Hours, Minutes, Seconds, MilliSeconds);
  _logOutput->print(timestamp);
}

void onScreenRolloutTimeout()
{
  Log.infoln("Screen rollout");
  displayNextScreen();
  TimerStart(&screen_rollout_timer);
}

void setup()
{
  Serial.begin(115200);
  Log.setPrefix(logPrefix);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.noticeln("Bird Tracking device");
  Log.noticeln("Starting...");

  display_on();
  display.init();

  gps_config();

  txNumber=0;
  lora_tx_done = true;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

  // For testing, start in Lora directly
  device_state = BTD_STATE_GPS_START;

  // Setup timer for screen rolling
  TimerInit(&screen_rollout_timer, onScreenRolloutTimeout);
  TimerSetValue(&screen_rollout_timer, SCREEN_ROLLOUT_DELAY);
  TimerStart(&screen_rollout_timer);

  Log.noticeln("Setup done, enter main loop");
}

void loop(){
  char state_str[30];
  char str[30];
  int index = 0;

  switch(device_state) {
    case BTD_STATE_GPS_START:
      index = sprintf(state_str, "GPS Start...");
      state_str[index] = 0;
      if (GPS_LOW_POWER) {
        gpsStart();
      }
      device_state = BTD_STATE_GPS_SEARCHING;
      break;
    case BTD_STATE_GPS_SEARCHING:
      Log.noticeln("GPS Searching...");
      GpsDateTime datetime;
      gps_get_date_time(&datetime);
      index = sprintf(state_str, "GPS searching...");
      state_str[index] = 0;

      if (gps_is_time_valid()) {
        index = sprintf(str,"%02d-%02d-%02d",datetime.year,datetime.day,datetime.month);
        str[index] = 0;
        Log.notice(str);

        index = sprintf(str,"-%02d:%02d:%02d",datetime.hour,datetime.minute,datetime.second);
        str[index] = 0;
        Log.noticeln(str);
      } else {
        Log.noticeln("No GPS time yet...");
      }

      if (gps_is_position_valid()) {
        device_state = BTD_STATE_GPS_LOCKED;
      } else {
        gps_update(1000);
      }

      break;
    case BTD_STATE_GPS_LOCKED:
      Log.noticeln("GPS Locked...");
      index = sprintf(state_str, "GPS Ok");
      state_str[index] = 0;

      if (GPS_LOW_POWER) {
        gps_idle();
      }
      device_state = BTD_STATE_LORA_SEND;
    break;
    case BTD_STATE_LORA_SEND: {
        Log.noticeln("LoRa Sending...");
        display.clear();
        index = sprintf(state_str, "LoRa Tx...");
        state_str[index] = 0;

        // Build payload
        GpsInfo gps_info;
        gps_get_info(&gps_info);
        // sprintf(tx_packet, "lat:%f lon:%f alt:%f", gps_info.latitude, gps_info.longitude, gps_info.altitude);
        DecodedPayload payload;
        payload.command = CMD_FULL_POSITION_UPDATE;
        payload.latitude = gps_info.latitude;
        payload.longitude = gps_info.longitude;
        payload.altitude = gps_info.altitude;
        // payload.latitude = 10.0;
        // payload.longitude = 20.0;
        // payload.altitude = 30.0;
        size_t payload_length = encode_payload(payload, (uint8_t*)&tx_packet, sizeof(tx_packet));
        if (payload_length != 14) {
          Log.errorln("Failed to build payload %d", payload_length);
        }

        // Start sending
        char payload_txt[30];
        for (int i=0; i<payload_length; i++) {
          sprintf(&payload_txt[2*i], "%02X", tx_packet[i]);
        }
        uint32_t time_on_air = Radio.TimeOnAir(MODEM_LORA, payload_length);
        Log.traceln("Sending packet \"%s\" , length %d, time: %d (ms)", payload_txt, payload_length, time_on_air);
        // turnOnRGB(COLOR_SEND,0); //change rgb color
        tx_start_time = millis();
        lora_tx_done = false;
        Radio.Send( (uint8_t *)tx_packet, payload_length); //send the package out 
        device_state = BTD_STATE_LORA_SEND_WAIT_DONE;
      }
      break;
    case BTD_STATE_LORA_SEND_WAIT_DONE:
      if (lora_tx_done) {
        tx_end_time = millis();
        Log.traceln("Tx time %u -> %u delta %u (ms)", tx_start_time, tx_end_time, (tx_end_time - tx_start_time));
        minimum_pause = (tx_end_time - tx_start_time) * 100;
        device_state = BTD_STATE_SLEEP;
      }
      break;
    case BTD_STATE_SLEEP:
      Log.noticeln("Sleeping...");
      index = sprintf(state_str, "Sleeping...");
      state_str[index] = 0;

      Log.traceln("Legal limit, wait %u sec.", (int)((minimum_pause - (millis() - tx_end_time)) / 1000) + 1);
      Log.noticeln("Enter deep sleep for %u...", minimum_pause);
      delay(minimum_pause);
      device_state = BTD_STATE_GPS_SEARCHING;
      break;
    default:
      Log.errorln("Unhandled state &i", device_state);
      break;
  }

  displayInfo(state_str);
}

void OnTxDone( void )
{
  turnOffRGB();
  lora_tx_done = true;
}

void OnTxTimeout( void )
{
  turnOffRGB();
  Radio.Sleep( );
  Log.noticeln("TX Timeout......");
  lora_tx_done = true;
}
