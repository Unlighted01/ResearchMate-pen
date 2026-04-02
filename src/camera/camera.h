#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h"

// Initialize the camera
bool initCamera();

// Capture an image
bool captureImage(uint8_t** buffer, size_t* size);

// Release captured image
void releaseImage();

// Set image quality (0=best, 63=worst)
void setImageQuality(int quality);

// Set resolution
void setImageResolution(int framesize);

// Streaming mode
void setStreamingMode();

// Get frame directly
camera_fb_t* captureFrame();

// Capture at UXGA with proper stabilization (drains queue, switches res, flushes AEC)
camera_fb_t* captureHighRes();

// Return frame
void returnFrame(camera_fb_t* frame);

#endif