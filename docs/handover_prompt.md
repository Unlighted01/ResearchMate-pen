# 📦 Project Handover: ResearchMate Smart Pen (ESP32-S3)

## 🚀 Current Status: STABLE & WORKING

The firmware is fully functional. The "Infinite Capture Loop" issue has been resolved.

### ✅ Key Features Working:

1.  **Camera:** OV5640 initializes and captures images correctly.
2.  **Display:** ST7789 TFT shows live status, debug info, and "Blue Flash" on capture.
3.  **Button:** **BOOT Button (GPIO 0)** triggers capture/upload.
    - **Logic:** Press & Release -> Capture -> Upload to Supabase.
4.  **Web Interface:** Simplified to "Capture Preview" and "Upload" buttons (Auto-refresh removed to prevent loops).
5.  **Cloud:** Successfully uploads images to Supabase storage.
6.  **Offline SD Card Queuing:** (CODE WRITTEN - AWAITING USER HARDWARE TEST)
    - **Current State:** The SPI logic (`src/storage/storage.cpp`), cloud routing (`cloud.cpp`), and sync timer (`main.cpp`) have been fully written. However, the user had to step away before physically compiling or testing the SD Card integration. It is UNVERIFIED on hardware.
    - **Hardware:** MicroSD Card Module (SPI) connected to JTAG pins (`CS:42`, `MOSI:41`, `MISO:40`, `CLK:39`).
    - **Logic:** Automatically writes `image/jpeg` byte buffers to physical storage if Wi-Fi is disconnected during capture.
    - **Syncing:** A 10-second background polling timer in `main.cpp` automatically flushes cached photos back to Supabase the moment the connection is restored.

### 🛠️ Hardware Configuration (CRITICAL)

- **Board:** ESP32-S3-WROOM-N16R8-CAM (Freenove/Generic style)
- **Camera:** OV2640 (Custom pin mapping in `config.h`, NOT standard AI-Thinker)
- **Display:** ST7789 (Pins: MOSI=11, CLK=13, CS=10, DC=14, BL=8)
- **SD Card SPI:** (Pins: CS=42, MOSI=41, MISO=40, SCK=39)
- **Capture Button:** **GPIO 0** (The on-board **BOOT** button).
  - _Note: Previously tried GPIO 19 but it conflicted with USB D-._

### 🧩 Recent Fixes (The "Ghost Button" Saga)

- **Issue:** Device was infinitely capturing/uploading.
- **Cause 1:** GPIO 19 (USB D-) was used for button, triggering on USB traffic. -> **Fixed by moving to GPIO 0.**
- **Cause 2:** Web Interface "Auto" mode was left on. -> **Fixed by removing Auto button from HTML.**
- **Cause 3:** Interrupts were too sensitive. -> **Fixed by using Polling in `loop()`.**

### 📂 Code Structure

- `src/main.cpp`: Main logic, Web Server, Button Polling, SD SDync Task Loop.
- `src/config.h`: **ALL Pin Definitions are here.** (Camera, Display, Button, SD).
- `src/camera/`: Camera driver (customized for this board).
- `src/display/`: LovyanGFX driver & Debug UI.
- `src/cloud/`: Supabase Authentication & SD Queue logic.
- `src/storage/`: SD Card SPI format and Queue abstraction.

### ⏭️ Next Steps: Hardware Finalization

The foundational offline caching logic is now complete. To finish the production-ready "Smart Pen", the following features must be built:

1.  **ESP32 Power Management (Priority 1):**
    - **Goal:** Implement deep sleep states to conserve battery when the pen is idle.
    - **Action:** Track capture instances, map idle time, and program the ESP32 to drop to micro-amp power draw when not in use, waking instantly upon a button press.
2.  **Stabilize Camera Frame Geometry (Priority 2):**
    - **Goal:** Lock in the perfect UXGA/VGA streaming balance.
    - **Action:** Ensure the centralized OCR API receives the highest clarity possible without triggering an Out-Of-Memory (OOM) crash on the ESP32 hardware.

**"The Ghost is Busted, and the AI Pipeline is Live. Good luck!"** 👻🚀
