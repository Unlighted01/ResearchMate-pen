// ============================================
// ResearchMate Smart Pen - Full Integration
// Camera + TFT Display + Cloud Pairing
// ============================================

#include "camera/camera.h"
#include "cloud/cloud.h"
#include "config.h"
#include "display/display.h"
#include "storage/storage.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

WebServer server(8080);
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// Global state
static char lastCapturedFilename[64] = {0};
static char pairingCode[16] = {0};
static bool isPaired = false;
static bool livePreviewActive = false; // Add live preview state
static unsigned long lastPairingCheck = 0;
static const unsigned long PAIRING_CHECK_INTERVAL = 5000;

// Camera debug state
static int totalItemsUploaded = 0;
static unsigned long lastCaptureTimestamp = 0;
static char lastCaptureStatus[32] = "Ready";

// Button state tracking (polling)
static bool lastCaptureButtonState = HIGH; // Default high (INPUT_PULLUP)
static unsigned long captureButtonPressStart = 0;

// static bool lastPowerButtonState = HIGH; // Dedicated power button
// static unsigned long powerButtonPressStart = 0;

// ============================================
// Web Handlers (from original project)
// ============================================

void handleRoot() {
  String pairingStatus =
      isPaired
          ? "<span style='color: #00ff41;'>[OK] Paired</span>"
          : "<span style='color: #ffd700;'>[!] Waiting for pairing...</span>";

  String pairingCodeHtml = "";
  if (!isPaired && pairingCode[0]) {
    pairingCodeHtml =
        String("<div style='background: #0f3460; padding: 20px; border: 2px "
               "solid #ffd700; margin: 20px 0; border-radius: 10px;'>") +
        "<p style='text-align: center; color: #999; font-size: 12px;'>PAIRING "
        "CODE</p>" +
        "<p id='pairingCode' style='text-align: center; font-size: 32px; "
        "font-weight: bold; color: #ffd700; letter-spacing: 10px;'>" +
        pairingCode + "</p>" +
        "<p style='text-align: center; color: #999; font-size: 11px;'>Enter "
        "this code on your ResearchMate website</p>" +
        "<button class='btn-dark' style='width: 100%; margin-top: 15px;' "
        "onclick='refreshCode()'>REFRESH CODE</button>" +
        "</div>";
  }

  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ResearchMate Smart Pen</title>";
  html +=
      "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: #1a1a2e; color: "
          "#eee; margin: 0; padding: 20px; }";
  html += ".container { background: #16213e; padding: 30px; border-radius: "
          "15px; max-width: 700px; margin: 0 auto; }";
  html += "h1 { color: #00d4ff; text-align: center; }";
  html += ".subtitle { text-align: center; color: #999; font-size: 12px; }";
  html += "#camera { width: 100%; border: 2px solid #00d4ff; border-radius: "
          "10px; margin: 20px 0; background: #000; }";
  html += ".buttons { display: grid; grid-template-columns: 1fr 1fr; gap: "
          "10px; margin: 20px 0; }";
  html += "button { padding: 12px; font-weight: bold; cursor: pointer; border: "
          "none; border-radius: 5px; font-size: 14px; }";
  html += ".btn-cyan { background: #00d4ff; color: #000; }";
  html += ".btn-green { background: #00ff41; color: #000; }";
  html += ".btn-dark { background: #0f3460; color: #fff; border: 2px solid "
          "#00d4ff; }";
  html += ".btn-red { background: #ff6b6b; color: #fff; }";
  html += ".btn-orange { background: #ffd700; color: #000; }";
  html +=
      ".btn-purple { background: #9d4edd; color: #fff; grid-column: 1 / -1; }";
  html +=
      ".status { background: #0f3460; padding: 15px; border-left: 4px solid "
      "#00ff41; border-radius: 5px; margin: 20px 0; text-align: center; }";
  html += ".status.unpaired { border-left: 4px solid #ffd700; }";
  html += "#uploadStatus { margin-top: 20px; padding: 15px; border-radius: "
          "5px; background: #0f3460; color: #999; text-align: center; "
          "font-size: 12px; display: none; }";
  html += "</style></head><body>";

  html += "<div class='container'>";
  html += "<h1>ResearchMate Smart Pen</h1>";
  html += "<p class='subtitle'>OV2640 Camera + TFT Display</p>";

  if (isPaired) {
    html += "<div class='status'>[OK] Connected - " + pairingStatus +
            " <button class='btn-dark' style='padding: 5px 15px; margin-left: "
            "10px; font-size: 11px;' onclick='unpair()'>UNPAIR</button></div>";
  } else {
    html +=
        "<div class='status unpaired'>[PAIRING] " + pairingStatus + "</div>";
  }

  html += pairingCodeHtml;
  html += "<img id='camera' src='/capture' alt='Camera Feed'>";

  html += "<div class='buttons'>";
  html +=
      "<button class='btn-cyan' onclick='capture()'>CAPTURE PREVIEW</button>";
  html += "<button class='btn-purple' id='uploadBtn' onclick='upload()'>UPLOAD "
          "TO CLOUD</button>";
  html += "</div>";

  html += "<div id='uploadStatus'></div>";

  // --- [TO BE REMOVED LATER] VIRTUAL LCD START ---
  html += "<div id='virtualLcd' style='background: #111; border: 2px solid "
          "#333; padding: 15px; border-radius: 8px; font-family: monospace; "
          "color: #00d4ff; margin-top: 20px;'>";
  html += "<div>STATUS: <span id='lcdStatus' style='font-weight:bold; color: "
          "#fff;'>Ready</span></div>";
  html += "<div>UPLOADS: <span id='lcdCount'>0</span></div>";
  html += "<div>LAST CAPTURE: <span id='lcdTime'>Never</span></div>";
  html += "</div>";
  // --- [TO BE REMOVED LATER] VIRTUAL LCD END ---

  html += "<script>";
  html += "var isPaired = " + String(isPaired ? "true" : "false") + ";";

  html += "function capture() { document.getElementById('camera').src = "
          "'/capture?t=' + Date.now(); }";

  html += "function upload() {";
  html +=
      "  if (!isPaired) { showStatus('Not paired yet!', 'error'); return; }";
  html += "  document.getElementById('uploadBtn').disabled = true;";
  html += "  showStatus('Uploading...', 'pending');";
  html += "  fetch('/api/upload', { method: 'POST' })";
  html += "    .then(r => r.json())";
  html += "    .then(data => {";
  html +=
      "      if (data.success) { showStatus('[OK] Uploaded!', 'success'); }";
  html += "      else { showStatus('[X] Failed: ' + data.error, 'error'); }";
  html += "      document.getElementById('uploadBtn').disabled = false;";
  html += "    });";
  html += "}";

  html += "function showStatus(msg, type) {";
  html += "  var el = document.getElementById('uploadStatus');";
  html += "  el.textContent = msg;";
  html += "  el.style.display = 'block';";
  html += "  setTimeout(() => { el.style.display = 'none'; }, 5000);";
  html += "}";

  html += "function checkPairing() { if (!isPaired) { "
          "fetch('/api/pairing-status').then(r => r.json()).then(data => { if "
          "(data.paired) { location.reload(); } }); } }";
  html +=
      "function refreshCode() { fetch('/api/pairing-start', { method: 'POST' "
      "}).then(r => r.json()).then(data => { if (data.success) { "
      "document.getElementById('pairingCode').textContent = data.code; } }); }";
  html +=
      "function unpair() { if (!confirm('Unpair?')) return; "
      "fetch('/api/unpair', { method: 'POST' }).then(r => r.json()).then(data "
      "=> { if (data.success) { location.reload(); } }); }";

  html += "capture();";
  html += "if (!isPaired) setInterval(checkPairing, 5000);";

  // --- [TO BE REMOVED LATER] VIRTUAL LCD JS START ---
  html += "function updateVirtualLcd() {";
  html += "  fetch('/api/status').then(r => r.json()).then(data => {";
  html +=
      "    document.getElementById('lcdStatus').textContent = data.lastStatus;";
  html +=
      "    document.getElementById('lcdCount').textContent = data.totalItems;";
  html += "    if(data.secondsSinceLastCapture > 0) {";
  html += "       document.getElementById('lcdTime').textContent = "
          "data.secondsSinceLastCapture + ' sec ago';";
  html += "    }";
  html += "  }).catch(e => console.error(e));";
  html += "}";
  html += "setInterval(updateVirtualLcd, 2000);"; // Poll every 2 seconds
  // --- [TO BE REMOVED LATER] VIRTUAL LCD JS END ---

  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleCapture() {
  Serial.println("[Web] Request received: GET /capture");
  camera_fb_t *fb = captureFrame();
  if (!fb) {
    Serial.println("[Capture] ERROR: captureFrame returned NULL");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-Length: " + String(fb->len) + "\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "Connection: close\r\n";
  response += "\r\n";

  client.print(response);
  client.write(fb->buf, fb->len);

  returnFrame(fb);
  Serial.println("[Web] Served /capture successfully");
}

void handleSDCapture() {
  Serial.println("[Capture] Acquiring frame for SD Card...");

  displayCaptureFlash();
  displayScanning();

  // Give the SPI bus a moment to completely reset and allow the camera DMA to
  // flush
  delay(100);

  // The live preview sometimes leaves a frame stuck in the buffer right as the
  // button is pressed. Flush it once to guarantee a fresh, stable frame for the
  // SD card.
  camera_fb_t *fb = captureFrame();
  if (fb)
    returnFrame(fb);

  // Wait for the sensor line to restabilize
  delay(50);

  // Now grab the actual frame safely without dropping reference
  fb = captureFrame();
  if (!fb) {
    Serial.println("[ERROR] SD Capture Failed: No frame available.");
    displayError("Capture Failed");
    strcpy(lastCaptureStatus, "Capture Error");
    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);
    return;
  }

  displayUploading(50); // Show "Saving..." roughly
  String filename = saveImageToSD(fb->buf, fb->len);
  returnFrame(fb);

  if (filename.length() > 0) {
    Serial.printf("[Upload] Queued offline: %s\n", filename.c_str());
    strcpy(lastCaptureStatus, "Saved to SD");
    displayStatus("Saved to SD");
    lastCaptureTimestamp = millis();
    totalItemsUploaded++; // Count as handled

    led.setPixelColor(0, led.Color(0, 255, 0)); // Green blink
    led.show();
    delay(500);

    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);
  } else {
    Serial.println("[ERROR] Failed to save frame to SD Card memory.");
    displayError("SD Save Failed");
    strcpy(lastCaptureStatus, "SD Save Error");
    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);

    led.setPixelColor(0, led.Color(255, 0, 0)); // Red blink
    led.show();
    delay(500);
  }
}

void handleUpload() {
  Serial.println("[Web] Request received: POST /api/upload");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // Show capture flash for visual feedback
  displayCaptureFlash();

  // Show scanning animation
  displayScanning();

  camera_fb_t *fb = captureFrame();
  if (!fb) {
    Serial.println("[Upload] CAMERA BUFFER FAILED!");
    displayError("Capture Failed");
    strcpy(lastCaptureStatus, "Capture Error");
    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);

    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = "Camera capture failed";
    String responseStr;
    serializeJson(doc, responseStr);
    server.send(400, "application/json", responseStr);
    return;
  }

  displayUploading(50);
  strcpy(lastCaptureStatus, "Uploading...");

  Serial.println("[Upload] Dispatched to Cloud...");
  char *response = uploadImage(fb->buf, fb->len);
  returnFrame(fb);

  if (response) {
    // Success - update counters
    totalItemsUploaded++;
    lastCaptureTimestamp = millis();
    strcpy(lastCaptureStatus, "Upload OK");

    displayUploadComplete();
    server.send(200, "application/json", response);
    led.setPixelColor(0, led.Color(0, 255, 0));
    led.show();
    delay(1000);

    // Show success with debug info
    displaySuccess();
    delay(500);
    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);
  } else {
    // Failed - update status
    strcpy(lastCaptureStatus, "Upload Failed");

    displayError("Upload Failed");
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = "Upload failed";
    String responseStr;
    serializeJson(doc, responseStr);
    server.send(500, "application/json", responseStr);
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
    delay(1000);

    // Show error with debug info
    displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                       lastCaptureTimestamp);
  }
}

// --- [TO BE REMOVED LATER] VIRTUAL LCD ENDPOINT START ---
void handleStatus() {
  JsonDocument doc;
  doc["totalItems"] = totalItemsUploaded;
  doc["lastStatus"] = lastCaptureStatus;

  if (lastCaptureTimestamp > 0) {
    doc["secondsSinceLastCapture"] = (millis() - lastCaptureTimestamp) / 1000;
  } else {
    doc["secondsSinceLastCapture"] = 0;
  }

  doc["isPaired"] = isPaired;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
// --- [TO BE REMOVED LATER] VIRTUAL LCD ENDPOINT END ---

void handlePairingStart() {
  char *code = startPairing();

  JsonDocument doc;
  if (code) {
    doc["success"] = true;
    doc["code"] = code;
    strncpy(pairingCode, code, sizeof(pairingCode) - 1);
    displayPairingCode(pairingCode);
  } else {
    doc["success"] = false;
    doc["error"] = "Failed to generate code";
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePairingStatus() {
  JsonDocument doc;
  if (pairingCode[0] != '\0') {
    char *token = checkPairingStatus(pairingCode);
    if (token) {
      isPaired = true;
      doc["paired"] = true;
      displaySuccess();
      led.setPixelColor(0, led.Color(0, 255, 0));
      led.show();
    } else {
      doc["paired"] = false;
    }
  } else {
    doc["paired"] = false;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleUnpair() {
  clearAuthToken();
  isPaired = false;
  pairingCode[0] = '\0';

  char *code = startPairing();

  JsonDocument doc;
  doc["success"] = true;
  if (code) {
    strncpy(pairingCode, code, sizeof(pairingCode) - 1);
    doc["new_code"] = code;
    displayPairingCode(pairingCode);
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  led.setPixelColor(0, led.Color(255, 165, 0));
  led.show();
}

void handleFactoryReset() {
  Serial.println("[System] Factory resetting Wi-Fi and Cloud credentials...");
  displayStatus("Resetting...");

  // Wipe Cloud Pairing
  clearAuthToken();
  isPaired = false;

  // Wipe Wi-Fi Manager Settings
  WiFiManager wm;
  wm.resetSettings();

  server.send(200, "text/plain",
              "Factory Reset Complete! Rebooting into AP Setup Mode...");
  delay(1000);
  ESP.restart();
}

// ============================================
// WiFiManager Callbacks
// ============================================

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

  displayWiFiSetup(myWiFiManager->getConfigPortalSSID().c_str());

  // Fast orange blinking for setup mode indication
  led.setPixelColor(0, led.Color(255, 165, 0));
  led.show();
}

// ============================================
// Setup
// ============================================

void setup() {
  // --- POWER LATCH CONFIG ---
  // Turn on the N-Channel MOSFET instantly to keep the P-Channel open.
  // pinMode(POWER_LATCH_PIN, OUTPUT);
  // digitalWrite(POWER_LATCH_PIN, HIGH); // Latch power ON!

  delay(5000);
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n=================================");
  Serial.println("=== ResearchMate Smart Pen ===");
  Serial.println("=== TFT Display + Camera ===");
  Serial.println("=================================\n");

  // Capture button (GPIO 0)
  pinMode(CAPTURE_BUTTON_PIN, INPUT_PULLUP);

  // Dedicated Power Button (GPIO 2)
  // pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Buttons initialized");

  // LED
  led.begin();
  led.setBrightness(50);
  led.setPixelColor(0, led.Color(0, 255, 255));
  led.show();

  // TFT Display
  Serial.println("[1/4] Initializing TFT display...");
  if (initDisplay()) {
    Serial.println("      [OK] Display ready");
    displayStatus("Booting...");
  } else {
    Serial.println("      [X] Display failed");
  }

  delay(500);

  // Camera
  Serial.println("[2/4] Initializing camera...");
  displayStatus("Camera init...");
  if (!initCamera()) {
    Serial.println("      [X] Camera FAILED!");
    displayError("Camera Error");
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
  } else {
    Serial.println("      [OK] Camera ready");
    led.setPixelColor(0, led.Color(0, 255, 0));
    led.show();
  }

  delay(500);

  // Cloud & Storage
  Serial.println("[3/4] Initializing cloud & SD storage...");
  displayStatus("Storage init...");
  initSDCard();
  initCloud();

  delay(500);

  // WiFi
  Serial.println("[4/4] Connecting WiFi...");
  displayStatus("WiFi connecting...");
  led.setPixelColor(0, led.Color(255, 165, 0));
  led.show();

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  // Optional: timeout to prevent getting stuck in portal mode forever
  // wifiManager.setConfigPortalTimeout(180);

  // AutoConnect does the magic
  if (!wifiManager.autoConnect(AP_NAME)) {
    Serial.println("Failed to connect and hit timeout");
    displayError("WiFi Failed");
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
    delay(3000);
    ESP.restart(); // Reset and try again
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[OK] WiFi connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
    displayWiFiInfo(WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

    // Prevent the S3 WiFi modem from entering aggressive sleep routing
    // which drops incoming TCP HTTP requests randomly
    WiFi.setSleep(false);

    led.setPixelColor(0, led.Color(0, 0, 255));
    led.show();
    delay(2000);

    // Check pairing
    if (getAuthToken()) {
      Serial.println("[OK] Already paired!");
      isPaired = true;
      livePreviewActive = true;
      displayReady();
      led.setPixelColor(0, led.Color(0, 255, 0));
      led.show();
    } else {
      Serial.println("[!] Not paired - starting pairing...");
      char *code = startPairing();
      if (code) {
        strncpy(pairingCode, code, sizeof(pairingCode) - 1);
        Serial.printf("Pairing code: %s\n", pairingCode);
        displayPairingCode(pairingCode);
        led.setPixelColor(0, led.Color(255, 165, 0));
        led.show();
      }
    }
  } else {
    Serial.println("[X] WiFi failed!");
    displayError("WiFi Failed");
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
  }

  // Web server
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/api/upload", HTTP_POST, handleUpload);
  server.on("/api/pairing-start", HTTP_POST, handlePairingStart);
  server.on("/api/pairing-status", HTTP_GET, handlePairingStatus);
  server.on("/api/unpair", HTTP_POST, handleUnpair);
  // --- [TO BE REMOVED LATER] ---
  server.on("/api/status", HTTP_GET, handleStatus);
  // ----------------------------
  server.begin();

  Serial.println("\n=== READY ===");
  Serial.printf("Open: http://%s:8080\n", WiFi.localIP().toString().c_str());
}

// Button debouncing and multi-press tracking
static unsigned long buttonPressStartTime = 0;
static unsigned long buttonReleaseTime = 0;
static int buttonPressCount = 0;
static bool isButtonPressed = false;
static bool longPressHandled = false;

// Time thresholds (milliseconds)
const unsigned long DEBOUNCE_TIME = 50;
const unsigned long LONG_PRESS_TIME = 800;
const unsigned long DOUBLE_PRESS_GAP = 400;

void updateButtonState() {
  bool currentButtonState = (digitalRead(CAPTURE_BUTTON_PIN) == LOW);

  if (currentButtonState && !isButtonPressed) {
    if (millis() - buttonReleaseTime > DEBOUNCE_TIME) {
      isButtonPressed = true;
      buttonPressStartTime = millis();
      longPressHandled = false;
    }
  } else if (!currentButtonState && isButtonPressed) {
    isButtonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStartTime;
    buttonReleaseTime = millis();

    if (!longPressHandled && pressDuration > DEBOUNCE_TIME &&
        pressDuration < LONG_PRESS_TIME) {
      buttonPressCount++;
    }
  } else if (currentButtonState && isButtonPressed) {
    if (!longPressHandled &&
        (millis() - buttonPressStartTime > LONG_PRESS_TIME)) {
      livePreviewActive = false;
      Serial.println("[Button] LONG PRESS Detected: Saving to SD Card!");
      displayStatus("Saving to SD...");
      handleSDCapture();
      displayReady();
      livePreviewActive = true;
      longPressHandled = true;
      buttonPressCount = 0;
    }
  }
}

void evaluateButtonActions() {
  if (buttonPressCount > 0 && !isButtonPressed &&
      (millis() - buttonReleaseTime > DOUBLE_PRESS_GAP)) {
    // CRITICAL: We only evaluate actions AFTER the double-press window expires.

    if (buttonPressCount == 1) {
      livePreviewActive = false;
      Serial.println("[Button] SINGLE PRESS Detected: Capturing Preview!");
      displayCaptureFlash();
      camera_fb_t *fb = captureFrame();
      if (fb)
        returnFrame(fb);
      displayReady();
      livePreviewActive = true;
    } else if (buttonPressCount >= 2) {
      livePreviewActive = false;
      Serial.println("[Button] DOUBLE PRESS Detected: Uploading to Cloud!");
      displayStatus("Uploading...");
      handleUpload(); // Capture and dispatch to Supabase
      displayReady();
      livePreviewActive = true;
    }
    buttonPressCount = 0; // Reset after handling
  }
}

void loop() {
  server.handleClient();

  // Continuously poll the button
  updateButtonState();
  evaluateButtonActions();

  // Periodic debug display update (every 5 seconds)
  static unsigned long lastDebugUpdate = 0;
  if (millis() - lastDebugUpdate > 5000) {
    if (!livePreviewActive) {
      displayCameraDebug(totalItemsUploaded, lastCaptureStatus,
                         lastCaptureTimestamp);
    }
    lastDebugUpdate = millis();
  }

  // Periodic pairing check
  if (!isPaired && (millis() - lastPairingCheck) > PAIRING_CHECK_INTERVAL) {
    lastPairingCheck = millis();
    // ... rest of pairing loop
    if (pairingCode[0] != '\0') {
      char *token = checkPairingStatus(pairingCode);
      if (token) {
        isPaired = true;
        livePreviewActive = true;
        Serial.println("[OK] Device paired!");
        displayReady();
        delay(500);
        displayReady(); // Clear again just to be safe before preview starts
        led.setPixelColor(0, led.Color(0, 255, 0));
        led.show();
      }
    }
  }

  // Periodic SD Sync Queue Check (every 10 seconds)
  static unsigned long lastSyncCheck = 0;
  if (millis() - lastSyncCheck > 10000) {
    lastSyncCheck = millis();
    syncPendingQueue();
  }

  // --- LIVE CAMERA PREVIEW ---
  // Suspend camera pulling during double-press gap to allow fast polling of
  // button
  if (isPaired && livePreviewActive && buttonPressCount == 0) {
    camera_fb_t *fb = captureFrame();
    if (fb) {
      displayDrawFrame(fb->buf, fb->len);
      returnFrame(fb);
    }
  }

  delay(10);
}