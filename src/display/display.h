// ============================================
// DISPLAY MODULE HEADER - ILI9341 2.4" TFT
// ResearchMate Smart Pen
// ============================================

#ifndef DISPLAY_H
#define DISPLAY_H

// Core display functions
bool initDisplay();
void clearDisplay();

// Status display
void displayStatus(const char* message);
void displayWiFiInfo(const char* ip, const char* ssid);
void displayCameraStatus(bool working);
void displayError(const char* error);
void displaySuccess();

// Smart Pen specific displays
void displayPairingCode(const char* code);
void displayScanning();
void displayUploading(int progress);
void displayUploadComplete();

#endif