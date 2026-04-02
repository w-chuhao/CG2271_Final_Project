#pragma once
#include <Arduino.h>
#include <stdint.h>

// Pin Definitions
#define BUZZER_PIN  9
#define DHTPIN      4
#define DHTTYPE     DHT11
#define TRIG_PIN    5
#define ECHO_PIN    18
#define RX1_PIN     1   // Connect to MCXC TX
#define TX1_PIN     2   // Connect to MCXC RX

// Serial Protocol
#define SYNC_BYTE      0xAA
#define ID_LIGHT       0x01
#define ID_NOISE       0x02
#define ID_RGB         0x03
#define ID_SYSTEM_CTRL 0x04
#define ID_ALERT_ACK   0x05

// RGB / System States
#define OFF      0x00
#define NORMAL   0x01
#define WARNING  0x02
#define CRITICAL 0x03

// Sensor Thresholds
#define TEMP_LOW   25.0f
#define TEMP_HI    30.0f
#define HUMID_LOW  80.0f
#define HUMID_HI   90.0f
#define DIST_LOW   15.0f
#define DIST_HI    30.0f

#define LIGHT_LOW  1365
#define LIGHT_HI   2730

// Timing
#define ACK_QUIET_MS  10000UL
#define LOOP_DELAY_MS 500

struct DeskState {
  float    temp;
  float    humidity;
  float    distance;
  uint16_t light;
  uint16_t mcxcMicP2P;
  bool     noise;
  bool     systemActive;
  uint8_t  rgbState;
  uint32_t lastAckTime;
};
