#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// Simple data structure for capture info
struct CaptureData {
    char filename[64];
    uint32_t timestamp;
    uint32_t filesize;
};

// WiFi status
struct WiFiStatusData {
    bool connected;
    char ssid[32];
};

// Battery status
struct BatteryStatusData {
    float voltage;
    uint8_t percentage;
    bool is_charging;
};

// Device status
struct DeviceStatusData {
    uint32_t uptime_ms;
    uint32_t free_heap;
    bool camera_initialized;
};

#endif // TYPES_H