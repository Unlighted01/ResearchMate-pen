/**
 * Camera Module Implementation for ESP32-S3-WROOM
 * OV5640 via parallel DVP interface
 */

#include "camera.h"
#include "config.h"
#include "esp_camera.h"

// ESP32-S3-EYE / OceanLabz Board Camera Pin Definitions (OV2640)
#define PWDN_GPIO_NUM     -1  // Not used
#define RESET_GPIO_NUM    -1  // Not used  
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4   // SDA
#define SIOC_GPIO_NUM     5   // SCL

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

static camera_fb_t* fb = nullptr;
static bool cameraInitialized = false;

bool initCamera() {
    if (cameraInitialized) {
        return true;
    }

    Serial.println("Initializing OV2640 camera (ESP32S3_EYE board)...");
    
    camera_config_t config = {};  // Initialize to zero!
    
    // Use predefined pins from camera_pins.h for ESP32S3_EYE
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    
    // Camera configuration
    config.xclk_freq_hz = 20000000;  // 20MHz XCLK
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return false;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s == nullptr) {
        Serial.println("Failed to get sensor!");
        return false;
    }
    
    // Apply sensor settings for OV2640
    s->set_vflip(s, 1);           // Flip vertically (fixes inverted image!)
    s->set_brightness(s, 1);       // Increase brightness (0 is too dark)
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
    s->set_whitebal(s, 1);        // AWB - Auto White Balance
    s->set_exposure_ctrl(s, 1);   // AE - Auto Exposure  
    s->set_gain_ctrl(s, 1);       // AGC - Auto Gain Control
    
    Serial.println("Camera initialized!");
    cameraInitialized = true;
    return true;
}

bool captureImage(uint8_t** buffer, size_t* size) {
    if (!cameraInitialized) {
        return false;
    }
    
    releaseImage();
    
    fb = esp_camera_fb_get();
    if (!fb) {
        return false;
    }
    
    *buffer = fb->buf;
    *size = fb->len;
    return true;
}

camera_fb_t* captureFrame() {
    if (!cameraInitialized) return nullptr;
    return esp_camera_fb_get();
}

void returnFrame(camera_fb_t* frame) {
    if (frame) esp_camera_fb_return(frame);
}

void releaseImage() {
    if (fb) {
        esp_camera_fb_return(fb);
        fb = nullptr;
    }
}

void setImageQuality(int quality) {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
    }
}

void setImageResolution(int framesize) {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, (framesize_t)framesize);
    }
}

void setHighResCapture() {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, FRAMESIZE_UXGA);
        s->set_quality(s, 15);
    }
}

void setStreamingMode() {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, FRAMESIZE_VGA);
        s->set_quality(s, 25);
    }
}

void triggerAutofocus() {
    Serial.println("Autofocus ready");
}