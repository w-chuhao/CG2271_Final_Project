#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("Wi-Fi connection failed.");
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void ensureWiFiConnected() {
    if (!isWiFiConnected()) {
        Serial.println("Wi-Fi lost. Reconnecting...");
        connectWiFi();
    }
}
