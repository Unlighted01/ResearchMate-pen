# ResearchMate Smart Pen - ESP32-S3 Firmware

ESP32-S3 firmware for the ResearchMate Smart Pen with camera, TFT display, and cloud integration.

---

## 🛠️ Setup Instructions for Developers

### Prerequisites

- Windows/Mac/Linux computer
- USB cable for ESP32
- WiFi connection (for downloading dependencies)
- Supabase account credentials

---

## 📥 Step 1: Install Required Software

### 1.1 Install Visual Studio Code

- Download: https://code.visualstudio.com/
- Size: ~100 MB
- Install with default settings

### 1.2 Install PlatformIO Extension

1. Open VSCode
2. Go to Extensions (Ctrl+Shift+X)
3. Search for "PlatformIO IDE"
4. Click **Install**
5. Restart VSCode when prompted

⚠️ **This will download ~500 MB - 1 GB of data when you first open the project**

### 1.3 Install USB Driver

Download the driver for your ESP32's USB chip:

- **CH340 Driver** (most common): http://www.wch-ic.com/downloads/CH341SER_EXE.html (~2 MB)
- **CP2102 Driver**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

---

## 📦 Step 2: Clone the Project

```bash
git clone https://github.com/Unlighted01/ResearchMate-pen.git
cd ResearchMate-pen
```

---

## ⚙️ Step 3: Configure Credentials

1. Copy the example config file:

   ```bash
   copy src\config.example.h src\config.h
   ```

2. Edit `src/config.h` and fill in your credentials:
   ```cpp
   #define WIFI_SSID "your-wifi-name"
   #define WIFI_PASSWORD "your-wifi-password"
   #define SUPABASE_URL "your-supabase-url"
   #define SUPABASE_ANON_KEY "your-supabase-key"
   ```

⚠️ **NEVER commit `src/config.h` to Git** - it contains sensitive credentials!

---

## 📚 Step 4: Install Dependencies (Automatic)

1. Open the project folder in VSCode
2. PlatformIO will **automatically detect** `platformio.ini`
3. It will download:
   - ESP32 platform (~400 MB)
   - Arduino framework (~200 MB)
   - Required libraries:
     - TFT_eSPI (TFT display)
     - lvgl (UI framework)
     - ArduinoJson (JSON parsing)
     - And more...

**This happens automatically!** Just wait for the download to complete.

### Manual Installation (if needed)

```bash
pio lib install
```

---

## 🚀 Step 5: Build and Upload

### Using VSCode:

1. Connect ESP32 via USB
2. Click the **PlatformIO: Upload** button (→ arrow icon in bottom toolbar)
3. Wait for compilation and upload

### Using Terminal:

```bash
# Build only
pio run

# Build and upload
pio run --target upload

# Build, upload, and open serial monitor
pio run --target upload --target monitor
```

---

## 📊 Total Download Size

| Component            | Size        | Required? |
| -------------------- | ----------- | --------- |
| VSCode               | ~100 MB     | ✅ Yes    |
| PlatformIO Extension | ~50 MB      | ✅ Yes    |
| ESP32 Platform       | ~400 MB     | ✅ Yes    |
| Arduino Framework    | ~200 MB     | ✅ Yes    |
| Libraries            | ~100 MB     | ✅ Yes    |
| USB Driver           | ~2 MB       | ✅ Yes    |
| **TOTAL**            | **~850 MB** |           |

⚠️ **Recommendation**: Use WiFi for initial setup to avoid mobile data charges.

---

## 🔧 Troubleshooting

### "Port not found" error

- Install the USB driver (CH340/CP2102)
- Check Device Manager → Ports (COM & LPT)
- Try a different USB cable

### "Upload failed" error

- Press and hold the BOOT button on ESP32
- Click Upload
- Release BOOT when "Connecting..." appears

### Libraries not installing

- Delete `.pio` folder
- Restart VSCode
- Reopen the project

---

## 📝 Project Structure

```
ResearchMate Smart pen/
├── src/
│   ├── main.cpp              # Main firmware code
│   ├── config.h              # Your credentials (DO NOT COMMIT)
│   └── config.example.h      # Template file
├── include/                  # Header files
├── lib/                      # Custom libraries
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

---

## 👥 For Team Members

**Quick Start:**

1. Install VSCode + PlatformIO
2. Clone the repo
3. Create `src/config.h` from `src/config.example.h`
4. Ask team lead for WiFi and Supabase credentials
5. Open project in VSCode (dependencies auto-install)
6. Upload to ESP32

---

## 📞 Need Help?

Contact the project lead or check the documentation in the `docs/` folder.
