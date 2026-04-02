#include <Arduino.h>
#include "uart_receive.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  uartReceiveInit();
  Serial.println("Listening for MCXC packets on Serial1...");
}

void loop() {
  uartReceiveLoop();
}
