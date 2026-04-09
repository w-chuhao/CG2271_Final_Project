#include <Arduino.h>
#include "uart_receive.h"
#include "sensors.h"

DeskState espDesk = {0.0f, 0.0f, 0.0f, 0, false, true, OFF, 0};
uint32_t lastEspPrintMs = 0;

static void printEspSensors() {
  Serial.print("ESP DIST_CM=");
  if (espDesk.distance < 0) {
    Serial.print("ERR");
  } else {
    Serial.print(espDesk.distance, 2);
  }

  Serial.print(" | HUMIDITY=");
  if (isnan(espDesk.humidity)) {
    Serial.print("ERR");
  } else {
    Serial.print(espDesk.humidity, 1);
    Serial.print("%");
  }

  Serial.print(" | TEMP_C=");
  if (isnan(espDesk.temp)) {
    Serial.println("ERR");
  } else {
    Serial.print(espDesk.temp, 1);
    Serial.println("C");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  uartReceiveInit();
  sensorsInit();

  Serial.println("Listening for MCXC packets on Serial1...");
}

void loop() {
  uartReceiveLoop();

  if (millis() - lastEspPrintMs >= 1000UL) {
    lastEspPrintMs = millis();
    sensorsRead(espDesk);
    printEspSensors();
  }
}
