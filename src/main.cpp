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
static bool livePreviewActive = false;
static unsigned long lastPairingCheck = 0;
static const unsigned long PAIRING_CHECK_INTERVAL = 5000;
static unsigned long pairingCodeTimestamp = 0;
static unsigned long pairingCodeExpiry = 5UL * 60UL * 1000UL; // updated from server's expires_in
static bool needsFactoryReset = false;
static bool hasHandledConnect = false;

// Camera debug state
static int totalItemsUploaded = 0;
static unsigned long lastCaptureTimestamp = 0;
static char lastCaptureStatus[32] = "Ready";

// Button state tracking (polling)
static bool lastCaptureButtonState = HIGH; // Default high (INPUT_PULLUP)
static unsigned long captureButtonPressStart = 0;

// Background Cloud Sync State
static TaskHandle_t cloudTaskHandle = NULL;
static bool forceSyncNext = false; // Flag to trigger immediate sync from web app
static volatile bool pairingJustSucceeded = false; // Set by background task, consumed by main loop

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

  // Lazy init camera if needed
  if (!initCamera()) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(500, "text/plain", "Camera Init Failed");
    return;
  }

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

  // Switch to full UXGA resolution for OCR-quality capture (preview runs at QVGA)
  setHighResCapture();
  vTaskDelay(pdMS_TO_TICKS(150)); // one frame at 10MHz XCLK to flush the QVGA buffer

  // Lazy init camera if needed
  if (!initCamera()) {
    Serial.println("[ERROR] SD Capture Failed: Camera init failed.");
    setLastAction("Camera Error", true);
    drawBottomPanel();
    return;
  }

  displayCaptureFlash();
  setLastAction("Scanning...", false);
  drawBottomPanel();

  // Grab the frame for SD card saving
  camera_fb_t *fb = captureFrame();
  if (!fb) {
    Serial.println("[ERROR] SD Capture Failed: No frame available.");
    setLastAction("Capture Error", true);
    drawBottomPanel();
    displayReady(); // Ensure we return to clean state
    return;
  }

  setLastAction("Saving...", false);
  drawBottomPanel();
  String filename = saveImageToSD(fb->buf, fb->len);
  returnFrame(fb);

  if (filename.length() > 0) {
    Serial.printf("[Upload] Queued offline: %s\n", filename.c_str());
    setLastAction("Saved to SD", false);
    totalItemsUploaded++; // Count as handled
    setQueueCount(totalItemsUploaded);
    drawBottomPanel();

    led.setPixelColor(0, led.Color(0, 255, 0)); // Green blink
    led.show();
    // Non-blocking wait for LED feedback
    vTaskDelay(pdMS_TO_TICKS(500)); 

  } else {
    Serial.println("[ERROR] Failed to save frame to SD Card memory.");
    setLastAction("SD Save Error", true);
    drawBottomPanel();

    led.setPixelColor(0, led.Color(255, 0, 0)); // Red blink
    led.show();
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  // CRITICAL: Always return display to READY state to clear "Capturing..."
  displayReady();
}

void handleUpload() {
  Serial.println("[Web] Request received: POST /api/upload");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // --- NEW LOGIC: Just manually trigger the sync queue! ---
  setLastAction("Syncing SD Queue...", false);
  drawBottomPanel();
  
  if (!WiFi.isConnected()) {
      Serial.println("[Upload] WiFi is not connected. Cannot sync to cloud.");
      setLastAction("No WiFi", true);
      drawBottomPanel();
      
      JsonDocument doc;
      doc["success"] = false;
      doc["error"] = "No WiFi Connection";
      String responseStr;
      serializeJson(doc, responseStr);
      server.send(500, "application/json", responseStr);
      
      led.setPixelColor(0, led.Color(255, 0, 0));
      led.show();
      delay(1000);
      return;
  }
  
  // Show visual feedback that a sync attempt is starting
  led.setPixelColor(0, led.Color(0, 0, 255)); // Blue for "Syncing"
  led.show();

  // Call the core sync logic - via flag to background task!
  forceSyncNext = true;

  // We have no immediate return value from syncPendingQueue() to send a specific HTTP response
  // so we'll just return a generic OK. The device UI handles the true status natively.
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Sync process completed.";
  String responseStr;
  serializeJson(doc, responseStr);
  server.send(200, "application/json", responseStr);

  // The LED reset is handled in syncPendingQueue upon success, or falling back inside loop()
  delay(500);
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
  char *code = startPairing(&pairingCodeExpiry);

  JsonDocument doc;
  if (code) {
    doc["success"] = true;
    doc["code"] = code;
    strncpy(pairingCode, code, sizeof(pairingCode) - 1);
    pairingCodeTimestamp = millis();
    doc["expires_in"] = pairingCodeExpiry / 1000;
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
      displayReady();
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
  pairingCodeTimestamp = 0;

  char *code = startPairing(&pairingCodeExpiry);

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

void triggerFactoryReset() {
  Serial.println("[System] Factory resetting Wi-Fi and Cloud credentials...");

  // Wipe pairing state
  clearAuthToken();
  isPaired = false;
  pairingCode[0] = '\0';
  pairingCodeTimestamp = 0;
  livePreviewActive = false;
  hasHandledConnect = false; // Allow onWiFiConnected to re-fire after reconnect

  // Schedule WiFi reset + portal in main loop (avoids blocking here)
  needsFactoryReset = true;

  setUIMode("FACTORY RESET");
  setLastAction("Resetting...", true);
  drawTopBar();
  drawBottomPanel();
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();
}

void handleFactoryReset() {
  triggerFactoryReset();
  server.send(200, "text/plain", "Factory Reset initiated. Entering WiFi Setup Mode...");
}

// ============================================
// Asynchronous WiFi Connection Handler
// ============================================
void onWiFiConnected() {
  if (hasHandledConnect) return;
  hasHandledConnect = true;

  Serial.printf("\n[OK] WiFi connected! IP: %s\n",
                WiFi.localIP().toString().c_str());
  setWiFiStatus(true);
  setUIMode("CONNECTED");
  setLastAction("WiFi Connected", false);

  // Immediately clear the viewfinder so the WiFi setup QR doesn't linger
  clearViewfinder();
  drawTopBar();
  drawBottomPanel();

  // Prevent the S3 WiFi modem from entering aggressive sleep routing
  // which drops incoming TCP HTTP requests randomly
  WiFi.setSleep(false);

  led.setPixelColor(0, led.Color(0, 0, 255));
  led.show();

  // Check pairing
  if (getAuthToken()) {
    Serial.println("[OK] Already paired!");
    isPaired = true;
    livePreviewActive = true;
    setPairingStatus(true);
    displayReady();
    led.setPixelColor(0, led.Color(0, 255, 0));
    led.show();
  } else {
    Serial.println("[!] Not paired - starting pairing...");
    setPairingStatus(false);
    char *code = startPairing(&pairingCodeExpiry);
    if (code) {
      strncpy(pairingCode, code, sizeof(pairingCode) - 1);
      pairingCodeTimestamp = millis();
      Serial.printf("Pairing code: %s (expires in %lu ms)\n", pairingCode, pairingCodeExpiry);
      displayPairingCode(pairingCode);
      led.setPixelColor(0, led.Color(255, 165, 0));
      led.show();
    }
  }

  Serial.println("\n=== READY ===");
  Serial.printf("Open: http://%s:8080\n", WiFi.localIP().toString().c_str());

  // LAZY INIT: Start server and camera ONLY once WiFi is solid.
  // This saves ~200KB DRAM for WiFiManager portal strings and DHCP buffers.
  Serial.println("[LazyInit] Starting Web Server...");
  server.begin();
  
  Serial.println("[LazyInit] Initializing Camera...");
  if (initCamera()) {
    Serial.println("      [OK] Camera ready (Post-WiFi)");
  } else {
    Serial.println("      [X] Camera failed to initialize (Post-WiFi)");
  }
}

// ============================================
// WiFiManager Callbacks
// ============================================

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

  displayWiFiSetupQR(myWiFiManager->getConfigPortalSSID().c_str());

  // Fast orange blinking for setup mode indication
  led.setPixelColor(0, led.Color(255, 165, 0));
  led.show();
}

// ============================================
// Global Objects
// ============================================

WiFiManager wifiManager;

// ============================================
// Setup
// ============================================

void setup() {
  // Power latch disabled - booting directly when plugged in

  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n=================================");
  Serial.println("=== ResearchMate Smart Pen ===");
  Serial.println("=== TFT Display + Camera ===");
  Serial.println("=================================\n");

  pinMode(CAPTURE_BUTTON_PIN, INPUT_PULLUP);

  // Initialize TFT really early so we can show wipe progress on screen
  // (Note: Do not add delays before this, the display chip power-on timing requests immediate init)
  Serial.println("[Display] Initializing earliest TFT...");
  initDisplay();

  // Boot-time factory reset removed. Now handled via Capture button long press.

  Serial.println("Buttons initialized");

  // LED
  led.begin();
  led.setBrightness(50);
  led.setPixelColor(0, led.Color(0, 255, 255));
  led.show();

  Serial.println("[1/4] TFT display already initialized early.");

  // Now draw the normal boot UI elements
  setUIMode("BOOTING");
  setLastAction("Initializing...", false);
  drawTopBar();
  drawBottomPanel();

  // Cloud & Storage - We keep SD and Cloud structure init but defer hardware camera
  Serial.println("[2/4] Initializing cloud & SD storage...");
  setLastAction("Storage init...", false);
  drawBottomPanel();
  initSDCard();
  initCloud();

  // WiFi
  Serial.println("[4/4] Connecting WiFi...");
  setLastAction("WiFi connecting...", false);
  setWiFiStatus(false);
  drawTopBar();
  drawBottomPanel();
  led.setPixelColor(0, led.Color(255, 165, 0));
  led.show();

  // Use the standard Station Mode initialization cleanly before AutoConnect.
  WiFi.mode(WIFI_STA);
  // Ensure the S3 WiFi modem doesn't sleep during the initial connection phase
  WiFi.setSleep(false);
  
  // Check if we have saved credentials
  if (WiFi.SSID() != "") {
    Serial.printf("[WiFi] Saved credentials found for: %s\n", WiFi.SSID().c_str());
  } else {
    Serial.println("[WiFi] No saved credentials found.");
  }
  
  vTaskDelay(pdMS_TO_TICKS(100));
  
  // CRITICAL: Make the setup portal non-blocking so the user can still
  // click the button to take offline pictures while the QR code is on screen!
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(10);      // Give up trying to connect after 10s → start AP
  wifiManager.setConfigPortalTimeout(0); // Portal stays open indefinitely (no auto-close)
  wifiManager.setAPCallback(configModeCallback);

  // AutoConnect does the magic
  if (!wifiManager.autoConnect(AP_NAME)) {
    Serial.println("Starting non-blocking WiFi Setup Portal...");
  }



  // Web server endpoints registered but begin() deferred until WiFi is ready
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/api/upload", HTTP_POST, handleUpload);
  server.on("/api/pairing-start", HTTP_POST, handlePairingStart);
  server.on("/api/pairing-status", HTTP_GET, handlePairingStatus);
  server.on("/api/unpair", HTTP_POST, handleUnpair);
  server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  server.on("/api/status", HTTP_GET, handleStatus);

  Serial.println("\n=== READY ===");
  Serial.printf("Open: http://%s:8080\n", WiFi.localIP().toString().c_str());

  // Create the background cloud task (lower priority than main loop)
  xTaskCreate(
      [](void *pvParameters) {
        for (;;) {
          // 1. Skip all cloud operations if we are in WiFi Setup / Config Mode
          // or if WiFi is disconnected. This is CRITICAL for the captive portal!
          if (!WiFi.isConnected() || wifiManager.getConfigPortalActive()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
          }

          // 2. Handle Pairing (Check/Request)
          if (!isPaired && pairingCode[0] != '\0') {
             // Auto-expire
             if (millis() - pairingCodeTimestamp > pairingCodeExpiry) {
               pairingCode[0] = '\0';
               pairingCodeTimestamp = 0;
             } else {
               char *token = checkPairingStatus(pairingCode);
               if (token) {
                 isPaired = true;
                 livePreviewActive = true;
                 setPairingStatus(true);
                 pairingJustSucceeded = true; // signal main loop to update display
               }
             }
          } else if (!isPaired && pairingCode[0] == '\0') {
             // Request new code
             char *code = startPairing(&pairingCodeExpiry);
             if (code && code[0] != '\0') {
               strncpy(pairingCode, code, sizeof(pairingCode) - 1);
               pairingCodeTimestamp = millis();
             }
          }

          // 3. Handle SD Queue Sync (Only when explicitly triggered by double press)
          // NOTE: Periodic auto-sync intentionally removed. Single press = SD only.
          // Double press = user-initiated upload. Background task must not steal photos.
          if (isPaired && forceSyncNext) {
            forceSyncNext = false;
            syncPendingQueue();
          }

          vTaskDelay(pdMS_TO_TICKS(500)); // Sleep 500ms between rounds
        }
      },
      "cloudTask", 
      8192,  // Stack size
      NULL,  // Param
      1,     // Priority (lower than loop)
      &cloudTaskHandle
  );
}

// Button debouncing and multi-press tracking
static unsigned long buttonPressStartTime = 0;
static unsigned long buttonReleaseTime = 0;
// buttonPressCount removed — replaced by pendingButtonAction (long-press model)
static bool isButtonPressed = false;
static bool factoryResetHandled = false;
static bool buttonReleasedSinceBoot = false;

// Time thresholds (milliseconds)
const unsigned long DEBOUNCE_TIME    = 50;
const unsigned long LONG_PRESS_TIME  = 800;   // hold ≥ 800ms then release → cloud upload
const unsigned long FACTORY_RESET_TIME = 5000; // hold ≥ 5s → factory reset

// Action determined on release: 0=none, 1=short(SD save), 2=long(upload)
static int pendingButtonAction = 0;

void performFactoryReset() {
  Serial.println("\n[!] FACTORY RESET DETECTED [!]");
  Serial.println("Hold button for 5 seconds to wipe device...");
  
  // Show Wipe UI
  clearScreen();
  drawHeader();
  displayWipeStart();

  // Light LED Red
  led.setBrightness(100);
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();

  bool wipeConfirmed = true;
  for (int i = 0; i < 50; i++) { // 100ms * 50 = 5 seconds
    if (digitalRead(CAPTURE_BUTTON_PIN) == HIGH) {
      wipeConfirmed = false;
      Serial.println("Reset cancelled.");
      
      displayWipeCancelled();
      delay(1000);
      break; // They let go, abort!
    }
    
    // Update progress bar
    displayWipeProgress(i * 2);
    delay(100);
  }

  if (wipeConfirmed) {
    Serial.println("\n[!] Wiping Device Credentials...");
    
    // 1. Wipe Supabase Auth Token
    clearAuthToken(); 
    
    // 2. Wipe WiFi CredentialsCtrl + Shift + P → Arduino: Select Serial Port
    WiFiManager wm;
    wm.resetSettings();
    delay(100);

    // 3. Wipe SD Card Images
    if (initSDCard()) {
       Serial.println("[!] Wiping SD Card Queue...");
       wipeOfflineQueue();
    }

    Serial.println("[!] Device Wiped Successfully.");
    
    // Flash Green rapidly 3 times to confirm
    displayWipeComplete();

    for (int i = 0; i < 3; i++) {
      led.setPixelColor(0, led.Color(0, 255, 0));
      led.show();
      delay(200);
      led.clear();
      led.show();
      delay(200);
    }
    
    Serial.println("\nRebooting now...");
    
    // Clear TFT completely to black before hardware restart to avoid white flash
    displaySleep();
    
    ESP.restart(); // Reboot into out-of-box state
  }
}

void updateButtonState() {
  bool currentButtonState = (digitalRead(CAPTURE_BUTTON_PIN) == LOW);

  // Don't process anything until button is released at least once after boot.
  // Prevents factory reset re-trigger if button is still held during ESP.restart().
  if (!buttonReleasedSinceBoot) {
    if (!currentButtonState) buttonReleasedSinceBoot = true;
    return;
  }

  if (currentButtonState && !isButtonPressed) {
    // Button just pressed
    if (millis() - buttonReleaseTime > DEBOUNCE_TIME) {
      isButtonPressed = true;
      buttonPressStartTime = millis();
      factoryResetHandled = false;
    }
  } else if (!currentButtonState && isButtonPressed) {
    // Button just released
    isButtonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStartTime;
    buttonReleaseTime = millis();

    Serial.printf("[Button] Released: duration=%lums, factoryHandled=%d\n", pressDuration, factoryResetHandled);
    if (!factoryResetHandled && pressDuration >= DEBOUNCE_TIME) {
      if (pressDuration >= LONG_PRESS_TIME) {
        // Long press (800ms–5s): cloud upload
        Serial.println("[Button] -> LONG PRESS detected (upload)");
        pendingButtonAction = 2;
        displayFactoryResetProgress(-1); // clear the progress bar
      } else {
        // Short press (< 800ms): SD save
        Serial.println("[Button] -> SHORT PRESS detected (SD save)");
        pendingButtonAction = 1;
      }
    } else if (!factoryResetHandled && pressDuration > 500) {
      displayFactoryResetProgress(-1); // clear bar on early release
    }
  } else if (currentButtonState && isButtonPressed) {
    unsigned long held = millis() - buttonPressStartTime;
    static unsigned long lastProgressUpdate = 0;

    // Show a blue progress bar during upload-hold zone (800ms → 5000ms)
    // Turns red when entering factory-reset zone
    if (!factoryResetHandled && held > 500 && millis() - lastProgressUpdate > 100) {
      lastProgressUpdate = millis();
      int pct = (int)min(100UL, held * 100UL / FACTORY_RESET_TIME);
      displayFactoryResetProgress(pct == 0 ? 1 : pct);
    }

    // 5s hold: Factory reset
    if (!factoryResetHandled && held > FACTORY_RESET_TIME) {
      Serial.println("[Button] FACTORY RESET: Wiping WiFi + Pairing...");
      displayFactoryResetProgress(100);
      vTaskDelay(pdMS_TO_TICKS(200));
      triggerFactoryReset();
      factoryResetHandled = true;
      pendingButtonAction = 0;
    }
  }
}

void evaluateButtonActions() {
  // Actions are set on button release — consume immediately when button is up
  if (pendingButtonAction == 0 || isButtonPressed) return;

  int action = pendingButtonAction;
  pendingButtonAction = 0;

  if (action == 1) {
    // Short press: capture and save to SD card only
    livePreviewActive = false;
    Serial.println("[Button] SHORT PRESS: Saving to SD Card!");
    setUIMode("SAVING");
    setLastAction("Saving to SD...", false);
    drawTopBar();
    drawBottomPanel();
    handleSDCapture();
    displayReady();
    livePreviewActive = true;
    Serial.println("[Display] Resumed Live Preview");

  } else if (action == 2) {
    // Long press: upload existing SD photo to cloud
    livePreviewActive = false;
    Serial.println("[Button] LONG PRESS: Uploading SD photo to Cloud!");
    setUIMode("SYNCING");
    setLastAction("Uploading...", false);
    drawTopBar();
    drawBottomPanel();
    displaySyncing();
    led.setPixelColor(0, led.Color(0, 0, 255));
    led.show();

    if (!WiFi.isConnected()) {
      Serial.println("[Button] LONG PRESS: No WiFi.");
      setLastAction("No WiFi", true);
      drawBottomPanel();
      led.setPixelColor(0, led.Color(255, 0, 0));
      led.show();
      vTaskDelay(pdMS_TO_TICKS(1500));
    } else if (!getAuthToken()) {
      Serial.println("[Button] LONG PRESS: Not paired.");
      setLastAction("Not Paired", true);
      drawBottomPanel();
      led.setPixelColor(0, led.Color(255, 165, 0));
      led.show();
      vTaskDelay(pdMS_TO_TICKS(1500));
    } else if (getNextPendingUpload().length() == 0) {
      Serial.println("[Button] LONG PRESS: Queue empty.");
      setLastAction("Queue Empty", true);
      drawBottomPanel();
      led.setPixelColor(0, led.Color(255, 165, 0));
      led.show();
      vTaskDelay(pdMS_TO_TICKS(1500));
    } else {
      Serial.println("[Button] LONG PRESS: Uploading from SD queue...");
      syncPendingQueue();
      setLastAction("Upload Done", false);
      drawBottomPanel();
      led.setPixelColor(0, led.Color(0, 255, 0));
      led.show();
      vTaskDelay(pdMS_TO_TICKS(500));
    }

    displayReady();
    livePreviewActive = true;
    Serial.println("[Display] Resumed Live Preview after Upload");
  }
}

void loop() {
  // Handle factory reset: wipe WiFi creds and re-enter AP config portal
  if (needsFactoryReset) {
    needsFactoryReset = false;

    // Wipe credentials before reboot so the device boots straight into AP mode.
    // Re-running autoConnect() on the existing wifiManager object leaves internal
    // portal/DNS server state inconsistent, causing the captive portal to render blank.
    wifiManager.resetSettings();
    WiFi.disconnect(true); // also erases ESP32's internal credential store

    setUIMode("RESTARTING");
    setLastAction("Restarting...", false);
    drawTopBar();
    drawBottomPanel();
    clearViewfinder();

    delay(1000);
    ESP.restart();
  }

  server.handleClient();

  // CRITICAL: Process the background non-blocking WiFi Setup portal.
  // If we don't call this continuously, the portal buttons won't work!
  wifiManager.process();

  // Asynchronous WiFi Connection Handler
  if (WiFi.status() == WL_CONNECTED) {
    onWiFiConnected();
  }

  // Continuously poll the button
  updateButtonState();
  evaluateButtonActions();

  // Periodic redraw of Top Bar for clock/status updates if needed, though we don't have a clock.
  // We can just omit drawing here unless state changes.
  // Removed old displayCameraDebug.

  // NOTE: Pairing and Sync checks were moved to background cloudTask!
  // This loop now only handles UI, button polling, and non-blocking WiFi tasks.

  // Consume pairing-success signal from background task
  if (pairingJustSucceeded) {
    pairingJustSucceeded = false;
    Serial.println("[Pairing] Confirmed! Updating display...");
    displayReady();
    drawTopBar();
    drawBottomPanel();
    led.setPixelColor(0, led.Color(0, 255, 0));
    led.show();
  }

  // --- LIVE CAMERA PREVIEW ---
  // Suspend camera pulling during double-press gap to allow fast polling of button.
  // QVGA (320x240) for preview: fast decode, correct scale. UXGA is restored before SD/upload capture.
  static bool previewResSet = false;
  if (isPaired && livePreviewActive && !isButtonPressed) {
    if (!previewResSet) {
      setImageResolution(FRAMESIZE_QVGA); // 320x240 for live preview
      previewResSet = true;
    }
    camera_fb_t *fb = captureFrame();
    if (fb) {
      displayDrawFrame(fb->buf, fb->len);
      returnFrame(fb);
    }
  } else {
    previewResSet = false; // reset so UXGA is restored on next capture
  }

  // Only yield briefly when idle (not paired/previewing) so wifiManager.process()
  // and server.handleClient() can serve HTTP as fast as possible.
  if (!isPaired || !livePreviewActive) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}