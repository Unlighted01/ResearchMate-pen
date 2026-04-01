# 📦 AI Handover: ResearchMate Smart Pen (ESP32-S3)

## 🚀 Architectural Context
The ResearchMate Smart Pen is a custom hardware device built on the **ESP32-S3-WROOM-1** (N16R8) with an integrated **OV2640 camera** and **ILI9163 1.8" TFT LCD (128×160)**. It serves as the physical ingestion point for the ecosystem, capturing high-resolution "Research Scans" and piping them to the Cloud.

### 🧠 Strategic "Magic" Logic (Read Carefully)
1.  **Strapping Pin Evasion (GPIO 3):**
    - **CRITICAL:** GPIO 3 is a hardware strapping pin on the S3. If pulled low at boot, it kills the USB CDC port. **Do not use GPIO 3 for any input.**
2.  **Concurrency & Watchdogs:**
    - Firmware utilizes **FreeRTOS** tasks. All blocking waits use `vTaskDelay` to yield to the RTOS scheduler and prevent hardware watchdog resets.
    - Display updates use **LovyanGFX**, which is significantly faster than TFT_eSPI, allowing smooth camera previews while managing background SD card writes.
3.  **The Ingestion Pipeline:**
    - The pen uploads raw JPEG buffers directly to the Supabase Edge Function (`/smart-pen/index.ts`).
    - The Edge Function validates the image (JPEG magic bytes `0xFF 0xD8 0xFF`, max 10MB) before proxying to the **Vercel OCR API**.
    - On OCR failure, the item is saved with `ocr_failed: true` and `ocr_error` in the DB — the pen receives HTTP 422 instead of a silent empty result.
    - Scans are stored as `image/jpeg` in Supabase Storage (previously incorrectly stored as `image/bmp`).
    - The Vercel OCR URL is configured via Supabase secret `VERCEL_OCR_URL` (run: `supabase secrets set VERCEL_OCR_URL=https://...`).
4.  **Stability & Power Management (NEW):**
    - **WiFi Sleep:** `WiFi.setSleep(false)` is called *before* connection in `setup()` to prevent the S3 radio from entering low-power drops during the initial handshake.
    - **WiFi Persistence (NEW):** Manual `WiFi.disconnect()` was removed from `setup()` to prevent the ESP32 from clearing saved session credentials on every reboot, ensuring it "sticks" to the known network.
    - **Lazy Initialization (RAM FIX):** To resolve the "WiFi Portal White Screen" caused by DRAM exhaustion, `initCamera()` and `server.begin()` are deferred. They only start *after* WiFi is stable or when an offline capture is triggered. This maximizes available heap for `WiFiManager`.
    - **Robust Pairing UI (NEW):** Added a periodic retry loop and a 5-second automatic redraw in `main.cpp`. This ensures the pairing code remains visible on the LCD until pairing is confirmed, preventing it from being cleared by other UI events.
    - **Unified Pairing State (NEW):** `display.cpp` state is now explicitly synced via `setPairingStatus()` whenever `isPaired` changes in `main.cpp`. This prevents the camera from triggering while the device technically shows an `[UNPAIRED]` status.
    - **Camera Scaling:** Camera outputs 320x240 JPEG, scaled 1/4 (80×60) and centered in the 128×120 content zone on the 128×160 display.
5.  **Display Flash Prevention:**
    - `displaySleep()` is available to put the ILI9163 into a sleeping state and kill the backlight before any `ESP.restart()` call, preventing a jarring "white flash" during reboot.
6.  **Display Clipping Bug & Endianness:**
    - **BUG:** Previously, negative `x` coordinates during JPEG decode callback caused a `drawW` underflow (wrapping to 65k) which drew an enormous amount of garbage pixels per row, resulting in "rainbow colors" on the display.
    - **FIX:** Replaced the complex bounds checking in `display.cpp`'s `tft_output` callback. Now, we just skip the block if it starts left of the screen (`if (x < 0) return 1;`), and let LovyanGFX automatically handle clipping on the right edge.
    - **IMPORTANT (Colors):** When pushing the decoded JPEG to the TFT via LovyanGFX, `.setSwapBytes(false)` is used alongside an explicit `(lgfx::swap565_t*)bitmap` cast inside the `tft.pushImage(...)` callback. This is required because `TJpgDec` outputs big-endian pixel data; casting to `rgb565_t` will scramble the Red and Blue channels.

### 🛠️ Hardware Stack
- **LCD (SPI2_HOST):** ILI9163 1.8" 128×160, MOSI=35, CLK=37, CS=38, DC=14, RST=21, BL=47. No touch controller.
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
- **White Screen?** Check GPIO 35 (MOSI) connection. Also note: only the ILI9163 driver works on this panel — ST7735/ST7789/ILI9341 drivers all produce a white screen due to incompatible init sequences.
- **Factory Reset:** Triggered by a long press of the Capture button during normal operation, then holding it for 5 seconds while the wipe UI completes to confirm. (The previous boot-time reset logic was removed).
