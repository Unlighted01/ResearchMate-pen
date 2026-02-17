# ResearchMate Smart Pen - Hardware Pinout Reference

## ESP32-S3-WROOM-N16R8-CAM Pin Assignments

### 📷 Camera Interface (OV5640) - Built-in DVP Parallel

| Function                  | GPIO    | Notes                |
| ------------------------- | ------- | -------------------- |
| **XCLK** (Master Clock)   | GPIO 15 | 20MHz camera clock   |
| **SIOD** (I2C SDA)        | GPIO 4  | Camera control data  |
| **SIOC** (I2C SCL)        | GPIO 5  | Camera control clock |
| **Y9** (Data bit 7)       | GPIO 16 | MSB parallel data    |
| **Y8** (Data bit 6)       | GPIO 17 |                      |
| **Y7** (Data bit 5)       | GPIO 18 |                      |
| **Y6** (Data bit 4)       | GPIO 12 |                      |
| **Y5** (Data bit 3)       | GPIO 10 |                      |
| **Y4** (Data bit 2)       | GPIO 8  |                      |
| **Y3** (Data bit 1)       | GPIO 9  |                      |
| **Y2** (Data bit 0)       | GPIO 11 | LSB parallel data    |
| **VSYNC** (Vertical Sync) | GPIO 6  | Frame sync signal    |
| **HREF** (Horizontal Ref) | GPIO 7  | Line reference       |
| **PCLK** (Pixel Clock)    | GPIO 13 | Pixel clock signal   |

> **DO NOT USE** these GPIOs for other peripherals - they are hardwired to the camera module!

---

### 🖥️ TFT Display - SPI Interface (Conflict-Free)

| Function              | GPIO        | Original (Conflicted) | Status         |
| --------------------- | ----------- | --------------------- | -------------- |
| **MOSI** (Master Out) | **GPIO 35** | ~~GPIO 11~~           | ✅ Fixed       |
| **CLK** (SPI Clock)   | **GPIO 36** | ~~GPIO 13~~           | ✅ Fixed       |
| **MISO** (Master In)  | **GPIO 37** | ~~GPIO 12~~           | ✅ Fixed       |
| **CS** (Chip Select)  | **GPIO 38** | ~~GPIO 10~~           | ✅ Fixed       |
| **DC** (Data/Command) | **GPIO 14** | GPIO 14               | ✅ No conflict |
| **RST** (Reset)       | **GPIO 21** | ~~GPIO 9~~            | ✅ Fixed       |
| **BL** (Backlight)    | **GPIO 20** | ~~GPIO 8~~            | ✅ Fixed       |

**Update your `config.h` with:**

```cpp
// TFT Display - Conflict-free pin assignment
#define TFT_MOSI 35
#define TFT_CLK  36
#define TFT_MISO 37
#define TFT_CS   38
#define TFT_DC   14  // No change
#define TFT_RST  21
#define TFT_BL   20
```

---

### 💡 Status LED & Buttons

| Function           | GPIO    | Type            | Notes            |
| ------------------ | ------- | --------------- | ---------------- |
| **RGB LED**        | GPIO 48 | WS2812B         | Status indicator |
| **Capture Button** | GPIO 0  | Input + Pull-up | Active LOW       |

**Additional Button Suggestions:**

- **Menu/Mode Button**: GPIO 1 or GPIO 2
- **Power Button**: Use EN pin with RC circuit for hardware power control

---

### 🔋 Power Pins

| Pin     | Voltage | Function                                               |
| ------- | ------- | ------------------------------------------------------ |
| **5V**  | 5.0V    | USB-C input / TP4056 output                            |
| **3V3** | 3.3V    | Internal LDO regulator output                          |
| **GND** | 0V      | Common ground                                          |
| **BAT** | 3.7V    | Direct battery connection (if using built-in charging) |

---

### ⚠️ Reserved / Strapping Pins (Use with Caution)

| GPIO            | Function      | Recommendation                  |
| --------------- | ------------- | ------------------------------- |
| **GPIO 0**      | Boot mode     | ✅ Used for button (acceptable) |
| **GPIO 3**      | JTAG          | ⚠️ Avoid unless debugging       |
| **GPIO 45**     | Strapping pin | ⚠️ Use pull-up/down only        |
| **GPIO 46**     | Strapping pin | ⚠️ Use pull-up/down only        |
| **GPIO 19, 20** | USB D+/D-     | ⚠️ OK if not using native USB   |

---

### ✅ Available GPIO Pins for Future Expansion

These pins are free and safe to use:

| GPIO Range | Quantity | Notes                             |
| ---------- | -------- | --------------------------------- |
| GPIO 1-3   | 3 pins   | Safe if using USB CDC (not UART)  |
| GPIO 19-21 | 3 pins   | ✅ Currently using 20, 21 for TFT |
| GPIO 33-42 | 10 pins  | ✅ Best for general purpose I/O   |
| GPIO 43-44 | 2 pins   | UART TX/RX, safe if using USB CDC |
| GPIO 47    | 1 pin    | Safe general purpose              |

**Usage Ideas:**

- SD card reader (SPI or SDIO)
- Additional buttons/switches
- External sensors (I2C/SPI)
- Buzzer/speaker (PWM)
- Battery voltage monitoring (ADC)

---

## 🔌 Wiring Diagram

```
ESP32-S3-CAM                     1.47" TFT Display
┌──────────────┐                ┌──────────────┐
│              │                │              │
│   GPIO 35 ───┼────────────────┼─→ MOSI (SDA) │
│   GPIO 36 ───┼────────────────┼─→ SCK (CLK)  │
│   GPIO 37 ───┼────────────────┼─→ MISO       │
│   GPIO 38 ───┼────────────────┼─→ CS         │
│   GPIO 14 ───┼────────────────┼─→ DC         │
│   GPIO 21 ───┼────────────────┼─→ RST        │
│   GPIO 20 ───┼────────────────┼─→ BL         │
│              │                │              │
│   3.3V ──────┼────────────────┼─→ VCC        │
│   GND ───────┼────────────────┼─→ GND        │
│              │                │              │
└──────────────┘                └──────────────┘

ESP32-S3-CAM                     WS2812B LED
│   GPIO 48 ───┼────────────────┼─→ DIN        │
│   3.3V ──────┼────────────────┼─→ VCC        │
│   GND ───────┼────────────────┼─→ GND        │

ESP32-S3-CAM                     Button
│   GPIO 0 ────┼────┐            │
│              │    └─[Button]─GND
│              │    (Internal pull-up enabled)
```

---

## 📝 Configuration Update Checklist

- [ ] Update `src/config.h` with new TFT pin definitions
- [ ] Verify TFT library SPI pin configuration (TFT_eSPI or Adafruit GFX)
- [ ] Test display initialization with new pins
- [ ] Confirm camera still works after pin changes
- [ ] Update hardware documentation/schematics

---

## 🎓 For Thesis Documentation

Include this pinout table in your:

- **Hardware Design section** - Complete pin mapping
- **Assembly Instructions** - Wiring diagram reference
- **Troubleshooting Guide** - Pin conflict resolution example
