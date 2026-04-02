# ResearchMate Smart Pen - Hardware Pinout Reference

> ⚠️ **VERIFIED WORKING BUILD** — Last confirmed: 2026-04-02
> Do NOT change any pin assignments without physically re-wiring the hardware first.

## ESP32-S3-WROOM-N8R16 Pin Assignments

### TFT Display - SPI Interface

> **Display:** 1.8" ILI9163 TFT, 128x160 portrait. Confirmed working 2026-04-02.
> **Pin Migration (2026-04-02):** MOSI moved from GPIO 35→45, CLK from GPIO 37→46, MISO disabled (-1). This frees GPIO 33-37 for Octal PSRAM (16MB), enabling UXGA camera captures.

| Function              | GPIO        | Notes                                    |
| --------------------- | ----------- | ---------------------------------------- |
| **MOSI** (Master Out) | **GPIO 45** | SPI Data — mapped to `SDA` on ILI9163 PCB. Moved from GPIO 35 (PSRAM D5 conflict) |
| **CLK** (SPI Clock)   | **GPIO 46** | SPI Clock. Moved from GPIO 37 (PSRAM D7 conflict) |
| **MISO**              | **-1**      | Disabled — ILI9163 is write-only. GPIO 36 freed for PSRAM D6 |
| **CS** (Chip Select)  | **GPIO 38** | Active LOW                               |
| **DC** (Data/Command) | **GPIO 14** | HIGH=Data, LOW=Command — labeled `AO` on ILI9163 PCB |
| **RST** (Reset)       | **GPIO 21** | Active LOW hardware RST                  |
| **BL** (Backlight)    | **GPIO 47** | PWM backlight control — labeled `LED` on ILI9163 PCB |

**Driver:** LovyanGFX v1, `Panel_ILI9163`, `SPI2_HOST` (FSPI), 27MHz write clock
**Panel Config:** 128x160, `offset_x=2`, `offset_y=1`, `invert=false`, `rgb_order=false`, `bus_shared=true`
**Backlight:** `Light_PWM`, `pwm_channel=7`, `freq=44100`, `invert=false`

**Previous displays (both broken/replaced):** 2.4" ILI9341 240x320, then briefly ST7735 attempt (white screen). Only ILI9163 works.

---

### 📷 Camera Interface (OV2640) - Built-in DVP Parallel

| Function                  | GPIO    |
| ------------------------- | ------- |
| **XCLK** (Master Clock)   | GPIO 15 |
| **SIOD** (I2C SDA)        | GPIO 4  |
| **SIOC** (I2C SCL)        | GPIO 5  |
| **Y9** (Data bit 7)       | GPIO 16 |
| **Y8** (Data bit 6)       | GPIO 17 |
| **Y7** (Data bit 5)       | GPIO 18 |
| **Y6** (Data bit 4)       | GPIO 12 |
| **Y5** (Data bit 3)       | GPIO 10 |
| **Y4** (Data bit 2)       | GPIO 8  |
| **Y3** (Data bit 1)       | GPIO 9  |
| **Y2** (Data bit 0)       | GPIO 11 |
| **VSYNC** (Vertical Sync) | GPIO 6  |
| **HREF** (Horizontal Ref) | GPIO 7  |
| **PCLK** (Pixel Clock)    | GPIO 13 |

> These are hardwired on the camera module — do NOT reassign.

---

### � SD Card Reader - SPI Interface

| Function | GPIO        |
| -------- | ----------- |
| **CS**   | **GPIO 42** |
| **MOSI** | **GPIO 41** |
| **MISO** | **GPIO 40** |
| **SCK**  | **GPIO 39** |

---

### 💡 Status LED & Buttons

| Function               | GPIO        | Type            | Notes                        |
| ---------------------- | ----------- | --------------- | ---------------------------- |
| **RGB LED**            | GPIO 48     | WS2812B         | NeoPixel status indicator    |
| **Capture Button**     | GPIO 2      | Input + Pull-up | Active LOW, wired to GND     |

---

### ⚠️ Reserved / Dangerous Pins

| GPIO       | Why                                    | Rule                        |
| ---------- | -------------------------------------- | --------------------------- |
| **GPIO 0** | Boot mode strapping                    | OK for button (acceptable)  |
| **GPIO 3** | **JTAG/USB strapping pin**             | **NEVER USE FOR BUTTONS**   |
| **GPIO 33-37** | **Octal PSRAM data bus (D4-D7, DQS)** | **Reserved for PSRAM — do NOT wire anything here** |
| **GPIO 19, 20** | USB D+/D- (used by native USB CDC) | Do not touch                |

> **Note on GPIO 45/46:** These are strapping pins but safe to use for SPI after boot. The display SPI initializes after boot completes, so strapping state doesn't matter. Verified working 2026-04-02.

---

## 🔌 Wiring Diagram

```
ESP32-S3-WROOM (N8R16)           ILI9163 1.8" TFT (128x160)
┌──────────────┐                ┌──────────────┐
│   GPIO 45 ───┼────────────────┼─→ SDA (MOSI) │
│   GPIO 46 ───┼────────────────┼─→ SCK        │
│   (no wire)  │                │  (no MISO)   │
│   GPIO 38 ───┼────────────────┼─→ CS         │
│   GPIO 14 ───┼────────────────┼─→ AO (DC)    │
│   GPIO 21 ───┼────────────────┼─→ RESET       │
│   GPIO 47 ───┼────────────────┼─→ LED (BL)   │
│   3.3V ──────┼────────────────┼─→ VCC        │
│   GND ───────┼────────────────┼─→ GND        │
└──────────────┘                └──────────────┘

ESP32-S3-CAM                     SD Card Reader
│   GPIO 42 ───┼────────────────┼─→ CS         │
│   GPIO 41 ───┼────────────────┼─→ MOSI       │
│   GPIO 40 ───┼────────────────┼─→ MISO       │
│   GPIO 39 ───┼────────────────┼─→ SCK        │
│   3.3V ──────┼────────────────┼─→ VCC        │
│   GND ───────┼────────────────┼─→ GND        │

ESP32-S3-CAM                     Components
│   GPIO 48 ───┼─→ WS2812B DIN
│   GPIO 2  ───┼─→ Capture Button ─→ GND
│   GPIO 1  ───┼─→ (unused — power controlled by hardware latch switch)
```
