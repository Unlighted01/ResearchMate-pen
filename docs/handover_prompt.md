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
4.  **Web Interface:** Serves pairing codes and handles manual triggers. (Currently moved to Port 8080, but still unreachable—suspect AP isolation or Modem Sleep).
5.  **Cloud:** Uploads large multipart image binaries successfully. The `HTTPClient` timeout has been safely extended to 30s to accommodate cold-starts on Supabase Edge functions.
6.  **Offline SD Card Queuing:** (WORKING)
    - **Hardware:** MicroSD Card Module (SPI) connected to pins (`CS:42`, `MOSI:41`, `MISO:40`, `CLK:39`).
    - **Logic:** Successfully writes `image/jpeg` byte buffers to physical storage (`/queue/scan_...jpg`) when requested.

### 🛠️ Hardware Configuration (CRITICAL)

- **Board:** ESP32-S3-WROOM-N16R8-CAM (8MB Flash, external PSRAM)
- **Display:** TFT_eSPI Library used. (Pins defined in `platformio.ini`: CS=38, DC=14, RST=21, MOSI=11, SCK=13, MISO=-1)
- **SD Card SPI:** (Pins: CS=42, MOSI=41, MISO=40, SCK=39) - Note: carefully isolated from Camera I2C lanes.
- **Capture Button:** **GPIO 1** (Wired direct to GND, uses internal `INPUT_PULLUP`).

### 🧩 Recent Fixes & Changes

- **Memory Allocation Crash (`0xffffffff`):** Enabled PSRAM compiler flags. Camera driver now successfully allocates the 640x480 frame buffer into SPIRAM.
- **Library Migration:** Replaced `LovyanGFX` with `TFT_eSPI` to resolve `sdkconfig.h` compilation errors on newer ESP32 Arduino Cores (v3.0+).
- **HTTP Timeout Bug:** Fixed `read Timeout` on Superbase uploads by extending the Arduino timeout threshold from 5 to 30 seconds.
- **SD Card SPI Conflict & Mount Failures:** Disabled TFT_MISO (`-1`) to free up pin overlap. Migrated the SD Card from `FSPI` to `HSPI` to stop collisions with `TFT_eSPI`. Also added explicit pull-ups and lowered the initialization frequency to 1MHz for stability.

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

While the hardware and memory are stable, the WebServer remains unreachable despite moving it to Port 8080 and verifying Wi-Fi connectivity. Next debugging focus:
1.  **AP Isolation / Local Network:** The `ping` command from the PC returned "Destination Host Unreachable", suggesting the home router's AP Isolation or a Guest Network policy is blocking cross-device TCP routing.
2.  **Anti-Sleep:** Further investigate aggressive RTOS modem-sleep disabling to prevent the Wi-Fi transceiver from ignoring pings.
