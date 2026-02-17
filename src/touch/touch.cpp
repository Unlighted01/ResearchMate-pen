#include <Arduino.h>
#include "touch.h"

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

bool initTouch() {
    LOG_DEBUG("Touch module initialized");
    return true;
}

void updateTouch() {
    // Stub - placeholder
}

bool isTouched() {
    return false;
}