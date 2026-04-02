#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// ResearchMate Smart Pen Configuration - ESP32-S3-WROOM
// =============================================================================

// WiFi & Network Configuration
#define AP_NAME "ResearchMate-Pen" // Name of the Captive Portal WiFi

// =============================================================================
// POWER MANAGEMENT
// =============================================================================
// Direct power mode - latch disabled

// Supabase API Configuration
#define SUPABASE_URL "https://jxevjkzojfbywxvtcwtl.supabase.co"
#define SUPABASE_ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp4ZXZqa3pvamZieXd4dnRjd3RsIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTk5MDc4MzEsImV4cCI6MjA3NTQ4MzgzMX0.hZL-wGTcmD9H0bsmj_jqzZ2iw1GZyJM5X14meIRKgNQ"
#define SMART_PEN_ENDPOINT "/functions/v1/smart-pen"

// =============================================================================
// CAMERA CONFIGURATION - ESP32-S3-WROOM with EXTERNAL OV2640
// =============================================================================

// Camera uses OceanLabz/ESP32S3-EYE pin configuration (hardcoded in camera.cpp)
// Pins: GPIO 4-18 (see camera/camera.cpp for details)
#define CAMERA_MODEL_ESP32S3_EYE

// =============================================================================
// SPI TFT DISPLAY CONFIGURATION (1.8" 128x160 portrait, ILI9163 driver)
// =============================================================================

#define TFT_MOSI 45   // Moved from 35 (35 conflicts with Octal PSRAM D5)
#define TFT_CLK  46   // Moved from 37 (37 conflicts with Octal PSRAM D7)
#define TFT_MISO -1   // Not connected (ILI9163 is write-only, 36 conflicts with PSRAM D6)
#define TFT_CS   38
#define TFT_DC   14
#define TFT_RST  21
#define TFT_BL   47

#define TFT_SPI_HOST FSPI_HOST
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// =============================================================================
// STORAGE CONFIGURATION (SD Card SPI)
// =============================================================================

#define SD_CS    42
#define SD_MOSI  41
#define SD_MISO  40
#define SD_SCK   39

// =============================================================================
// BUTTON & LED
// =============================================================================

#define BUTTON_PIN 2         // Multi-function button
#define CAPTURE_BUTTON_PIN 2   // Capture button
#define POWER_BUTTON_PIN 1     // Dedicated Power / Reset button (Changed from 3, as 3 is an ESP32S3 strapping pin that crashes USB)
#define LED_PIN 48           // RGB LED (if available)

// =============================================================================
// DEVICE SETTINGS
// =============================================================================

#define DEVICE_NAME "ResearchMate-SmartPen"
#define FIRMWARE_VERSION "0.3.0-wroom"

// Camera settings
#define IMAGE_QUALITY 12         // JPEG quality (0=best, 63=worst)
#define IMAGE_SIZE FRAMESIZE_VGA  // 640x480

// LVGL configuration
#define LVGL_H_RES TFT_WIDTH
#define LVGL_V_RES TFT_HEIGHT
#define LVGL_TICK_PERIOD_MS 5

// Deep sleep settings
#define DEEP_SLEEP_ENABLED false
#define DEEP_SLEEP_DURATION_US 10000000

#endif // CONFIG_H