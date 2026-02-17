#include <Arduino.h>
#include "wifi.h"

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

bool initWiFi() {
    LOG_DEBUG("WiFi module initialized");
    return true;
}

bool isWiFiConnected() {
    return true;
}

void updateWiFi() {
    // Stub - placeholder
}