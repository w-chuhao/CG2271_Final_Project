#include "firebase_client.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>

void initFirebase() {
    Serial.println("Firebase client ready.");
}

// Writes sensor data to Firebase RTDB via REST API
bool logToFirebase(const SensorData_t& data) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;

    // Build JSON payload
    String json = "{";
    json += "\"temperature\":" + String(data.temperature, 1) + ",";
    json += "\"humidity\":"    + String(data.humidity, 1) + ",";
    json += "\"light\":"       + String(data.light) + ",";
    json += "\"noise\":"       + String(data.noise) + ",";
    json += "\"distance\":"    + String(data.distance, 1) + ",";
    json += "\"warningLevel\":" + String(data.warningLevel) + ",";
    json += "\"timestamp\":{\".sv\":\"timestamp\"}";
    json += "}";

    // --- Update /latest ---
    String latestUrl = "https://" + String(FIREBASE_HOST) + "/latest.json?auth=" + String(FIREBASE_AUTH);
    http.begin(latestUrl);
    http.addHeader("Content-Type", "application/json");
    int putCode = http.PUT(json);

    if (putCode > 0) {
        Serial.print("Firebase /latest updated: ");
        Serial.println(putCode);
    } else {
        Serial.print("Firebase /latest error: ");
        Serial.println(http.errorToString(putCode));
    }
    http.end();

    // --- Append to /readings ---
    String readingsUrl = "https://" + String(FIREBASE_HOST) + "/readings.json?auth=" + String(FIREBASE_AUTH);
    http.begin(readingsUrl);
    http.addHeader("Content-Type", "application/json");
    int postCode = http.POST(json);

    if (postCode > 0) {
        Serial.print("Firebase /readings logged: ");
        Serial.println(postCode);
    } else {
        Serial.print("Firebase /readings error: ");
        Serial.println(http.errorToString(postCode));
    }
    http.end();

    return (putCode > 0 && postCode > 0);
}
