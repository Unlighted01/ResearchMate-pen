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

### 🛠️ Hardware Configuration (CRITICAL)

- **Board:** ESP32-S3-WROOM-N16R8-CAM (Freenove/Generic style)
- **Camera:** OV2640 (Custom pin mapping in `config.h`, NOT standard AI-Thinker)
- **Display:** ST7789 (Pins: MOSI=11, CLK=13, CS=10, DC=14, BL=8)
- **Capture Button:** **GPIO 0** (The on-board **BOOT** button).
  - _Note: Previously tried GPIO 19 but it conflicted with USB D-._

### 🧩 Recent Fixes (The "Ghost Button" Saga)

- **Issue:** Device was infinitely capturing/uploading.
- **Cause 1:** GPIO 19 (USB D-) was used for button, triggering on USB traffic. -> **Fixed by moving to GPIO 0.**
- **Cause 2:** Web Interface "Auto" mode was left on. -> **Fixed by removing Auto button from HTML.**
- **Cause 3:** Interrupts were too sensitive. -> **Fixed by using Polling in `loop()`.**

### 📂 Code Structure

- `src/main.cpp`: Main logic, Web Server, Button Polling, Task Loop.
- `src/config.h`: **ALL Pin Definitions are here.** (Camera, Display, Button).
- `src/camera/`: Camera driver (customized for this board).
- `src/display/`: LovyanGFX driver & Debug UI.

### ⏭️ Next Steps for New "Antigravity" Agent:

1.  **Resume:** The user is migrating to a new laptop.
2.  **Verify:** Ask user to upload code to confirm environment is set up.
3.  **Future Feature:** The user mentioned "1st press capture, 2nd press upload".
    - _Current State:_ It does both in one go (Capture -> Upload).
    - _To Do:_ Split this logic if they still want that specific 2-step flow.

**"The Ghost is Busted. Good luck on the new machine!"** 👻🚫
