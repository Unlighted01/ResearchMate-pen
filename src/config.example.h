#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// ResearchMate Smart Pen Configuration - ESP32-S3-WROOM
// =============================================================================
// SETUP INSTRUCTIONS:
// 1. Copy this file to config.h
// 2. Replace placeholder values with your actual credentials
// 3. NEVER commit config.h to Git (it's in .gitignore)

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID_HERE"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD_HERE"

// Supabase API Configuration
#define SUPABASE_URL "YOUR_SUPABASE_URL_HERE"
#define SUPABASE_ANON_KEY "YOUR_SUPABASE_ANON_KEY_HERE"
#define SMART_PEN_ENDPOINT "/functions/v1/smart-pen"

// =============================================================================
// CAMERA CONFIGURATION - ESP32-S3-WROOM with EXTERNAL OV5640
// =============================================================================

#define CAMERA_MODEL_ESP32S3_EYE

// =============================================================================
// SPI TFT DISPLAY CONFIGURATION (1.47" 320x172)
// =============================================================================

#define TFT_MOSI 35
#define TFT_CLK  36
#define TFT_MISO 37
#define TFT_CS   38
#define TFT_DC   14
#define TFT_RST  21
#define TFT_BL   20

#define TFT_SPI_HOST HSPI_HOST
#define TFT_WIDTH  320
#define TFT_HEIGHT 172

// =============================================================================
// BUTTON & LED
// =============================================================================

#define BUTTON_PIN 0
#define LED_PIN 48

// =============================================================================
// DEVICE SETTINGS
// =============================================================================

#define DEVICE_NAME "ResearchMate-SmartPen"
#define FIRMWARE_VERSION "0.3.0-wroom"

#define IMAGE_QUALITY 12
#define IMAGE_SIZE FRAMESIZE_VGA

#define LVGL_H_RES TFT_WIDTH
#define LVGL_V_RES TFT_HEIGHT
#define LVGL_TICK_PERIOD_MS 5

#define DEEP_SLEEP_ENABLED false
#define DEEP_SLEEP_DURATION_US 10000000

#endif // CONFIG_H
