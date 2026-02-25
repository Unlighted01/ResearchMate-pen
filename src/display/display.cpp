// ============================================
// TFT Display Driver - ResearchMate Smart Pen
// Beautiful UI matching web interface
// ============================================

#include "../config.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// Initialize display with hardware SPI
TFT_eSPI tft = TFT_eSPI();

static bool displayInitialized = false;

// ============================================
// ResearchMate Color Palette (RGB565)
// Matching web interface: #1a1a2e, #16213e, #00d4ff, etc.
// ============================================
#define BG_DARK 0x0A0F   // #1a1a2e - main background
#define BG_PANEL 0x0926  // #16213e - panel/card background
#define BG_STATUS 0x0823 // #0f3460 - status bar background
#define CYAN 0x06BF      // #00d4ff - accent cyan
#define GREEN 0x07E8     // #00ff41 - success green
#define GOLD 0xFEC0      // #ffd700 - warning gold
#define ORANGE 0xFD20    // #ff8c00 - orange status
#define RED 0xFB6D       // #ff6b6b - error red
#define PURPLE 0x9A7B    // #9d4edd - upload purple
#define WHITE 0xFFFF     // white text
#define GRAY 0x7BEF      // gray text

#define W 240
#define H 320

#include "display.h"

// ============================================
// Helper Functions
// ============================================

void clearScreen() { tft.fillScreen(BG_DARK); }

void drawPanel(int y, int h) {
  tft.fillRoundRect(10, y, W - 20, h, 8, BG_PANEL);
}

void drawHeader() {
  // Logo bar
  tft.fillRect(0, 0, W, 50, BG_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);

  // Title
  tft.setTextColor(CYAN);
  tft.drawString("ResearchMate", W / 2, 18);
  tft.setTextColor(GRAY);
  tft.drawString("Smart Pen", W / 2, 36);

  // Bottom accent line
  tft.fillRect(60, 48, W - 120, 2, CYAN);
}

void drawStatusBar(const char *text, uint16_t color) {
  tft.fillRoundRect(10, 60, W - 20, 35, 5, BG_STATUS);
  tft.fillRect(10, 60, 4, 35, color); // Left accent
  tft.setTextColor(color);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, W / 2, 77);
}

void drawButton(int y, const char *text, uint16_t color) {
  tft.fillRoundRect(20, y, W - 40, 40, 8, color);
  tft.setTextColor(color == GOLD ? 0x0000 : WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, W / 2, y + 20);
}

void drawProgressBar(int y, int progress, uint16_t color) {
  int barW = W - 60;
  int barH = 20;
  int x = 30;

  // Background
  tft.fillRoundRect(x, y, barW, barH, 5, BG_STATUS);

  // Fill
  int fillW = (barW - 4) * progress / 100;
  if (fillW > 0) {
    tft.fillRoundRect(x + 2, y + 2, fillW, barH - 4, 4, color);
  }

  // Percentage text
  tft.setTextColor(WHITE);
  tft.setTextDatum(MC_DATUM);
  char buf[10];
  sprintf(buf, "%d%%", progress);
  tft.drawString(buf, W / 2, y + barH / 2);
}

// ============================================
// Public Display Functions
// ============================================

bool initDisplay() {
  if (displayInitialized)
    return true;

  Serial.println("[Display] Initializing ILI9341...");

  tft.init();
  tft.setRotation(0); // Normal orientation
  tft.fillScreen(BG_DARK);
  tft.setTextWrap(false);

  // Boot splash
  drawHeader();

  // Version info
  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("v1.0", W / 2, H / 2);

  // Loading bar animation
  for (int i = 0; i <= 100; i += 10) {
    drawProgressBar(H / 2 + 40, i, CYAN);
    delay(50);
  }

  displayInitialized = true;
  Serial.println("[Display] Ready!");
  return true;
}

void displayStatus(const char *status) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar(status, CYAN);

  // Spinner dots animation area
  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Please wait...", W / 2, H / 2);
}

void displayWiFiInfo(const char *ip, const char *ssid) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[OK] WiFi Connected", GREEN);

  // Info panel
  drawPanel(110, 100);

  tft.setTextColor(GRAY);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Network:", 25, 125);
  tft.drawString("IP Address:", 25, 160);

  tft.setTextColor(WHITE);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(ssid, W - 25, 125);
  tft.drawString(ip, W - 25, 160);
}

void displayWiFiSetup(const char *apName) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[!] Setup Mode", ORANGE);

  // Info panel
  drawPanel(110, 130);

  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connect phone to WiFi:", W / 2, 135);

  tft.setTextColor(ORANGE);
  tft.setTextSize(2);
  tft.drawString(apName, W / 2, 175);
  tft.setTextSize(1);

  tft.setTextColor(CYAN);
  tft.drawString("to configure internet", W / 2, 220);
}

void displayPairingCode(const char *code) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[!] Pairing Mode", GOLD);

  // Code panel
  drawPanel(110, 130);

  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Enter code on website:", W / 2, 135);

  // Big pairing code
  tft.setTextColor(GOLD);
  tft.setTextSize(2);
  tft.drawString(code, W / 2, 175);
  tft.setTextSize(1);

  tft.setTextColor(GRAY);
  tft.drawString("researchmate.app", W / 2, 220);

  // Waiting animation hint
  tft.setTextColor(CYAN);
  tft.drawString("Waiting for confirmation...", W / 2, H - 40);
}

void displaySuccess() {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[OK] Paired & Ready", GREEN);

  // Ready panel
  drawPanel(110, 120);

  tft.setTextColor(GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("READY", W / 2, 140);
  tft.setTextSize(1);

  tft.setTextColor(WHITE);
  tft.drawString("Press button to scan", W / 2, 175);

  // Show WebServer IP Address on the screen!
  tft.setTextColor(CYAN);
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString("Web UI IP Address:", W / 2, 205);
    tft.setTextColor(WHITE);
    tft.drawString(WiFi.localIP().toString(), W / 2, 220);
  } else {
    tft.drawString("WiFi Disconnected", W / 2, 215);
  }

  // Hint
  tft.setTextColor(GRAY);
  tft.drawString("Auto-sync enabled", W / 2, H - 40);
}

void displayError(const char *error) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[X] Error", RED);

  // Error panel
  drawPanel(110, 80);

  tft.setTextColor(RED);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(error, W / 2, 150);

  tft.setTextColor(GRAY);
  tft.drawString("Please try again", W / 2, H - 40);
}

void displayScanning() {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("Scanning...", CYAN);

  // Camera icon placeholder
  drawPanel(110, 120);

  tft.setTextColor(CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("[ ]", W / 2, 160);
  tft.setTextSize(1);

  tft.setTextColor(WHITE);
  tft.drawString("Capturing image...", W / 2, 210);
}

void displayUploading(int progress) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("Uploading to Cloud", PURPLE);

  // Progress panel
  drawPanel(110, 100);

  tft.setTextColor(WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Syncing with ResearchMate...", W / 2, 135);

  drawProgressBar(165, progress, PURPLE);
}

void displayUploadComplete() {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();
  drawStatusBar("[OK] Upload Complete!", GREEN);

  // Success panel
  drawPanel(110, 100);

  tft.setTextColor(GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("DONE", W / 2, 160);
  tft.setTextSize(1);

  tft.setTextColor(GRAY);
  tft.drawString("Saved to cloud", W / 2, H - 40);
}

void displayColorDemo(uint16_t color, const char *colorName) {
  if (!displayInitialized)
    return;

  tft.fillScreen(color);
  tft.setTextColor(color == WHITE ? 0x0000 : WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString(colorName, W / 2, H / 2);
  tft.setTextSize(1);
}

void displaySDCardStatus(bool available, const char *info) {
  if (!displayInitialized)
    return;

  clearScreen();
  drawHeader();

  if (available) {
    drawStatusBar("[OK] SD Card Ready", GREEN);

    drawPanel(110, 100);
    tft.setTextColor(WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Storage Available", W / 2, 135);

    tft.setTextColor(CYAN);
    tft.drawString(info, W / 2, 170);
  } else {
    drawStatusBar("[!] No SD Card", GOLD);

    drawPanel(110, 80);
    tft.setTextColor(GRAY);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Insert SD card for", W / 2, 135);
    tft.drawString("offline backup", W / 2, 155);
  }
}

void clearDisplay() { clearScreen(); }

void displayCameraStatus(bool working) {
  if (!displayInitialized)
    return;
  Serial.printf("[Display] Camera: %s\n", working ? "OK" : "ERROR");
}

void displayTestPattern() {
  if (!displayInitialized)
    return;
  clearScreen();
  drawHeader();
  tft.setTextColor(WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Test Pattern", W / 2, H / 2);
}

// ============================================
// Camera Debug Display
// ============================================

void displayCameraDebug(int itemCount, const char *lastStatus,
                        unsigned long lastCaptureTime) {
  if (!displayInitialized)
    return;

  // Clear bottom section for debug panel
  tft.fillRect(0, H - 80, W, 80, BG_DARK);

  // Draw rounded panel background
  tft.fillRoundRect(10, H - 75, W - 20, 70, 5, BG_PANEL);

  // Title
  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CAMERA DEBUG", W / 2, H - 68);

  // Item count (large, cyan)
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  char countStr[32];
  sprintf(countStr, "%d ITEMS", itemCount);
  tft.drawString(countStr, W / 2, H - 50);

  // Last status with color based on result
  tft.setTextSize(1);
  uint16_t statusColor = GRAY;
  if (strstr(lastStatus, "OK") || strstr(lastStatus, "Success")) {
    statusColor = GREEN;
  } else if (strstr(lastStatus, "Error") || strstr(lastStatus, "Failed")) {
    statusColor = RED;
  } else if (strstr(lastStatus, "Upload")) {
    statusColor = ORANGE;
  }
  tft.setTextColor(statusColor);
  tft.drawString(lastStatus, W / 2, H - 30);

  // Time since last capture
  if (lastCaptureTime > 0) {
    unsigned long elapsed = (millis() - lastCaptureTime) / 1000;
    char timeStr[32];
    if (elapsed < 60) {
      sprintf(timeStr, "%lu sec ago", elapsed);
    } else {
      sprintf(timeStr, "%lu min ago", elapsed / 60);
    }
    tft.setTextColor(GRAY);
    tft.drawString(timeStr, W / 2, H - 15);
  } else {
    tft.setTextColor(GRAY);
    tft.drawString("No captures yet", W / 2, H - 15);
  }
}

void displayCaptureFlash() {
  if (!displayInitialized)
    return;

  // Quick cyan flash for visual feedback
  tft.fillScreen(CYAN);
  delay(80);

  // Show "CAPTURING..." text briefly
  tft.setTextColor(BG_DARK);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CAPTURING...", W / 2, H / 2);
  delay(120);

  // Screen will be redrawn by next display update
}
