# ResearchMate Smart Pen - Hardware Pinout Reference

> ⚠️ **VERIFIED WORKING BUILD** — Last confirmed: 2026-03-05
> Do NOT change any pin assignments without physically re-wiring the hardware first.

## ESP32-S3-WROOM-N16R8-CAM Pin Assignments

### � TFT Display - SPI Interface (ILI9341, LovyanGFX)

| Function              | GPIO        | Notes                   |
| --------------------- | ----------- | ----------------------- |
| **MOSI** (Master Out) | **GPIO 35** | SPI Data to display     |
| **CLK** (SPI Clock)   | **GPIO 37** | SPI Clock               |
| **MISO** (Master In)  | **GPIO 36** | SPI Data from display   |
| **CS** (Chip Select)  | **GPIO 38** | Active LOW              |
| **DC** (Data/Command) | **GPIO 14** | HIGH=Data, LOW=Command  |
| **RST** (Reset)       | **GPIO 21** | Active LOW hardware RST |
| **BL** (Backlight)    | **GPIO 47** | PWM backlight control   |

**Driver:** LovyanGFX v1.2.19, `Panel_ILI9341`, `SPI2_HOST` (FSPI), 27MHz write clock  
**Panel Config:** 240×320, no offset, `invert=false`, `rgb_order=false`, `bus_shared=true`  
**Backlight:** `Light_PWM`, `pwm_channel=7`, `freq=44100`, `invert=false`

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
| **Power/Reset Button** | GPIO 1      | Input + Pull-up | Active LOW, wired to GND     |

---

### ⚠️ Reserved / Dangerous Pins

| GPIO       | Why                                    | Rule                        |
| ---------- | -------------------------------------- | --------------------------- |
| **GPIO 0** | Boot mode strapping                    | OK for button (acceptable)  |
| **GPIO 3** | **JTAG/USB strapping pin**             | **NEVER USE FOR BUTTONS**   |
| **GPIO 45** | VDD_SPI voltage strapping             | Avoid                       |
| **GPIO 46** | Boot ROM log strapping                | Avoid                       |
| **GPIO 19, 20** | USB D+/D- (used by native USB CDC) | Do not touch                |

---

## 🔌 Wiring Diagram

```
ESP32-S3-CAM                     ILI9341 TFT Display
┌──────────────┐                ┌──────────────┐
│   GPIO 35 ───┼────────────────┼─→ MOSI (SDA) │
│   GPIO 37 ───┼────────────────┼─→ SCK (CLK)  │
│   GPIO 36 ───┼────────────────┼─→ MISO       │
│   GPIO 38 ───┼────────────────┼─→ CS         │
│   GPIO 14 ───┼────────────────┼─→ DC         │
│   GPIO 21 ───┼────────────────┼─→ RST        │
│   GPIO 47 ───┼────────────────┼─→ BL         │
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
│   GPIO 1  ───┼─→ Power Button ──→ GND
```
