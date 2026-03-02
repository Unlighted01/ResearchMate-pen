#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

bool initSDCard();
String saveImageToSD(const uint8_t* data, size_t size);
String getNextPendingUpload();
bool deleteImageFromSD(const String& filename);
void wipeOfflineQueue();
uint8_t* readImageFromSD(const String& filename, size_t* outSize);

#endif