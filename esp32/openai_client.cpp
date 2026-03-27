#include "openai_client.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void initOpenAI() {
    Serial.println("OpenAI client ready.");
}

// Build a prompt from sensor data and call the Chat Completions API.
// Returns a one-sentence study advisory string.
String getAdvisory(const SensorData_t& data) {
    if (WiFi.status() != WL_CONNECTED) return "Wi-Fi not connected.";

    HTTPClient http;
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
    http.setTimeout(15000);

    // Build user message with current sensor readings
    String userMsg = "Current study desk conditions: ";
    userMsg += "Temperature=" + String(data.temperature, 1) + "C, ";
    userMsg += "Humidity=" + String(data.humidity, 1) + "%, ";
    userMsg += "Light=" + String(data.light) + " (higher=brighter), ";
    userMsg += "Noise=" + (data.noise ? String("noisy") : String("quiet")) + ", ";
    userMsg += "Sitting distance=" + String(data.distance, 1) + "cm. ";

    const char* levels[] = {"Normal", "Warning", "Critical"};
    int idx = constrain(data.warningLevel, 0, 2);
    userMsg += "Overall status: " + String(levels[idx]) + ".";

    // Build JSON request body
    JsonDocument doc;
    doc["model"] = "gpt-4o-mini";
    doc["max_tokens"] = 80;
    doc["temperature"] = 0.7;

    JsonArray messages = doc["messages"].to<JsonArray>();

    JsonObject sysMsg = messages.add<JsonObject>();
    sysMsg["role"] = "system";
    sysMsg["content"] = "You are a study environment advisor. Given the sensor readings from a student's desk, respond with exactly ONE short sentence of actionable advice to improve their study conditions. Be specific and practical.";

    JsonObject usrMsg = messages.add<JsonObject>();
    usrMsg["role"] = "user";
    usrMsg["content"] = userMsg;

    String requestBody;
    serializeJson(doc, requestBody);

    int code = http.POST(requestBody);

    String advisory = "Unable to get advisory.";

    if (code == 200) {
        String response = http.getString();

        JsonDocument resDoc;
        DeserializationError err = deserializeJson(resDoc, response);
        if (!err) {
            const char* content = resDoc["choices"][0]["message"]["content"];
            if (content) {
                advisory = String(content);
            }
        }
    } else {
        Serial.print("OpenAI error: ");
        Serial.println(code);
    }

    http.end();
    return advisory;
}
