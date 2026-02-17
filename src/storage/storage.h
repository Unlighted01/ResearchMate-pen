#ifndef STORAGE_H
#define STORAGE_H

#include <cstdint>
#include <cstddef>

bool initSDCard();
bool saveImage(const char* filename, uint8_t* data, size_t size);
bool loadImage(const char* filename, uint8_t* buffer, size_t* size);

#endif