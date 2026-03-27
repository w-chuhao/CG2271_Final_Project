#ifndef CONFIG_H
#define CONFIG_H

// ==================== Wi-Fi ====================
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ==================== Firebase RTDB ====================
#define FIREBASE_HOST "YOUR_PROJECT.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"

// ==================== Telegram Bot ====================
#define TELEGRAM_BOT_TOKEN "YOUR_BOT_TOKEN"
#define TELEGRAM_CHAT_ID   "YOUR_CHAT_ID"

// ==================== OpenAI API ====================
#define OPENAI_API_KEY "YOUR_OPENAI_API_KEY"

// ==================== UART ====================
#define UART_RX_PIN 16
#define UART_TX_PIN 17
#define UART_BAUD   9600

// ==================== LCD I2C ====================
#define LCD_I2C_ADDR 0x27
#define LCD_COLS     16
#define LCD_ROWS     2

// ==================== Thresholds (defaults) ====================
#define DEFAULT_TEMP_THRESHOLD  32.0f
#define DEFAULT_DIST_THRESHOLD  30.0f

#endif
