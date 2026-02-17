#include <Arduino.h>
#include "display.h"

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

// Display is disabled - causes boot loops with available libraries
// TODO: Need to research compatible display library or use I2C OLED

bool initDisplay() {
    LOG_DEBUG("Display module disabled (stub)");
    return true;
}

void clearDisplay() {
    // Stub
}

void displayStatus(const char* message) {
    LOG_DEBUG("Display: %s", message);
}

void displayWiFiInfo(const char* ip, const char* ssid) {
    LOG_DEBUG("Display WiFi: %s @ %s", ssid, ip);
}

void displayCameraStatus(bool working) {
    LOG_DEBUG("Display camera: %s", working ? "OK" : "ERROR");
}

void displayError(const char* error) {
    LOG_DEBUG("Display Error: %s", error);
}

void displaySuccess() {
    LOG_DEBUG("Display: Ready!");
}

void displayPairingCode(const char* code) {
    LOG_DEBUG("Display pairing code: %s", code);
}

void displayScanning() {
    LOG_DEBUG("Display: Scanning...");
}

void displayUploading(int progress) {
    LOG_DEBUG("Display: Uploading %d%%", progress);
}

void displayUploadComplete() {
    LOG_DEBUG("Display: Upload complete!");
}