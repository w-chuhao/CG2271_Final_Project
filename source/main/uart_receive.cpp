#include <Arduino.h>
#include <stdio.h>

#include "uart_receive.h"

namespace {

constexpr size_t kFrameBufferSize = 64;
char g_frameBuffer[kFrameBufferSize];
size_t g_frameIndex = 0;

void printParsedFrame(unsigned int light, unsigned int micP2P, unsigned int started, unsigned int alert) {
  Serial.print("MCXC LIGHT=");
  Serial.print(light);
  Serial.print(" | MCXC MIC_P2P=");
  Serial.print(micP2P);
  Serial.print(" | STARTED=");
  Serial.print(started);
  Serial.print(" | ALERT=");
  Serial.println(alert);
}

void handleFrame(const char *frame) {
  unsigned int light = 0;
  unsigned int micP2P = 0;
  unsigned int started = 0;
  unsigned int alert = 0;

  const int parsed = sscanf(frame, "$MCXC,%u,%u,%u,%u", &light, &micP2P, &started, &alert);
  if (parsed == 4) {
    printParsedFrame(light, micP2P, started, alert);
    return;
  }

  Serial.print("INVALID FRAME: ");
  Serial.println(frame);
}

}  // namespace

void uartReceiveInit() {
  Serial1.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial.println("UART receive monitor ready");
}

void uartReceiveLoop() {
  while (Serial1.available() > 0) {
    const char incoming = static_cast<char>(Serial1.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      if (g_frameIndex > 0) {
        g_frameBuffer[g_frameIndex] = '\0';
        handleFrame(g_frameBuffer);
        g_frameIndex = 0;
      }
      continue;
    }

    if (g_frameIndex < (kFrameBufferSize - 1U)) {
      g_frameBuffer[g_frameIndex++] = incoming;
    } else {
      g_frameBuffer[g_frameIndex] = '\0';
      Serial.print("FRAME TOO LONG: ");
      Serial.println(g_frameBuffer);
      g_frameIndex = 0;
    }
  }
}
