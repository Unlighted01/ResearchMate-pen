# 📦 AI Handover: ResearchMate Smart Pen (ESP32-S3)

## 🚀 Architectural Context
The ResearchMate Smart Pen is a custom hardware device built on the **ESP32-S3-WROOM-1** (N16R8) with an integrated **OV2640 camera** and **ILI9341 LCD**. It serves as the physical ingestion point for the ecosystem, capturing high-resolution "Research Scans" and piping them to the Cloud.

### 🧠 Strategic "Magic" Logic (Read Carefully)
1.  **Strapping Pin Evasion (GPIO 3):**
    - **CRITICAL:** GPIO 3 is a hardware strapping pin on the S3. If pulled low at boot, it kills the USB CDC port. **Do not use GPIO 3 for any input.**
2.  **Concurrency & Watchdogs:**
    - Firmware utilizes **FreeRTOS** tasks. All blocking waits use `vTaskDelay` to yield to the RTOS scheduler and prevent hardware watchdog resets.
    - Display updates use **LovyanGFX**, which is significantly faster than TFT_eSPI, allowing smooth camera previews while managing background SD card writes.
3.  **The Ingestion Pipeline:** 
    - The pen uploads raw JPEG buffers directly to the Supabase Edge Function (`/smart-pen/index.ts`).
    - From there, the Edge Function proxies the image to the **Vercel OCR API**. 
4.  **Stability & Power Management (NEW):**
    - **WiFi Sleep:** `WiFi.setSleep(false)` is called *before* connection in `setup()` to prevent the S3 radio from entering low-power drops during the initial handshake.
    - **WiFi Persistence (NEW):** Manual `WiFi.disconnect()` was removed from `setup()` to prevent the ESP32 from clearing saved session credentials on every reboot, ensuring it "sticks" to the known network.
    - **Lazy Initialization (RAM FIX):** To resolve the "WiFi Portal White Screen" caused by DRAM exhaustion, `initCamera()` and `server.begin()` are deferred. They only start *after* WiFi is stable or when an offline capture is triggered. This maximizes available heap for `WiFiManager`.
    - **Robust Pairing UI (NEW):** Added a periodic retry loop and a 5-second automatic redraw in `main.cpp`. This ensures the pairing code remains visible on the LCD until pairing is confirmed, preventing it from being cleared by other UI events.
    - **Unified Pairing State (NEW):** `display.cpp` state is now explicitly synced via `setPairingStatus()` whenever `isPaired` changes in `main.cpp`. This prevents the camera from triggering while the device technically shows an `[UNPAIRED]` status.
    - **Camera Scaling (NEW):** The JPEG scale is set to `1` (320x240) and centered both horizontally and vertically on the 240x320 display. This provides a full-width camera preview for the user.
5.  **Display Flash Prevention:**
    - `displaySleep()` is available to put the ILI9341 into a sleeping state and kill the backlight before any `ESP.restart()` call, preventing a jarring "white flash" during reboot.

### 🛠️ Hardware Stack
- **LCD (SPI2_HOST):** MOSI=35, CLK=37, MISO=36, CS=38 (Advanced SPI pins choice).
- **SD CARD (SPI3_HOST):** Uses a dedicated SPI bus to avoid bandwidth contention with the display preview.
- **CAMERA (Parallel DVP):** GPIOs 4-18. Uses PSRAM for captures.

### 📂 File Structure
- `src/main.cpp`: Orchestrates multi-tap capture button, WiFiManager, and Web Server (Port 8080).
- `src/display/`: LovyanGFX panel config and 3-zone UI layout.
- `src/cloud/`: Multipart JPEG upload logic with 30s connection timeouts.
- `src/storage/`: SD Card queue management for "Offline Capture" mode.

### ⏭️ Roadmap for the Next AI
1.  **Predictive Buffer:** Since the S3 has 8MB PSRAM, implementing a circular buffer for the camera would allow capturing the "moment before" the button was pressed.
2.  **True Sleep:** Hardware latch switch is now in place. Next step is a hardware-timed power latch circuit to achieve true 0uA sleep.
3.  **Remote Config:** Ability to update the Supabase Auth Token wirelessly via the web portal rather than reflashing.

### ⚠️ Dev Notes
- **White Screen?** Check the breadboard connections on GPIO 35 (MOSI). The ILI9341 RDDID command will return 0x00 if the SPI bus is physically disconnected.
- **Factory Reset:** Triggered by a long press of the Capture button during normal operation, then holding it for 5 seconds while the wipe UI completes to confirm. (The previous boot-time reset logic was removed).
