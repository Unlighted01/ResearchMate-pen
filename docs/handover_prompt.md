# 📦 Project Handover: ResearchMate Smart Pen (ESP32-S3)

## 🚀 Current Status: STABLE & WORKING

The firmware is fully functional. The display, camera, capture buttons, power/reset simulation, factory wipe, cloud upload, and SD card offline queue are all working.

### ✅ Key Features Working:

1.  **Camera:** OV2640 initializes and captures images correctly. Falls back to internal SRAM when PSRAM is not detected.
2.  **Display:** LCD initialization is driven by **LovyanGFX** (`Panel_ILI9341`, `SPI2_HOST`). The display shows a boot splash, status bars, camera preview, and pairing UI.
3.  **Multi-Function Capture Button (GPIO 2):**
    - **Single Press:** Captures and saves to SD Card (normal capture).
    - **Double Press:** Captures and uploads to Supabase Cloud.
    - **Long Press (>800ms):** Syncs SD Card queue to Cloud.
4.  **Simulated Power/Reset Button (GPIO 1):**
    - On boot, the device displays a "ResearchMate" logo and waits for a power button press to continue booting.
    - **Hold 5 seconds on boot:** Triggers factory reset (wipes WiFi, auth token, SD queue).
    - The factory reset has a full on-screen UI with progress bar, confirmation, and cancellation feedback.
    - After wipe, `displaySleep()` is called before `ESP.restart()` to prevent white screen flash.
5.  **Web Interface:** Serves pairing codes and handles manual triggers (Port 8080).
6.  **Cloud:** Uploads large multipart image binaries to Supabase. HTTPClient timeout extended to 30s for cold-start Edge Functions.
7.  **Offline SD Card Queuing:** Writes JPEG buffers to `/queue/scan_...jpg` on the SD card when offline.

### 🛠️ Hardware Configuration (CRITICAL)

- **Board:** ESP32-S3-WROOM-N16R8-CAM (8MB Flash)
- **Display Driver:** LovyanGFX with `Panel_ILI9341` on `SPI2_HOST` (FSPI)
- **Display Pins:** MOSI=35, CLK=37, MISO=36, CS=38, DC=14, RST=21, BL=47
- **SD Card SPI Pins:** CS=42, MOSI=41, MISO=40, SCK=39
- **Camera:** OV2640 on DVP parallel (GPIO 4-18, see `camera/camera.cpp`)
- **RGB LED:** WS2812B on GPIO 48
- **Capture Button:** GPIO 2 (INPUT_PULLUP, active LOW)
- **Power/Reset Button:** GPIO 1 (INPUT_PULLUP, active LOW)

> ⚠️ **GPIO 3 is a STRAPPING PIN** on ESP32-S3! Never use it for buttons or general I/O. It controls USB/JTAG routing at boot and will lock the ESP32 into Download Mode, disabling the Native USB CDC serial port (COM port disappears).

### 🧩 Recent Fixes & Changes

- **Strapping Pin Crash (GPIO 3 → GPIO 1):** The Power Button was originally assigned to GPIO 3, which is a hardware strapping pin. This caused the ESP32 to lock into USB Download Mode on boot, killing the COM port and preventing firmware execution. Moved to GPIO 1.
- **White Screen on Boot (Loose SPI Wires):** The ILI9341 LCD showed a persistent white screen. Diagnosed via raw SPI chip ID probe (RDDID command 0x04). Both SPI2_HOST and SPI3_HOST returned all zeros, confirming the SPI bus couldn't physically reach the display. Root cause was loose breadboard wires on GPIO 35/37/36. Resolved by rewiring.
- **Factory Reset UI:** Added dedicated on-screen wipe UI functions (`displayWipeStart`, `displayWipeProgress`, `displayWipeCancelled`, `displayWipeComplete`) in `display.cpp`.
- **Display Sleep Before Restart:** Added `displaySleep()` function that turns off the backlight and puts the TFT chip into sleep mode before `ESP.restart()` to prevent white screen flash after factory reset.
- **RTOS Watchdog Yields:** The simulated "off" state `while` loop uses `vTaskDelay(pdMS_TO_TICKS(10))` instead of `delay(10)` to prevent the ESP32 dual-core watchdog from killing the boot sequence.

### ⚙️ How `platformio.ini` Works

- **`platform = espressif32@6.5.0`:** Locks the SDK to a stable, tested version.
- **`-DARDUINO_USB_MODE=1` + `-DARDUINO_USB_CDC_ON_BOOT=1`:** Enables Native USB CDC for Serial output (no UART chip needed).
- **`-DUSER_SETUP_LOADED=1`:** Legacy flag, harmless.
- **`board_build.arduino.memory_type = qio_qspi`:** Flash memory mode. Do NOT change to `dio_qspi` or `qio_opi` without testing — some modes may conflict with GPIO 35-37 (display SPI pins).
- **`board_build.partitions = huge_app.csv`:** Uses a large app partition to fit the firmware + camera + display libraries.

### 📂 Code Structure

- `src/main.cpp`: Main logic, Web Server routing, Multi-Tap Button logic, Power Button logic, Factory Reset, and the simulated Off state.
- `src/config.h`: Centralized pin definitions and device settings.
- `src/camera/`: Camera HAL driver (OV2640 DVP parallel, falls back to DRAM if PSRAM missing).
- `src/display/`: LovyanGFX display driver with ILI9341 panel, 3-zone UI (top bar, viewfinder, bottom panel), QR code display, and factory reset UI.
- `src/cloud/`: Supabase Authentication & multipart HTTP uploads (30s timeout).
- `src/storage/`: SD Card SPI format and offline Queue abstraction.

### 🔧 Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| White screen, Serial works | Loose SPI wire (MOSI/CLK/MISO/CS) | Reseat breadboard wires on GPIO 35, 37, 36, 38 |
| White screen, no Serial, blue LED | ESP32 stuck in Download Mode | Don't use GPIO 3 for buttons. Press RST without holding BOOT. |
| COM port disappears | Infinite boot loop or strapping pin conflict | Hold BOOT, press RST, release BOOT. Reflash. |
| `[Camera] No PSRAM detected!` | Board has no external PSRAM | Normal — camera falls back to internal SRAM |
| Watchdog reset during "Off" state | `delay()` doesn't yield to RTOS | Use `vTaskDelay(pdMS_TO_TICKS(ms))` instead |

### ⏭️ Next Steps

- **Auto-Summarization Control:** The Supabase Edge Function currently auto-summarizes captured images. Add a flag to let the user choose when to summarize.
- **Physical Power Latch:** When a TP module, boost converter, and battery are added, replace the simulated power button with a real hardware latch circuit.
- **WebServer Accessibility:** The web server on Port 8080 is sometimes unreachable. Investigate AP isolation and modem sleep settings.
