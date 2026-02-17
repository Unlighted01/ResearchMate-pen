#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// ResearchMate Smart Pen Configuration - ESP32-S3-WROOM
// =============================================================================

// WiFi Configuration
#define WIFI_SSID "Unlighted-2.4G"
#define WIFI_PASSWORD "Zekken212"

// Supabase API Configuration
#define SUPABASE_URL "https://jxevjkzojfbywxvtcwtl.supabase.co"
#define SUPABASE_ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp4ZXZqa3pvamZieXd4dnRjd3RsIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTk5MDc4MzEsImV4cCI6MjA3NTQ4MzgzMX0.hZL-wGTcmD9H0bsmj_jqzZ2iw1GZyJM5X14meIRKgNQ"
#define SMART_PEN_ENDPOINT "/functions/v1/smart-pen"

// =============================================================================
// CAMERA CONFIGURATION - ESP32-S3-WROOM with EXTERNAL OV5640
// =============================================================================

// CORRECT BOARD TYPE - ESP32-S3 with built-in camera (OceanLabz board)
#define CAMERA_MODEL_ESP32S3_EYE

// Camera pins are automatically defined in camera_pins.h based on CAMERA_MODEL_ESP32S3_EYE

// =============================================================================
// SPI TFT DISPLAY CONFIGURATION (1.47" 320x172)
// =============================================================================
// IMPORTANT: Updated to avoid conflicts with OV5640 camera pins
// Old conflicting pins: 8, 9, 10, 11, 13 were used by camera
// New conflict-free pins: 20, 21, 35, 36, 37, 38

#define TFT_MOSI 35  // Was 11 (conflicted with camera Y2)
#define TFT_CLK  36  // Was 13 (conflicted with camera PCLK)
#define TFT_MISO 37  // Was 12 (conflicted with camera Y6)
#define TFT_CS   38  // Was 10 (conflicted with camera Y5)
#define TFT_DC   14  // No conflict (keep as is)
#define TFT_RST  21  // Was 9 (conflicted with camera Y3)
#define TFT_BL   20  // Was 8 (conflicted with camera Y4)

#define TFT_SPI_HOST HSPI_HOST
#define TFT_WIDTH  320
#define TFT_HEIGHT 172

// =============================================================================
// BUTTON & LED
// =============================================================================

#define BUTTON_PIN 0    // GPIO0
#define LED_PIN 48      // RGB LED (if available)

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