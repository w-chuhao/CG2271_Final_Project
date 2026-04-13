#include "time_util.h"
#include "wifi_manager.h"
#include <time.h>

// Singapore Standard Time — UTC+8, no DST
static const long  GMT_OFFSET_SEC      = 8 * 3600;
static const int   DAYLIGHT_OFFSET_SEC = 0;
// Use Google first — its NTP often punches through carrier hotspots that
// block pool.ntp.org. Fall back to two pool servers.
static const char *NTP_PRIMARY         = "time.google.com";
static const char *NTP_SECONDARY       = "pool.ntp.org";
static const char *NTP_TERTIARY        = "time.cloudflare.com";

static bool      ntpConfigured  = false;
static bool      ntpEverSynced  = false;
static uint32_t  lastRetryMs    = 0;
static const uint32_t NTP_RETRY_INTERVAL_MS = 30000;  // retry every 30 s

static void configureNtp() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
             NTP_PRIMARY, NTP_SECONDARY, NTP_TERTIARY);
  ntpConfigured = true;
}

void timeInit() {
  if (!wifiIsConnected()) return;
  Serial.println("[Time] Starting NTP (non-blocking)...");
  configureNtp();
  lastRetryMs = millis();
  // Don't wait — SNTP polls in the background. timeMaintain() handles retries.
}

void timeMaintain() {
  if (!wifiIsConnected()) return;

  struct tm t;
  if (getLocalTime(&t, 0)) {
    if (!ntpEverSynced) {
      ntpEverSynced = true;
      Serial.printf("[Time] NTP synced: %04d-%02d-%02d %02d:%02d:%02d SGT\n",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);
    }
    return;
  }

  // Not synced yet — re-issue configTime() periodically in case the initial
  // attempt happened before DNS was ready or UDP/123 was momentarily blocked.
  const uint32_t now = millis();
  if (now - lastRetryMs >= NTP_RETRY_INTERVAL_MS) {
    lastRetryMs = now;
    Serial.println("[Time] NTP still unsynced — retrying configTime().");
    configureNtp();
  }
}

bool timeIsSynced() {
  return ntpEverSynced;
}

String currentIsoString() {
  struct tm t;
  if (ntpConfigured && getLocalTime(&t, 0)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S SGT", &t);
    return String(buf);
  }
  return "uptime " + String(millis() / 1000.0f, 1) + "s";
}

uint32_t uptimeSeconds() {
  return millis() / 1000UL;
}
