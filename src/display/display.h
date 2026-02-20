// ============================================
// TFT Display Driver - ResearchMate
// ST7789 172x320 Display Test
// ============================================

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Initialize the TFT display
bool initDisplay();

// Clear the entire display
void clearDisplay();

// Show a status message on display
void displayStatus(const char* message);

// Show WiFi connection info
void displayWiFiInfo(const char* ip, const char* ssid);

// Show camera status
void displayCameraStatus(bool working);

// Show an error message
void displayError(const char* error);

// Show success/ready state
void displaySuccess();

// Show pairing code prominently
void displayPairingCode(const char* code);

// Show scanning animation
void displayScanning();

// Show upload progress
void displayUploading(int progress);

// Show upload complete
void displayUploadComplete();

// Simple test pattern
void displayTestPattern();

// Color demo for continuous cycling
void displayColorDemo(uint16_t color, const char* colorName);

// Show SD card status
void displaySDCardStatus(bool available, const char* info);

// Show camera debug info (item count, status, time)
void displayCameraDebug(int itemCount, const char* lastStatus, unsigned long lastCaptureTime);

// Flash screen when capturing (visual feedback)
void displayCaptureFlash();

#endif // DISPLAY_H