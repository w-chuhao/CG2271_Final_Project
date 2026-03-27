#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include "uart_handler.h"

void initTelegram();
void pollTelegram(const SensorData_t& data, float& tempThreshold, float& distThreshold);
void sendTelegramAlert(const char* message);
bool hasPendingCommand();
String getLastCommand();

#endif
