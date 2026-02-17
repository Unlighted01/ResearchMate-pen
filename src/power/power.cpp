#include <Arduino.h>
#include "power.h"

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

bool initPower() {
    LOG_DEBUG("Power management initialized");
    return true;
}

void updateBatteryStatus() {
    LOG_DEBUG("Battery status updated");
}

uint8_t getBatteryPercent() {
    return 100;  // Default: fully charged
}