#include "storage.h"
#include "../config.h"
#include <esp_heap_caps.h>

// Define a custom SPI class instance for the SD card
SPIClass sdSPI(HSPI);

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

static bool sdCardInitialized = false;

bool initSDCard() {
  // Explicitly pull CS HIGH before SPI init to prevent floating state failures
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Many SD modules (especially MicroSD adapters) require a pull-up on MISO
  pinMode(SD_MISO, INPUT_PULLUP);

  // Initialize SPI bus for SD Card using configured pins
  // CRITICAL: We pass -1 for CS because the SD library MUST manually control
  // the CS pin over long multi-byte transactions. If we give it to hardware
  // SPI, it violently rapid-fires.
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);

  // Initialize SD card at a very conservative 1MHz for stable mounting
  if (!SD.begin(SD_CS, sdSPI, 1000000)) {
    LOG_ERROR("[SD] Card Mount Failed or Not Inserted");
    sdCardInitialized = false;
    return false;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    LOG_ERROR("[SD] No SD card attached");
    sdCardInitialized = false;
    return false;
  }

  LOG_DEBUG("[SD] SD Card initialized successfully");
  sdCardInitialized = true;

  // Ensure the queue directory exists
  if (!SD.exists("/queue")) {
    SD.mkdir("/queue");
  }

  return true;
}

String saveImageToSD(const uint8_t *data, size_t size) {
  if (!sdCardInitialized || !data || size == 0)
    return "";

  // Generate a unique filename using millis and random number
  // Helps avoid collisions during offline captures before NTP sync
  String filename = "/queue/scan_" + String(millis()) + "_" +
                    String(random(1000, 9999)) + ".jpg";

  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (!file) {
    LOG_ERROR("[SD] Failed to open file for writing: %s", filename.c_str());
    return "";
  }

  size_t written = file.write(data, size);
  file.close();

  if (written != size) {
    LOG_ERROR("[SD] Write failed. Expected %d bytes, wrote %d bytes", size,
              written);
    SD.remove(filename.c_str()); // Clean up partial file fragment
    return "";
  }

  LOG_DEBUG("[SD] Saved offline image: %s (%d bytes)", filename.c_str(), size);
  return filename;
}

String getNextPendingUpload() {
  if (!sdCardInitialized)
    return "";

  File root = SD.open("/queue");
  if (!root || !root.isDirectory())
    return "";

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      file.close();

      // Normalize path structure just in case the SD library omits the root
      if (!filename.startsWith("/")) {
        filename = "/queue/" + filename;
      } else if (!filename.startsWith("/queue/")) {
        filename = "/queue" + filename;
      }

      return filename;
    }
    file = root.openNextFile();
  }
  return "";
}

bool deleteImageFromSD(const String &filename) {
  if (SD.exists(filename.c_str())) {
    if (SD.remove(filename.c_str())) {
      LOG_DEBUG("[SD] Deleted file: %s", filename.c_str());
      return true;
    } else {
      LOG_ERROR("[SD] Failed to delete file: %s", filename.c_str());
      return false;
    }
  }
  return true;
}

uint8_t *readImageFromSD(const String &filename, size_t *outSize) {
  File file = SD.open(filename.c_str(), FILE_READ);
  if (!file) {
    LOG_ERROR("[SD] Failed to open file for reading: %s", filename.c_str());
    *outSize = 0;
    return nullptr;
  }

  size_t size = file.size();
  if (size == 0) {
    file.close();
    *outSize = 0;
    return nullptr;
  }

  // Attempt to allocate in PSRAM for large JPEGs, fallback to internal SRAM
  uint8_t *buffer = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (!buffer) {
    buffer = (uint8_t *)malloc(size);
  }

  if (!buffer) {
    LOG_ERROR("[SD] Failed to allocate memory for reading: %d bytes", size);
    file.close();
    *outSize = 0;
    return nullptr;
  }

  // CRITICAL FIX FOR SPI TIMEOUTS: Read in chunks and yield to the RTOS
  // The ESP32 will crash/hang the SPI bus if we do a blocking multi-kilobyte read
  size_t totalRead = 0;
  const size_t CHUNK_SIZE = 1024; // Read 1KB at a time

  while (totalRead < size) {
    size_t toRead = size - totalRead;
    if (toRead > CHUNK_SIZE) toRead = CHUNK_SIZE;
    
    size_t bytesRead = file.read(buffer + totalRead, toRead);
    if (bytesRead == 0) break; // End of file or error
    
    totalRead += bytesRead;
    vTaskDelay(pdMS_TO_TICKS(5)); // Yield 5ms to WiFi / Display tasks
  }

  file.close();

  if (totalRead != size) {
    LOG_ERROR("[SD] Read failed. Expected %d bytes, read %d bytes", size, totalRead);
    free(buffer);
    *outSize = 0;
    return nullptr;
  }

  *outSize = size;
  return buffer;
}

void wipeOfflineQueue() {
  File dir = SD.open("/queue");
  if (!dir || !dir.isDirectory()) {
    return;
  }

  File file = dir.openNextFile();
  int count = 0;
  while (file) {
    String filename = String("/queue/") + file.name();
    file.close(); // Close before deleting
    if (SD.remove(filename.c_str())) {
      count++;
      LOG_DEBUG("[SD WIPE] Deleted %s", filename.c_str());
    }
    file = dir.openNextFile();
  }
  LOG_DEBUG("[SD WIPE] Wiped %d total files from queue.", count);
}