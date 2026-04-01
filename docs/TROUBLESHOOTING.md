# ResearchMate Smart Pen — Troubleshooting Guide

---

## OCR Issues

### Scan saves but text is empty / OCR shows no result

**Cause:** OCR was silently failing — the item was saved with `ocr_text: ""` and no error surfaced.

**Fix (applied March 2026):** The edge function now sets `ocr_failed: true` and `ocr_error` on the DB item, and returns HTTP 422 to the pen. Check the scan in the dashboard — if `ocr_failed` is true, the error message will tell you which provider failed and why.

**Common root causes:**
- All three OCR providers (OpenRouter, Gemini, Claude) have exhausted their API keys or rate limits
- The Vercel function cold-started and timed out (30s limit) — retry the scan
- `VERCEL_OCR_URL` secret is missing or wrong in Supabase — run:
  ```
  supabase secrets set VERCEL_OCR_URL=https://research-mate-website.vercel.app/api/ocr
  ```

---

### Upload rejected with "Invalid image format"

**Cause:** The pen sent a non-JPEG payload (corrupt frame buffer, partial capture, or wrong pixel format).

**Fix:** The edge function now validates JPEG magic bytes (`0xFF 0xD8 0xFF`) and rejects invalid images with HTTP 400. On the firmware side, check `captureImage()` in `camera.cpp` — if `esp_camera_fb_get()` returns NULL, the upload should be aborted before sending.

---

### Upload rejected with "Image too large"

**Cause:** Captured frame exceeded 10MB (unlikely at UXGA/Q12 but possible if quality was changed).

**Fix:** Check `setImageQuality()` call in your capture flow. UXGA at quality 12 produces ~150–400KB per frame. If you're seeing 10MB+, the resolution or quality setting has been changed unexpectedly.

---

### Confidence score seems too low

**Cause (old):** Confidence was purely word-count based — short but clean scans scored as low as 65%.

**Fix (applied March 2026):** Confidence is now calculated as:
- **Base score** by provider: OpenRouter 82%, Gemini 80%, Claude 78%
- **+Length bonus:** up to +15% for 200+ words
- **−Noise penalty:** up to −15% if the text has high non-standard character ratio (garbled scan)

If confidence is still low, the scan image itself may be blurry or underexposed.

---

### Scans stored as .bmp in Supabase but pen sends JPEG

**Cause (old):** The edge function was uploading to storage with `contentType: "image/bmp"` regardless of actual format.

**Fix (applied March 2026):** Storage now uses `contentType: "image/jpeg"` with filename `scan_<uuid>.jpg`. Old `.bmp` entries in storage are safe to leave or clean up manually.

---

## Camera / Hardware Issues

### Camera init failed (ESP32 serial log)

- Check that PSRAM is detected: look for `[Camera] No PSRAM detected!` in serial output
- If no PSRAM: resolution falls back to QVGA (320x240) automatically
- If camera still fails: check SDA/SCL pins (GPIO 4/5) for loose connections

### Image is upside-down

- `set_vflip(s, 1)` corrects this — already applied in `camera.cpp`
- If image is mirrored: `set_hmirror(s, 0)` — already set, check if it was accidentally changed

### White screen on TFT display

- Check GPIO 35 (MOSI) connection — a loose wire here prevents any commands reaching the panel
- **Driver matters:** only `Panel_ILI9163` works on this display. Using ST7735, ST7789, or ILI9341 drivers all produce a white screen due to incompatible init/gamma sequences
- Display is 1.8" ILI9163, 128×160 portrait, `setRotation(0)`, `invert=false`, `rgb_order=false`
- See `handover_prompt.md` for full hardware notes

### WiFi portal white screen / can't connect

- **Cause:** DRAM exhaustion before WiFiManager portal starts
- **Fix:** Camera lazy-init is already in place — `initCamera()` only runs after WiFi is stable
- If still failing: check heap with `ESP.getFreeHeap()` in serial monitor before WiFiManager starts

---

## General

### Scan saved but not appearing in dashboard

1. Check `user_id` — if `auth_token` lookup fails, item is saved without a user and won't appear in the user's feed
2. Check DB directly: look for items with `device_source = 'smart_pen'` and `user_id IS NULL`
3. Re-pair the pen to regenerate a valid `auth_token`

### Factory reset

Long-press the Capture button during normal operation, then hold for 5 seconds while the wipe UI completes. This clears the paired token — you'll need to re-pair via the dashboard.
