#include "config.h"
#include "wifi_manager.h"
#include "uart_handler.h"
#include "firebase_client.h"
#include "telegram_bot.h"
#include "openai_client.h"
#include "lcd_display.h"

// Thresholds (can be updated via Telegram)
float tempThreshold = DEFAULT_TEMP_THRESHOLD;
float distThreshold = DEFAULT_DIST_THRESHOLD;

// Track warning level changes to trigger OpenAI only when needed
int lastWarningLevel = 0;

// Timing
unsigned long lastSensorRead   = 0;
unsigned long lastTelegramPoll = 0;

const unsigned long SENSOR_INTERVAL   = 2000;  // Read UART every 2s
const unsigned long TELEGRAM_INTERVAL = 3000;  // Poll Telegram every 3s

// Latest sensor data (kept for Telegram /status responses)
SensorData_t latestData = {0, 0, 0, 0, 0, 0, false};

void setup() {
    Serial.begin(115200);
    Serial.println("=== Smart Study Monitor (ESP32) ===");

    initLCD();
    connectWiFi();
    initUART();
    initFirebase();
    initTelegram();
    initOpenAI();

    displayText("Monitor Ready", "Waiting data...");
}

void loop() {
    ensureWiFiConnected();

    unsigned long now = millis();

    // --- 1. Read sensor data from MCXC444 via UART ---
    if (now - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = now;

        SensorData_t data = readUART();
        if (data.valid) {
            latestData = data;

            // Show quick reading on LCD
            String line1 = "T:" + String(data.temperature, 0) + "C H:" + String(data.humidity, 0) + "% L:" + String(data.light);
            String line2 = "D:" + String(data.distance, 0) + "cm " + (data.noise ? "LOUD" : "Quiet");
            displayText(line1, line2);

            // --- 2. Log to Firebase ---
            logToFirebase(data);

            // --- 3. Check if warning state changed -> trigger advisory ---
            bool warningChanged = (data.warningLevel != lastWarningLevel);
            lastWarningLevel = data.warningLevel;

            if (warningChanged && data.warningLevel >= 1) {
                // Get AI advisory
                String advisory = getAdvisory(data);
                Serial.print("Advisory: ");
                Serial.println(advisory);

                // Display on LCD
                scrollText(advisory);

                // Send Telegram alert for critical
                if (data.warningLevel == 2) {
                    String alertMsg = "CRITICAL ALERT!\n" + advisory;
                    sendTelegramAlert(alertMsg.c_str());
                }

                // Send advisory back to MCXC444
                sendUART(advisory.c_str());
            }
        }
    }

    // --- 4. Poll Telegram for user commands ---
    if (now - lastTelegramPoll >= TELEGRAM_INTERVAL) {
        lastTelegramPoll = now;
        pollTelegram(latestData, tempThreshold, distThreshold);

        // If a command was received, trigger advisory
        if (hasPendingCommand() && latestData.valid) {
            String cmd = getLastCommand();

            if (cmd == "/status") {
                String advisory = getAdvisory(latestData);
                scrollText(advisory);
                sendTelegramAlert(advisory.c_str());
            }
        }
    }
}
