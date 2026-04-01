// ============================================
// TFT Display Driver - ResearchMate Smart Pen
// ILI9163 1.8" — 128x160 portrait
// ============================================

#include "../config.h"
#include <SPI.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <qrcode.h>

// ============================================
// LGFX — ILI9163 driver, 128x160
// ILI9163 shares all command opcodes with ILI9341 (no white-screen risk)
// but sends correct gamma/VCOM tables for 128x160 panels.
// ILI9341 gamma is tuned for 240x320 — wrong tables = washed-out colors.
// ============================================
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9163 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 27000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = TFT_CLK;
      cfg.pin_mosi    = TFT_MOSI;
      cfg.pin_miso    = TFT_MISO;
      cfg.pin_dc      = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 128;
      cfg.panel_height     = 160;
      cfg.offset_x         = 2;   // ILI9163 GRAM is 132 wide, center 128px panel
      cfg.offset_y         = 1;   // ILI9163 GRAM is 162 tall, center 160px panel
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
  }
};

static LGFX tft;
static bool displayInitialized = false;

// ============================================
// JPEG decoder callback
// ============================================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height() || x >= tft.width()) return 1;
  if (x < 0) return 1; // skip blocks that start left of screen (right edge clips automatically)
  tft.pushImage(x, y, w, h, (lgfx::swap565_t*)bitmap);
  return 1;
}

// ============================================
// Color palette (RGB565) — high contrast for small TFT
// ============================================
#define BG_DARK   0x0000  // pure black background
#define BG_PANEL  0x1082  // very dark gray for bars
#define BG_STATUS 0x2104  // dark gray for progress bg
#define CYAN      0x07FF  // pure cyan
#define GREEN     0x07E0  // pure green
#define GOLD      0xFFE0  // pure yellow
#define ORANGE    0xFD20  // orange
#define RED       0xF800  // pure red
#define WHITE     0xFFFF  // white
#define BLACK     0x0000  // black
#define GRAY      0x8410  // medium gray

// ============================================
// Layout — portrait 128 × 160
//
//  ┌──── 128px ────┐
//  │ W+  MODE   P+ │  y=0   top bar  16px
//  ├── (cyan line) ┤  y=16
//  │               │
//  │  content zone │  y=17  h=119px
//  │               │
//  ├── (cyan line) ┤  y=136
//  │ Action   Q:N  │  y=137 bot bar  23px
//  └───────────────┘  y=160
// ============================================
#define W         tft.width()
#define H         tft.height()
#define TOP_H     16
#define BOT_H     24
#define CONTENT_Y (TOP_H)
#define CONTENT_H (H - TOP_H - BOT_H)
#define BOT_Y     (H - BOT_H)

#include "display.h"

// ============================================
// UI state
// ============================================
static bool   wifiConnected     = false;
static bool   devicePaired      = false;
static String currentUIMode     = "BOOTING";
static int    offlineQueueCount = 0;
static String lastActionText    = "";
static bool   lastActionIsError = false;

void setWiFiStatus(bool c)                          { wifiConnected = c; }
void setPairingStatus(bool p)                       { devicePaired = p; }
void setUIMode(const char *m)                       { currentUIMode = m; }
void setQueueCount(int n)                           { offlineQueueCount = n; }
void setLastAction(const char *a, bool isError)     { lastActionText = a; lastActionIsError = isError; }

// ============================================
// Helpers
// ============================================
static void clearContent() {
  tft.fillRect(0, CONTENT_Y, W, CONTENT_H, BG_DARK);
}

static void drawProgressBar(int y, int pct, uint16_t color) {
  int x    = 6;
  int barW = W - 12;
  int barH = 10;
  tft.fillRoundRect(x, y, barW, barH, 2, BG_STATUS);
  int fillW = (barW - 2) * pct / 100;
  if (fillW > 0)
    tft.fillRoundRect(x + 1, y + 1, fillW, barH - 2, 1, color);
}

// ============================================
// Top bar  (y=0, h=16)
// ============================================
void drawTopBar() {
  if (!displayInitialized) return;

  tft.fillRect(0, 0, W, TOP_H, BG_PANEL);
  tft.setTextSize(1);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(wifiConnected ? GREEN : RED);
  tft.drawString(wifiConnected ? "W+" : "W-", 2, TOP_H / 2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString(currentUIMode.c_str(), W / 2, TOP_H / 2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(devicePaired ? CYAN : GRAY);
  tft.drawString(devicePaired ? "P+" : "P-", W - 2, TOP_H / 2);

  tft.fillRect(0, TOP_H, W, 1, CYAN);
}

// ============================================
// Bottom bar  (y=BOT_Y, h=24)
// ============================================
void drawBottomPanel() {
  if (!displayInitialized) return;

  tft.fillRect(0, BOT_Y - 1, W, 1, CYAN);
  tft.fillRect(0, BOT_Y, W, BOT_H, BG_PANEL);

  tft.setTextSize(1);

  // Last action — left-aligned, max ~13 chars at size 1
  String action = lastActionText.length() > 0 ? lastActionText : "Ready";
  if (action.length() > 13) action = action.substring(0, 12) + "~";
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(lastActionIsError ? RED : GREEN);
  tft.drawString(action.c_str(), 2, BOT_Y + BOT_H / 2);

  // Queue count — right-aligned
  char qBuf[8];
  sprintf(qBuf, "Q:%d", offlineQueueCount);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(offlineQueueCount > 0 ? GOLD : GRAY);
  tft.drawString(qBuf, W - 2, BOT_Y + BOT_H / 2);
}

// ============================================
// Init + boot splash
// ============================================
bool initDisplay() {
  if (displayInitialized) return true;

  Serial.println("[Display] Initializing 128x160 (ILI9163)...");

  tft.init();
  tft.setRotation(0);       // Portrait: 128 wide x 160 tall
  tft.setBrightness(255);
  tft.fillScreen(BG_DARK);
  tft.setTextWrap(false);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true); // TJpgDec will swap high/low bytes so memory is big-endian (swap565_t friendly)
  TJpgDec.setCallback(tft_output);

  // Boot splash
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.drawString("Research", W / 2, H / 2 - 18);
  tft.drawString("Mate", W / 2, H / 2 + 2);
  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.drawString("Smart Pen v1.0", W / 2, H / 2 + 22);
  tft.fillRect(W / 2 - 30, H / 2 + 32, 60, 1, CYAN);

  displayInitialized = true;
  Serial.println("[Display] Ready.");
  return true;
}

void clearScreen()     { tft.fillScreen(BG_DARK); }
void clearViewfinder() { if (displayInitialized) clearContent(); }

void displayReady() {
  if (!displayInitialized) return;
  setUIMode("READY");
  setLastAction("Camera Active", false);
  clearContent();
  drawTopBar();
  drawBottomPanel();
}

// ============================================
// WiFi Setup QR
// Content zone: 128 × 120px
// QR ver3 at boxSize=3 → 87×87px, centered
// Text hints below QR (120 - 87 = 33px)
// ============================================
void displayWiFiSetupQR(const char *ssid) {
  if (!displayInitialized) return;

  clearContent();
  setUIMode("SETUP");
  drawTopBar();
  drawBottomPanel();

  String wifiStr = String("WIFI:T:nopass;S:") + ssid + ";;";
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, wifiStr.c_str());

  int boxSize = 3;
  int qrPx    = qrcode.size * boxSize;         // ~87px
  int qrX     = (W - qrPx) / 2;               // center horizontally (~20px margin)
  int qrY     = CONTENT_Y + 4;

  tft.fillRect(qrX - 3, qrY - 3, qrPx + 6, qrPx + 6, WHITE);
  for (uint8_t row = 0; row < qrcode.size; row++)
    for (uint8_t col = 0; col < qrcode.size; col++)
      if (qrcode_getModule(&qrcode, col, row))
        tft.fillRect(qrX + col * boxSize, qrY + row * boxSize, boxSize, boxSize, BLACK);

  // Text below QR — compact, 2 lines
  int ty = qrY + qrPx + 6;
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  // Truncate SSID if too long (128px / 6px per char ≈ 21 chars)
  String s = String(ssid);
  if (s.length() > 20) s = s.substring(0, 19) + "~";
  tft.drawString(s.c_str(), W / 2, ty);
  tft.setTextColor(GOLD);
  tft.drawString("192.168.4.1", W / 2, ty + 12);
}

// ============================================
// Pairing Code
// ============================================
void displayPairingCode(const char *code) {
  if (!displayInitialized) return;

  clearContent();
  setUIMode("PAIRING");
  drawTopBar();
  drawBottomPanel();

  int cy = CONTENT_Y + CONTENT_H / 2;

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.drawString("Enter on website:", W / 2, cy - 28);

  tft.setTextColor(GOLD);
  tft.setTextSize(2);
  tft.drawString(code, W / 2, cy - 8);

  tft.setTextColor(CYAN);
  tft.setTextSize(1);
  tft.drawString("Waiting...", W / 2, cy + 16);
}

// ============================================
// Camera frame
// Input: 320x240 JPEG. Scale 1/2 → 160x120.
// Draw at x=0, y=CONTENT_Y. Right 32px clip naturally at screen edge (128px wide).
// ============================================
void displayDrawFrame(const uint8_t *jpg_data, size_t jpg_len) {
  if (!displayInitialized || !jpg_data) return;

  TJpgDec.setJpgScale(2);
  TJpgDec.drawJpg(0, CONTENT_Y, jpg_data, jpg_len);
  TJpgDec.setJpgScale(1);
  tft.endWrite();
}

void displayCaptureFlash() {
  if (!displayInitialized) return;
  tft.fillRect(0, CONTENT_Y, W, CONTENT_H, CYAN);
  delay(40);
  tft.setTextColor(BG_DARK);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CAPTURING", W / 2, CONTENT_Y + CONTENT_H / 2);
  delay(40);
}

// ============================================
// Factory reset
// ============================================
void displayWipeStart() {
  if (!displayInitialized) return;
  clearContent();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.drawString("RESET", W / 2, CONTENT_Y + CONTENT_H / 2 - 18);
  tft.setTextColor(GOLD);
  tft.setTextSize(1);
  tft.drawString("Hold Power", W / 2, CONTENT_Y + CONTENT_H / 2 + 4);
}

void displayWipeProgress(int pct) {
  if (!displayInitialized) return;
  drawProgressBar(CONTENT_Y + CONTENT_H / 2 + 20, pct, RED);
}

void displayWipeCancelled() {
  if (!displayInitialized) return;
  tft.fillRect(0, CONTENT_Y + CONTENT_H / 2, W, 20, BG_DARK);
  tft.setTextColor(GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Cancelled", W / 2, CONTENT_Y + CONTENT_H / 2 + 8);
}

void displayWipeComplete() {
  if (!displayInitialized) return;
  clearContent();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.drawString("WIPED!", W / 2, CONTENT_Y + CONTENT_H / 2);
}

void displayFactoryResetProgress(int pct) {
  if (!displayInitialized) return;

  if (pct < 0) { displayReady(); return; }

  if (pct == 0) {
    clearContent();
    setUIMode("HOLD RESET");
    drawTopBar();
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.drawString("RESET?", W / 2, CONTENT_Y + 40);
    tft.setTextSize(1);
    tft.setTextColor(GRAY);
    tft.drawString("Release to cancel", W / 2, CONTENT_Y + 62);
  }

  drawProgressBar(CONTENT_Y + 78, pct, RED);
}

// ============================================
// Sleep / Sync
// ============================================
void displaySleep() {
  if (!displayInitialized) return;
  tft.fillScreen(BLACK);
  tft.waitDisplay();
  tft.setBrightness(0);
  tft.sleep();
}

void displaySyncing() {
  if (!displayInitialized) return;
  clearContent();
  tft.fillRect(0, CONTENT_Y, W, CONTENT_H, 0x031A);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("SYNCING...", W / 2, CONTENT_Y + CONTENT_H / 2);
}

// ============================================
// Legacy compat
// ============================================
void drawHeader() {
  tft.fillRect(0, 0, W, TOP_H, BG_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(CYAN);
  tft.setTextSize(1);
  tft.drawString("ResearchMate", W / 2, TOP_H / 2);
  tft.fillRect(0, TOP_H, W, 1, CYAN);
}
