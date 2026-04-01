# ✅ WORKING BUILD REFERENCE — DO NOT MODIFY WITHOUT TESTING

> **Last Verified:** 2026-03-05  
> **Status:** All hardware functional (display, camera, SD, buttons, LED, WiFi, cloud)  
> **Rule:** If the display shows white after a code change, check wiring FIRST.

---

## Platform & Toolchain

| Setting | Value |
|---------|-------|
| PlatformIO Platform | `espressif32@6.5.0` |
| Board | `esp32-s3-devkitc-1` |
| Framework | `arduino` |
| Arduino Core | `3.20014.231204 (2.0.14)` |
| MCU | `esp32s3` @ 240MHz |
| Flash | 8MB QD |
| Partitions | `huge_app.csv` |
| Memory Type | `qio_qspi` |
| USB Mode | Native CDC (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`) |

---

## Libraries (Exact Versions)

| Library | Version | Purpose |
|---------|---------|---------|
| **LovyanGFX** | 1.2.19 | TFT display driver |
| **TJpg_Decoder** | 1.1.0 | JPEG decoding for camera preview |
| **esp32-camera** | ^2.0.4 | OV2640 camera HAL |
| **Adafruit NeoPixel** | 1.15.2 | WS2812B RGB LED |
| **ArduinoJson** | 7.4.2 | JSON parsing |
| **WiFiManager** | 2.0.17 | Captive portal WiFi setup |
| **QRCode** | 0.0.1 | QR code generation |

---

## Display Configuration (LovyanGFX)

> **Display:** 1.8" ILI9163 TFT, 128x160 portrait. Confirmed working 2026-04-01.

```cpp
// Driver: Panel_ILI9163 on SPI2_HOST (FSPI) — 1.8" 128x160
// ILI9163 shares command opcodes with ILI9341 but has correct gamma for 128x160.
// ST7735, ST7789, and ILI9341 drivers all produce a white screen on this panel.
cfg.spi_host    = SPI2_HOST;
cfg.spi_mode    = 0;
cfg.freq_write  = 27000000;   // 27MHz
cfg.freq_read   = 16000000;
cfg.spi_3wire   = false;
cfg.dma_channel = SPI_DMA_CH_AUTO;

cfg.pin_sclk = 37;  // TFT_CLK
cfg.pin_mosi = 35;  // TFT_MOSI
cfg.pin_miso = 36;  // TFT_MISO (readable=true)
cfg.pin_dc   = 14;  // TFT_DC

cfg.pin_cs   = 38;  // TFT_CS
cfg.pin_rst  = 21;  // TFT_RST
cfg.panel_width  = 128;
cfg.panel_height = 160;
cfg.offset_x = 2;   // ILI9163 GRAM is 132 wide, center 128px panel
cfg.offset_y = 1;   // ILI9163 GRAM is 162 tall, center 160px panel
cfg.invert   = false;
cfg.rgb_order = false;
cfg.bus_shared = true;

// Backlight: Light_PWM
cfg.pin_bl      = 47;  // TFT_BL
cfg.invert      = false;
cfg.freq        = 44100;
cfg.pwm_channel = 7;
```

---

## Pin Assignment Summary

```
TFT Display:  ILI9163 128x160, MOSI=35, CLK=37, MISO=36, CS=38, DC=14, RST=21, BL=47
SD Card:      CS=42, MOSI=41, MISO=40, SCK=39
Camera:       DVP parallel on GPIO 4-18 (hardwired on module)
LED:          GPIO 48 (WS2812B)
Capture Btn:  GPIO 2 (INPUT_PULLUP → GND)
Power:        Hardware latch switch on power line (GPIO 1 unused)
```

---

## Build Flags (platformio.ini)

```ini
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DUSER_SETUP_LOADED=1

board_build.arduino.memory_type = qio_qspi
```

---

## Known Gotchas

1. **GPIO 3 = DEATH.** It's a strapping pin that controls JTAG/USB routing. Using it for a button locks the ESP32 into Download Mode — COM port vanishes, firmware never runs.
2. **White screen = check SPI wires.** If RDDID returns `00 00 00`, the SPI data lines (MOSI/CLK/MISO) are physically disconnected. Reseat breadboard jumpers.
3. **`delay()` in long loops = watchdog crash.** Use `vTaskDelay(pdMS_TO_TICKS(ms))` instead of `delay()` in any blocking `while` loop during `setup()`.
4. **`memory_type = qio_qspi` may conflict with GPIO 35-37** on some ESP32-S3 variants with Octal PSRAM. Our board has no PSRAM, so it works fine. Do not change without testing.
5. **`tft.init()` must run early** in `setup()` — do not add long delays before it. ILI9163 has strict power-on timing requirements.
6. **ILI9163 pixel offset:** The ILI9163 GRAM is 132x162, panel is 128x160. Use `offset_x=2, offset_y=1` to center the visible area. Without offsets, screen content is shifted 2px right and 1px down.
