#include <Arduino.h>
#include "storage.h"

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

bool initSDCard() {
    LOG_DEBUG("SD card initialized");
    return true;
}

bool saveImage(const char* filename, uint8_t* data, size_t size) {
    LOG_DEBUG("Saved image: %s (%d bytes)", filename, size);
    return true;
}

bool loadImage(const char* filename, uint8_t* buffer, size_t* size) {
    LOG_DEBUG("Loaded image: %s", filename);
    return true;
}