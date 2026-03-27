#include "telegram_bot.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static long   lastUpdateId = 0;
static bool   pendingCommand = false;
static String lastCommand = "";

static const String BOT_URL = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN);

void initTelegram() {
    Serial.println("Telegram bot ready.");
}

// Send a message to the configured chat
static bool sendMessage(const String& text) {
    HTTPClient http;
    String url = BOT_URL + "/sendMessage";

    String payload = "{";
    payload += "\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + "\",";
    payload += "\"text\":\"" + text + "\",";
    payload += "\"parse_mode\":\"Markdown\"";
    payload += "}";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    http.end();

    return code == 200;
}

// Format sensor data as a readable status string
static String formatStatus(const SensorData_t& data) {
    String status = "--- Study Desk Status ---\n";
    status += "Temp: "     + String(data.temperature, 1) + " C\n";
    status += "Humidity: " + String(data.humidity, 1) + " %\n";
    status += "Light: "    + String(data.light) + "\n";
    status += "Noise: "    + (data.noise ? String("Noisy") : String("Quiet")) + "\n";
    status += "Distance: " + String(data.distance, 1) + " cm\n";

    const char* levels[] = {"Normal", "Warning", "Critical"};
    int idx = constrain(data.warningLevel, 0, 2);
    status += "Status: " + String(levels[idx]);

    return status;
}

// Poll Telegram for new messages and handle commands
void pollTelegram(const SensorData_t& data, float& tempThreshold, float& distThreshold) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = BOT_URL + "/getUpdates?offset=" + String(lastUpdateId + 1) + "&timeout=1&limit=5";

    http.begin(url);
    int code = http.GET();

    if (code != 200) {
        http.end();
        return;
    }

    String response = http.getString();
    http.end();

    // Parse JSON response
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) return;

    JsonArray results = doc["result"].as<JsonArray>();

    for (JsonObject result : results) {
        long updateId = result["update_id"];
        if (updateId > lastUpdateId) lastUpdateId = updateId;

        String text = result["message"]["text"] | "";
        text.trim();

        if (text.length() == 0) continue;

        Serial.print("Telegram cmd: ");
        Serial.println(text);

        pendingCommand = true;
        lastCommand = text;

        if (text == "/status") {
            sendMessage(formatStatus(data));
        }
        else if (text.startsWith("/settemp ")) {
            float val = text.substring(9).toFloat();
            if (val > 0) {
                tempThreshold = val;
                sendMessage("Temp threshold set to " + String(val, 1) + " C");
            } else {
                sendMessage("Usage: /settemp <value>");
            }
        }
        else if (text.startsWith("/setdist ")) {
            float val = text.substring(9).toFloat();
            if (val > 0) {
                distThreshold = val;
                sendMessage("Distance threshold set to " + String(val, 1) + " cm");
            } else {
                sendMessage("Usage: /setdist <value>");
            }
        }
        else if (text == "/start") {
            sendMessage("Smart Study Monitor active. Commands:\n/status - View readings\n/settemp <C> - Set temp threshold\n/setdist <cm> - Set distance threshold");
        }
    }
}

void sendTelegramAlert(const char* message) {
    sendMessage(String(message));
}

bool hasPendingCommand() {
    bool val = pendingCommand;
    pendingCommand = false;
    return val;
}

String getLastCommand() {
    return lastCommand;
}
