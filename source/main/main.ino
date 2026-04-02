#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "sensors.h"
#include "uart_receive.h"

DeskState myDesk = {0.0f, 0.0f, 0.0f, 0, 0, false, true, OFF, 0};

void setup() {
  Serial.begin(115200);

  uartReceiveInit();
  sensorsInit();
  displayInit();

  Serial.println("System fully initialized...");
  delay(1500);
}

void loop() {
  uartReceiveLoop(myDesk);

  if (!myDesk.systemActive) {
    myDesk.noise = false;
    myDesk.rgbState = OFF;
    digitalWrite(BUZZER_PIN, LOW);

    displayInactive();
    Serial.print("SYSTEM=INACTIVE");
    Serial.print(" | DIST_CM=");
    if (myDesk.distance < 0) Serial.print("ERR");
    else Serial.print(myDesk.distance, 2);
    Serial.print(" | HUMIDITY=");
    if (isnan(myDesk.humidity)) Serial.print("ERR");
    else { Serial.print(myDesk.humidity, 1); Serial.print("%"); }
    Serial.print(" | TEMP_C=");
    if (isnan(myDesk.temp)) Serial.print("ERR");
    else { Serial.print(myDesk.temp, 1); Serial.print("C"); }
    Serial.print(" | MCXC_LIGHT=");
    Serial.print(myDesk.light);
    Serial.print(" | MCXC_MIC_P2P=");
    Serial.print(myDesk.mcxcMicP2P);
    Serial.print(" | MCXC_ALERT=");
    Serial.println(myDesk.noise ? 1 : 0);
    delay(200);
    return;
  }

  sensorsRead(myDesk);

  const uint8_t newState = evaluateState(myDesk);
  uint8_t finalState = newState;

  if (newState == CRITICAL && millis() - myDesk.lastAckTime < ACK_QUIET_MS) {
    finalState = WARNING;
  }

  if (finalState != myDesk.rgbState) {
    myDesk.rgbState = finalState;

    if (finalState == CRITICAL) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }

  displayUpdate(myDesk, finalState);

  Serial.print("SYSTEM=");
  Serial.print(myDesk.systemActive ? "ACTIVE" : "INACTIVE");
  Serial.print(" | DIST_CM=");
  if (myDesk.distance < 0) Serial.print("ERR");
  else Serial.print(myDesk.distance, 2);
  Serial.print(" | HUMIDITY=");
  if (isnan(myDesk.humidity)) Serial.print("ERR");
  else { Serial.print(myDesk.humidity, 1); Serial.print("%"); }
  Serial.print(" | TEMP_C=");
  if (isnan(myDesk.temp)) Serial.print("ERR");
  else { Serial.print(myDesk.temp, 1); Serial.print("C"); }
  Serial.print(" | MCXC_LIGHT=");
  Serial.print(myDesk.light);
  Serial.print(" | MCXC_MIC_P2P=");
  Serial.print(myDesk.mcxcMicP2P);
  Serial.print(" | MCXC_ALERT=");
  Serial.println(myDesk.noise ? 1 : 0);

  delay(200);
}
