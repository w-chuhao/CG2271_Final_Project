#ifndef FIREBASE_CLIENT_H
#define FIREBASE_CLIENT_H

#include "uart_handler.h"

void initFirebase();
bool logToFirebase(const SensorData_t& data);

#endif
