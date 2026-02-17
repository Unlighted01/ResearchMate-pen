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

// High resolution capture mode
void setHighResCapture();

// Streaming mode
void setStreamingMode();

// Get frame directly
camera_fb_t* captureFrame();

// Return frame
void returnFrame(camera_fb_t* frame);

#endif