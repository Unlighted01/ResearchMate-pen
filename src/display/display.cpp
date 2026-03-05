// ============================================
// TFT Display Driver - ResearchMate Smart Pen
// Beautiful UI matching web interface
// ============================================

#include "../config.h"
#include <SPI.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <qrcode.h>

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

public:
  LGFX(void)
  {
    { 
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST; // FSPI
      cfg.spi_mode = 0;                  
      cfg.freq_write = 27000000;         // Safe 27MHz for generic red PCB ILI9341
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO; // Set DMA channel

      cfg.pin_sclk = TFT_CLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc   = TFT_DC;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false; // Standard for ILI9341
      cfg.rgb_order        = false; 
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;

      _panel_instance.config(cfg);
    }
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = TFT_BL;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX tft;
static bool displayInitialized = false;

// Add forward declaration for button updater so TJpgDec can use it
extern void updateButtonState();

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Ultra-fast poll of button during decode chunks
  updateButtonState();

  // Check if part of the image is completely off screen
  if (y >= tft.height() || x >= tft.width())
    return 1; // Return 1 to continue decoding next block

  // Push the decoded block to the screen using LGFX format
  tft.pushImage(x, y, w, h, (lgfx::rgb565_t*)bitmap);
  return 1;
}

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
#define BLACK 0x0000     // black
#define GRAY 0x7BEF      // gray text

#define SCREEN_W tft.width()
#define SCREEN_H tft.height()
#define W SCREEN_W
#define H SCREEN_H

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
  tft.fillRect(0, 0, SCREEN_W, 50, BG_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);

  // Title
  tft.setTextColor(CYAN);
  tft.drawString("ResearchMate", SCREEN_W / 2, 18);
  tft.setTextColor(GRAY);
  tft.drawString("Smart Pen", SCREEN_W / 2, 36);

  // Bottom accent line
  tft.fillRect(60, 48, SCREEN_W - 120, 2, CYAN);
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
  tft.setBrightness(255);
  tft.fillScreen(BG_DARK);
  tft.setTextWrap(false);

  // Initialize JPEG Decoder
  TJpgDec.setJpgScale(
      4); // Scale factor 4 native downsampling (1600x1200 -> 400x300)
  TJpgDec.setSwapBytes(false); // LGFX handles RGB565 correctly natively
  TJpgDec.setCallback(tft_output);

  // Boot splash
  drawHeader();

  // Version info
  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("v1.0", SCREEN_W / 2, SCREEN_H / 2);

  // Loading bar animation
  for (int i = 0; i <= 100; i += 10) {
    drawProgressBar(SCREEN_H / 2 + 40, i, CYAN);
    delay(50);
  }

  displayInitialized = true;
  Serial.println("[Display] Ready!");
  return true;
}

// ============================================
// UI State & Global Variables
// ============================================
static bool wifiConnected = false;
static bool devicePaired = false;
static String currentUIMode = "BOOTING";
static int offlineQueueCount = 0;
static String lastActionText = "";
static bool lastActionIsError = false;

// Helpers to push state updates without redrawing everything
void setWiFiStatus(bool connected) { wifiConnected = connected; }
void setPairingStatus(bool paired) { devicePaired = paired; }
void setUIMode(const char *mode) { currentUIMode = mode; }
void setQueueCount(int count) { offlineQueueCount = count; }
void setLastAction(const char *action, bool isError) { 
  lastActionText = action; 
  lastActionIsError = isError; 
}

// ============================================
// 3-Zone UI Layout Renderers
// ============================================

void drawTopBar() {
  if (!displayInitialized) return;

  tft.fillRect(0, 0, W, 32, BG_PANEL);

  // Left: WiFi Status
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(wifiConnected ? GREEN : RED);
  tft.drawString(wifiConnected ? "[WIFI ON]" : "[WIFI OFF]", 5, 16);

  // Center: Current Mode
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(currentUIMode, W / 2, 16);

  // Right: Pairing Status
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(devicePaired ? CYAN : GRAY);
  tft.drawString(devicePaired ? "[PAIRED]" : "[UNPAIRED]", W - 5, 16);

  // Bottom border line for Top Bar
  tft.fillRect(0, 31, W, 1, CYAN);
}

void drawBottomPanel() {
  if (!displayInitialized) return;

  tft.fillRect(0, 272, W, 48, BG_PANEL);
  
  // Top border line for Bottom Panel
  tft.fillRect(0, 272, W, 1, CYAN);

  // Text: Last Action
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(lastActionIsError ? RED : GREEN);
  tft.drawString(lastActionText.length() > 0 ? lastActionText : "Ready", W / 2, 286);

  // Text: Queue Count
  char qBuf[32];
  sprintf(qBuf, "Offline Queue: %d", offlineQueueCount);
  tft.setTextColor(GRAY);
  tft.drawString(qBuf, W / 2, 304);
}

// ============================================
// Viewfinder Zone (Middle 240x240)
// ============================================

void displayWiFiSetupQR(const char *ssid) {
  if (!displayInitialized) return;

  // Clear viewfinder area entirely
  tft.fillRect(0, 32, W, 240, BG_DARK);

  setUIMode("SETUP AP");
  drawTopBar();
  drawBottomPanel();

  // Build the standardized WiFi string
  String wifiStr = String("WIFI:T:nopass;S:") + ssid + ";;";

  // Generate QR Code data
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, wifiStr.c_str());

  // Calculate size and position to center it nicely in the viewfinder
  int boxSize = 2; // Reduced for smaller screens
  int qrWidth = qrcode.size * boxSize;
  int startX = (SCREEN_W - qrWidth) / 2;
  int startY = 20 + ((SCREEN_H - 40 - qrWidth) / 2);

  // Draw white background for contrast
  tft.fillRect(startX - 10, startY - 10, qrWidth + 20, qrWidth + 20, WHITE);

  // Draw the QR Code blocks
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * boxSize), startY + (y * boxSize), boxSize, boxSize, BLACK);
      }
    }
  }

  // Draw text hint below the QR code
  tft.setTextColor(CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Scan QR Setup", SCREEN_W / 2, startY + qrWidth + 15);
}

void displayPairingCode(const char *code) {
  if (!displayInitialized) return;

  // Clear viewfinder area entirely
  tft.fillRect(0, 20, SCREEN_W, SCREEN_H - 40, BG_DARK);

  setUIMode("PAIRING");
  drawTopBar();
  drawBottomPanel();

  tft.setTextColor(GRAY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Enter code on site:", SCREEN_W / 2, 50);

  // Big pairing code
  tft.setTextColor(GOLD);
  tft.setTextSize(3);
  tft.drawString(code, SCREEN_W / 2, 90);
  tft.setTextSize(1);

  // Waiting animation hint
  tft.setTextColor(CYAN);
  tft.drawString("Waiting for link...", SCREEN_W / 2, 140);
}

void displayReady() {
  if (!displayInitialized) return;
  setUIMode("READY");
  setLastAction("Camera Active", false);
  drawTopBar();
  drawBottomPanel();
}

void displayCaptureFlash() {
  if (!displayInitialized) return;

  // Flash ONLY the viewfinder zone
  tft.fillRect(0, 20, SCREEN_W, SCREEN_H - 40, CYAN);
  delay(50);

  // Show "CAPTURING..." text briefly
  tft.setTextColor(BG_DARK);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CAPTURING...", SCREEN_W / 2, SCREEN_H / 2);
  delay(50);
}

void displayDrawFrame(const uint8_t *jpg_data, size_t jpg_len) {
  if (!displayInitialized || !jpg_data) return;

  int viewHeight = SCREEN_H - 40;
  
  // Protect the top and bottom panels
  tft.setClipRect(0, 20, SCREEN_W, viewHeight);

  // With a 320x172 screen and 400x300 downscaled image:
  // Dynamically center it.
  int xOffset = (SCREEN_W - 400) / 2;
  int yOffset = 20 + (viewHeight - 300) / 2;
  
  TJpgDec.drawJpg(xOffset, yOffset, jpg_data, jpg_len);

  tft.clearClipRect();
  
  // CRITICAL FIX: Rapidly rendering JPEGs causes TFT_eSPI to hold the SPI bus
  // Host clamping the CS hardware pin. We MUST forcibly end the transaction
  // here so the SD card can use the SPI module safely later.
  tft.endWrite();
}

// ============================================
// Factory Reset Wipe UI
// ============================================

void displayWipeStart() {
  if (!displayInitialized) return;
  tft.setTextColor(RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("FACTORY RESET", SCREEN_W / 2, SCREEN_H / 2 - 20);
  
  tft.setTextColor(GOLD);
  tft.setTextSize(1);
  tft.drawString("Hold Power to confirm...", SCREEN_W / 2, SCREEN_H / 2 + 10);
}

void displayWipeProgress(int pct) {
  if (!displayInitialized) return;
  drawProgressBar(SCREEN_H / 2 + 30, pct, RED);
}

void displayWipeCancelled() {
  if (!displayInitialized) return;
  tft.setTextColor(GREEN);
  tft.fillRect(0, SCREEN_H / 2 + 5, SCREEN_W, 30, BG_DARK); // Clear "Hold" text
  tft.drawString("Reset Cancelled", SCREEN_W / 2, SCREEN_H / 2 + 10);
}

void displayWipeComplete() {
  if (!displayInitialized) return;
  tft.fillRect(0, SCREEN_H / 2 - 30, SCREEN_W, 100, BG_DARK);
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.drawString("WIPED!", SCREEN_W / 2, SCREEN_H / 2);
}

void displaySleep() {
  if (!displayInitialized) return;
  tft.fillScreen(BLACK);
  tft.waitDisplay();
  tft.setBrightness(0);
  tft.sleep();
}

