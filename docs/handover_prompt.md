# 📦 Project Handover: ResearchMate Smart Pen (ESP32-S3)

## 🚀 Current Status: STABLE & WORKING

The firmware is fully functional. The "Infinite Capture Loop", "Camera malloc failed", and "Display Driver Conflicts" have all been successfully resolved.

### ✅ Key Features Working:

1.  **Camera:** OV5640/OV2640 initializes and captures images correctly. Frame buffer allocation now utilizes PSRAM to prevent memory exhaustion, with a fallback to DRAM.
2.  **Display:** LCD initialization is now driven by `TFT_eSPI` (migrated from LovyanGFX for better compatibility with newer ESP32 cores).
3.  **Multi-Function Button:** The capture button has been moved to **GPIO 1** (Standard Digital Input) to avoid conflicts with the BOOT button (GPIO 0).
    - **Quick Tap:** Captures and displays a preview on the LCD.
    - **Double Tap:** Captures and uploads to Supabase Cloud.
    - **Long Press (>800ms):** Captures and saves directly to the offline SD Card queue (`handleSDCapture`).
4.  **Web Interface:** Serves pairing codes and handles manual triggers. (Currently investigating potential Modem Sleep drops on Port 80).
5.  **Cloud:** Uploads large multipart image binaries successfully. The `HTTPClient` timeout has been safely extended to 30s to accommodate cold-starts on Supabase Edge functions.

### 🛠️ Hardware Configuration (CRITICAL)

- **Board:** ESP32-S3-WROOM-N16R8-CAM (8MB Flash, external PSRAM)
- **Display:** TFT_eSPI Library used. (Pins defined in `platformio.ini`: CS=38, DC=14, RST=21, MOSI=11, SCK=13, MISO=-1)
- **SD Card SPI:** (Pins: CS=42, MOSI=41, MISO=40, SCK=39) - Note: carefully isolated from Camera I2C lanes.
- **Capture Button:** **GPIO 1** (Wired direct to GND, uses internal `INPUT_PULLUP`).

### 🧩 Recent Fixes & Changes

- **Memory Allocation Crash (`0xffffffff`):** Enabled PSRAM compiler flags. Camera driver now successfully allocates the 640x480 frame buffer into SPIRAM.
- **Library Migration:** Replaced `LovyanGFX` with `TFT_eSPI` to resolve `sdkconfig.h` compilation errors on newer ESP32 Arduino Cores (v3.0+).
- **HTTP Timeout Bug:** Fixed `read Timeout` on Superbase uploads by extending the Arduino timeout threshold from 5 to 30 seconds.
- **SD Card SPI Conflict:** Disabled TFT_MISO (`-1`) to free up pin overlap causing `sdSelectCard Failed` errors.

### ⚙️ How `platformio.ini` Works

The `platformio.ini` file controls the entire firmware build environment. Here's what we changed:
- **`platform = espressif32@6.5.0`:** Locks the SDK to a stable, tested version.
- **`-DBOARD_HAS_PSRAM`:** Essential flag that tells the ESP32 linker to map the external RAM chip, completely resolving the camera's `malloc` crashes.
- **`TFT_eSPI` configurations:** Display pins and drivers (ILI9341/ST7789) are defined directly in `build_flags` here instead of a separate `User_Setup.h` file for easier cross-compilation tracking.

### 📂 Code Structure

- `src/main.cpp`: Main logic, Web Server routing, and the Multi-Tap Button logic (`loop()`).
- `src/config.h`: Centralized pin definitions.
- `src/camera/`: Camera HAL driver (gracefully falls back to DRAM if PSRAM vanishes).
- `src/display/`: Cleaned up to wrap `TFT_eSPI` drawing calls.
- `src/cloud/`: Supabase Authentication & multipart HTTP uploads (30s timeout).
- `src/storage/`: SD Card SPI format and offline Queue abstraction (`saveImageToSD`).

### ⏭️ Next Steps: WebServer Stability

While the hardware and memory are stable, the device occasionally drops incoming HTML requests to the WebServer. Next debugging focus:
1.  **Port Allocation:** Check if `WiFiManager`'s temporary AP portal is failing to relinquish Port 80.
2.  **Anti-Sleep:** Further investigate aggressive RTOS modem-sleep disabling to prevent the Wi-Fi transceiver from ignoring pings.
