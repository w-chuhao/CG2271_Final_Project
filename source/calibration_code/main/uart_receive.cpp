#include <Arduino.h>
#include "uart_receive.h"

static void printPacket(uint8_t id, uint8_t high, uint8_t low) {
  switch (id) {
    case ID_LIGHT: {
      const uint16_t lightValue = (static_cast<uint16_t>(high) << 8) | low;
      Serial.print("LIGHT: ");
      Serial.println(lightValue);
      break;
    }

    case ID_NOISE:
      Serial.print("NOISE: ");
      Serial.println(low > 0 ? "DETECTED" : "CLEAR");
      break;

    case ID_RGB:
      Serial.print("RGB STATE: ");
      Serial.println(low);
      break;

    case ID_SYSTEM_CTRL:
      Serial.print("SYSTEM: ");
      Serial.println(low == 1 ? "STARTED" : "STOPPED");
      break;

    case ID_ALERT_ACK:
      Serial.println("ALERT ACK RECEIVED");
      break;

    default:
      Serial.print("UNKNOWN ID 0x");
      Serial.print(id, HEX);
      Serial.print(" DATA: 0x");
      Serial.print(high, HEX);
      Serial.print(" 0x");
      Serial.println(low, HEX);
      break;
  }
}

void uartReceiveInit() {
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial.println("UART receive monitor ready");
}

void uartReceiveLoop() {
  while (Serial1.available() >= 5) {
    if (Serial1.read() != SYNC_BYTE) {
      continue;
    }

    const uint8_t id = Serial1.read();
    const uint8_t high = Serial1.read();
    const uint8_t low = Serial1.read();
    const uint8_t checksum = Serial1.read();
    const uint8_t expectedChecksum = id ^ high ^ low;

    if (checksum != expectedChecksum) {
      Serial.print("CHECKSUM ERROR: expected 0x");
      Serial.print(expectedChecksum, HEX);
      Serial.print(", got 0x");
      Serial.println(checksum, HEX);
      continue;
    }

    printPacket(id, high, low);
  }
}
