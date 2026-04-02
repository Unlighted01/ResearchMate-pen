/**
 * Camera Module Implementation for ESP32-S3
 * OceanLabz / ESP32-S3-EYE Camera Pin Configuration
 */

#include "camera.h"
#include "../config.h"
#include "esp32-hal-psram.h"
#include "esp_camera.h"
#include <Arduino.h>

// ESP32-S3-EYE Camera Pin Definitions (Original Layout)
#define PWDN_GPIO_NUM -1  // Not used
#define RESET_GPIO_NUM -1 // Not used
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4 // SDA
#define SIOC_GPIO_NUM 5 // SCL

#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 10
#define Y4_GPIO_NUM 8
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 11

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

static camera_fb_t *fb = nullptr;
static bool cameraInitialized = false;

bool initCamera() {
  if (cameraInitialized) {
    return true;
  }

  Serial.println("Initializing camera (ESP32-S3-EYE pins)...");

  camera_config_t config = {}; // Initialize to zero!

  // Use pre-defined pins
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

  // Camera configuration - optimized for cool thermal output and max OCR
  // readability
  config.xclk_freq_hz =
      10000000; // 10MHz XCLK halves framerate to ~15FPS, vastly reducing heat
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size =
      FRAMESIZE_UXGA;       // 1600x1200 HD resolution for crystal clear OCR
  config.jpeg_quality = 12; // Adjusted back to 12 since we have safety buffers
  config.fb_count =
      2; // Increased to 2 to prevent DMA pipe collapse returning NULL

  // CRUCIAL MEMORY FIX: Force buffer allocation into internal RAM first!
  // Prevents 'cam_dma_config(300): frame buffer malloc failed' on WROOM chips
  // without 8MB Octal PSRAM.
  config.fb_location = CAMERA_FB_IN_PSRAM;
  if (!psramFound()) {
    Serial.println(
        "[Camera] No PSRAM detected! Falling back to internal SRAM...");
    config.fb_location = CAMERA_FB_IN_DRAM;
    // CRITICAL MEMORY FIX: Reduce to QVGA (320x240) to save ~100KB DRAM 
    // for WiFiManager portal strings and WebServer.
    config.frame_size = FRAMESIZE_QVGA; 
    config.fb_count = 1; // Single buffer in DRAM saves another 50KB
  }

  // Use LATEST so it drops stale frames and always gives us the current snapshot
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s == nullptr) {
    Serial.println("Failed to get sensor!");
    return false;
  }

  // Apply sensor settings
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);      // AWB
  s->set_exposure_ctrl(s, 1); // AE
  s->set_gain_ctrl(s, 1);     // AGC

  // Fix rotation - image was upside down
  s->set_hmirror(s, 0); // Horizontal mirror disabled (fixes mirror issue)
  s->set_vflip(s, 1);   // Vertical flip (fixes upside-down)

  Serial.println("Camera initialized!");
  cameraInitialized = true;
  return true;
}

bool captureImage(uint8_t **buffer, size_t *size) {
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

camera_fb_t *captureFrame() {
  if (!cameraInitialized) {
    Serial.println("[Camera] ERROR: Camera not initialized!");
    return nullptr;
  }

  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("[Camera] ERROR: esp_camera_fb_get returned NULL");
  }
  return frame;
}

void returnFrame(camera_fb_t *frame) {
  if (frame)
    esp_camera_fb_return(frame);
}

void releaseImage() {
  if (fb) {
    esp_camera_fb_return(fb);
    fb = nullptr;
  }
}

void setImageQuality(int quality) {
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_quality(s, quality);
  }
}

void setImageResolution(int framesize) {
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, (framesize_t)framesize);
  }
}

camera_fb_t* captureHighRes() {
  if (!cameraInitialized) {
    Serial.println("[Camera] ERROR: Camera not initialized!");
    return nullptr;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("[Camera] ERROR: Could not get sensor!");
    return nullptr;
  }

  // Step 1: Drain any stale frame sitting in the queue.
  camera_fb_t *stale = esp_camera_fb_get();
  if (stale) {
    Serial.printf("[Camera] Drained stale frame (%u bytes)\n", stale->len);
    esp_camera_fb_return(stale);
  }

  // Apply text-optimized sensor settings for OCR capture (OV5640)
  s->set_quality(s, 4);        // near-best JPEG quality for sharp text
  s->set_contrast(s, 2);       // boost text edge sharpness
  s->set_saturation(s, -2);    // reduce color noise, better text/bg separation
  s->set_brightness(s, 1);     // slight brightness boost for shadow text
  s->set_sharpness(s, 2);      // OV5640 hardware sharpening
  Serial.println("[Camera] Applied OCR sensor settings (quality=4, contrast+2, sharp+2)");

  if (psramFound()) {
    // PSRAM available: switch to UXGA for max OCR quality
    s->set_framesize(s, FRAMESIZE_UXGA);
    Serial.println("[Camera] PSRAM found — switching to UXGA for capture...");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Flush transition frames so AEC/AWB converge with new sensor settings
    for (int i = 0; i < 6; i++) {
      camera_fb_t *flush = esp_camera_fb_get();
      if (flush) {
        Serial.printf("[Camera] Flushed frame %d (%u bytes, %dx%d)\n",
                      i + 1, flush->len, flush->width, flush->height);
        esp_camera_fb_return(flush);
      } else {
        Serial.printf("[Camera] Flush %d: no frame yet\n", i + 1);
      }
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  } else {
    // No PSRAM: DMA buffers are QVGA-sized, can't switch to UXGA
    Serial.println("[Camera] No PSRAM — capturing at QVGA");
    vTaskDelay(pdMS_TO_TICKS(100));

    // Flush one frame to get fresh exposure
    camera_fb_t *flush = esp_camera_fb_get();
    if (flush) {
      esp_camera_fb_return(flush);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  // Capture the real frame
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame) {
    Serial.printf("[Camera] Captured frame: %u bytes, %dx%d\n",
                  frame->len, frame->width, frame->height);
  } else {
    Serial.println("[Camera] ERROR: Capture failed");
  }

  // Restore default sensor settings for preview
  s->set_quality(s, 12);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_brightness(s, 0);
  s->set_sharpness(s, 0);

  return frame;
}

void setStreamingMode() {
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_VGA);
    s->set_quality(s, 25);
  }
}

void triggerAutofocus() { Serial.println("Autofocus ready"); }