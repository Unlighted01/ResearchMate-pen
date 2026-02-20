#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================
// SIMPLE DATA STRUCTURES
// ============================================

struct CaptureData {
    char filename[64];
    uint32_t timestamp;
    uint32_t filesize;
    uint8_t* image_buffer;
    char ocr_text[2048];
    float ocr_confidence;
};

struct WiFiStatusData {
    bool connected;
    uint32_t signal_strength;
    char ssid[32];
};

struct BatteryStatusData {
    float voltage;
    uint8_t percentage;
    bool is_charging;
};

struct DeviceStatusData {
    uint32_t uptime_ms;
    uint32_t free_heap;
    uint32_t free_psram;
    float temperature_c;
    WiFiStatusData wifi;
    BatteryStatusData battery;
    bool camera_initialized;
    bool display_ok;
};

struct PairingData {
    char pen_id[64];
    char pairing_code[7];
    char user_id[64];
    bool is_paired;
};

struct APIResponse {
    bool success;
    int http_code;
    char image_url[256];
    char item_id[64];
    char error_message[256];
    uint32_t response_time_ms;
};

struct DisplayContext {
   enum {
        SCREEN_SPLASH = 0,
        SCREEN_HOME = 1,
        SCREEN_CAPTURING = 2,
        SCREEN_PREVIEW = 3,
        SCREEN_UPLOADING = 4,
        SCREEN_SUCCESS = 5,
        SCREEN_ERROR = 6
    } current_screen;
    
    char status_message[128];
    char error_message[256];
    uint8_t progress_percent;
};

struct AppState {
    DeviceStatusData device_status;
    PairingData pairing;
    DisplayContext display;
    CaptureData last_capture;
    APIResponse last_api_response;
    
    bool camera_ready;
    bool display_ready;
    bool sd_ready;
    bool wifi_ready;
    bool paired;
    
    uint32_t last_capture_time;
    uint32_t last_sync_time;
    uint32_t last_status_update;
};

#endif // TYPES_H