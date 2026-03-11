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
void clearScreen();
void drawHeader();

// Set the global Wi-Fi status for the Top Bar
void setWiFiStatus(bool connected);

// Set the global Pairing status for the Top Bar
void setPairingStatus(bool paired);

// Set the main operation mode (e.g. "SETUP", "READY", "CAPTURING")
void setUIMode(const char *mode);

// Set the bottom offline item queue count
void setQueueCount(int count);

// Set the bottom panel "last action" text
void setLastAction(const char *action, bool isError = false);

// Draw the Top Status Bar
void drawTopBar();

// Draw the Bottom Action Panel
void drawBottomPanel();

// ============================================
// Factory Reset Wipe UI
// ============================================
void displayWipeStart();
void displayWipeProgress(int pct);
void displayWipeCancelled();
void displayWipeComplete();
void displaySleep();
void displaySyncing();

// Show WiFi Setup QR Code centered in the viewfinder zone
void displayWiFiSetupQR(const char *ssid);

// Show pairing code prominently centered in the viewfinder zone
void displayPairingCode(const char *code);

// Draw a raw JPEG frame onto the screen, cropped to the 240x240 viewfinder
void displayDrawFrame(const uint8_t *jpg_data, size_t jpg_len);

// Flash screen when capturing (visual feedback) - restricted to viewfinder
void displayCaptureFlash();

// Restore the screen back to a ready/idle visual state
void displayReady();

#endif // DISPLAY_H