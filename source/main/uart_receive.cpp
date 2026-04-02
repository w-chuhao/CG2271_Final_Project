#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "uart_receive.h"

namespace {

constexpr size_t kFrameBufferSize = 64;
char g_frameBuffer[kFrameBufferSize];
size_t g_frameIndex = 0;

void resetFrame() {
  g_frameIndex = 0;
}

void handleFrame(const char *frame, DeskState &state) {
  unsigned int light = 0;
  unsigned int micP2P = 0;
  unsigned int started = 0;
  unsigned int alert = 0;

  const int parsed = sscanf(frame, "$MCXC,%u,%u,%u,%u", &light, &micP2P, &started, &alert);
  if (parsed == 4) {
    state.light = static_cast<uint16_t>(light);
    state.mcxcMicP2P = static_cast<uint16_t>(micP2P);
    state.systemActive = (started != 0U);
    state.noise = (alert != 0U);
  }
}

}  // namespace

void uartReceiveInit() {
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  resetFrame();
}

void uartReceiveLoop(DeskState &state) {
  while (Serial1.available() > 0) {
    const char incoming = static_cast<char>(Serial1.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '$') {
      resetFrame();
      g_frameBuffer[g_frameIndex++] = incoming;
      continue;
    }

    if (g_frameIndex == 0) {
      continue;
    }

    if (incoming == '\n') {
      g_frameBuffer[g_frameIndex] = '\0';
      if (strncmp(g_frameBuffer, "$MCXC,", 6) == 0) {
        handleFrame(g_frameBuffer, state);
      }
      resetFrame();
      continue;
    }

    if (g_frameIndex < (kFrameBufferSize - 1U)) {
      g_frameBuffer[g_frameIndex++] = incoming;
    } else {
      resetFrame();
    }
  }
}
