// ============================================
// ResearchMate Smart Pen - Full Integration
// Camera + TFT Display + Cloud Pairing  
// ============================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "camera/camera.h"
#include "display/display.h"
#include "cloud/cloud.h"
#include "config.h"

WebServer server(80);
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// Global state
static char lastCapturedFilename[64] = {0};
static char pairingCode[16] = {0};
static bool isPaired = false;
static unsigned long lastPairingCheck = 0;
static const unsigned long PAIRING_CHECK_INTERVAL = 5000;

// Camera debug state
static int totalItemsUploaded = 0;
static unsigned long lastCaptureTimestamp = 0;
static char lastCaptureStatus[32] = "Ready";

// Button state tracking (polling)
static bool lastButtonState = HIGH;  // Default high (INPUT_PULLUP)
static unsigned long buttonPressStart = 0;
// No Interrupt required for better stability


// ============================================
// Web Handlers (from original project)
// ============================================

void handleRoot() {
    String pairingStatus = isPaired ? 
        "<span style='color: #00ff41;'>[OK] Paired</span>" : 
        "<span style='color: #ffd700;'>[!] Waiting for pairing...</span>";
    
    String pairingCodeHtml = "";
    if (!isPaired && pairingCode[0]) {
        pairingCodeHtml = String("<div style='background: #0f3460; padding: 20px; border: 2px solid #ffd700; margin: 20px 0; border-radius: 10px;'>") +
        "<p style='text-align: center; color: #999; font-size: 12px;'>PAIRING CODE</p>" +
        "<p id='pairingCode' style='text-align: center; font-size: 32px; font-weight: bold; color: #ffd700; letter-spacing: 10px;'>" + 
        pairingCode + "</p>" +
        "<p style='text-align: center; color: #999; font-size: 11px;'>Enter this code on your ResearchMate website</p>" +
        "<button class='btn-dark' style='width: 100%; margin-top: 15px;' onclick='refreshCode()'>REFRESH CODE</button>" +
        "</div>";
    }

    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ResearchMate Smart Pen</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; margin: 0; padding: 20px; }";
    html += ".container { background: #16213e; padding: 30px; border-radius: 15px; max-width: 700px; margin: 0 auto; }";
    html += "h1 { color: #00d4ff; text-align: center; }";
    html += ".subtitle { text-align: center; color: #999; font-size: 12px; }";
    html += "#camera { width: 100%; border: 2px solid #00d4ff; border-radius: 10px; margin: 20px 0; background: #000; }";
    html += ".buttons { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 20px 0; }";
    html += "button { padding: 12px; font-weight: bold; cursor: pointer; border: none; border-radius: 5px; font-size: 14px; }";
    html += ".btn-cyan { background: #00d4ff; color: #000; }";
    html += ".btn-green { background: #00ff41; color: #000; }";
    html += ".btn-dark { background: #0f3460; color: #fff; border: 2px solid #00d4ff; }";
    html += ".btn-red { background: #ff6b6b; color: #fff; }";
    html += ".btn-orange { background: #ffd700; color: #000; }";
    html += ".btn-purple { background: #9d4edd; color: #fff; grid-column: 1 / -1; }";
    html += ".status { background: #0f3460; padding: 15px; border-left: 4px solid #00ff41; border-radius: 5px; margin: 20px 0; text-align: center; }";
    html += ".status.unpaired { border-left: 4px solid #ffd700; }";
    html += "#uploadStatus { margin-top: 20px; padding: 15px; border-radius: 5px; background: #0f3460; color: #999; text-align: center; font-size: 12px; display: none; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>ResearchMate Smart Pen</h1>";
    html += "<p class='subtitle'>OV2640 Camera + TFT Display</p>";
    
    if (isPaired) {
        html += "<div class='status'>[OK] Connected - " + pairingStatus + " <button class='btn-dark' style='padding: 5px 15px; margin-left: 10px; font-size: 11px;' onclick='unpair()'>UNPAIR</button></div>";
    } else {
        html += "<div class='status unpaired'>[PAIRING] " + pairingStatus + "</div>";
    }
    
    html += pairingCodeHtml;
    html += "<img id='camera' src='/capture' alt='Camera Feed'>";
    
    html += "<div class='buttons'>";
    html += "<button class='btn-cyan' onclick='capture()'>CAPTURE PREVIEW</button>";
    html += "<button class='btn-purple' id='uploadBtn' onclick='upload()'>UPLOAD TO CLOUD</button>";
    html += "</div>";
    
    html += "<div id='uploadStatus'></div>";
    
    html += "<script>";
    html += "var isPaired = " + String(isPaired ? "true" : "false") + ";";
    
    html += "function capture() { document.getElementById('camera').src = '/capture?t=' + Date.now(); }";
    
    html += "function upload() {";
    html += "  if (!isPaired) { showStatus('Not paired yet!', 'error'); return; }";
    html += "  document.getElementById('uploadBtn').disabled = true;";
    html += "  showStatus('Uploading...', 'pending');";
    html += "  fetch('/api/upload', { method: 'POST' })";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      if (data.success) { showStatus('[OK] Uploaded!', 'success'); }";
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
    
    html += "function checkPairing() { if (!isPaired) { fetch('/api/pairing-status').then(r => r.json()).then(data => { if (data.paired) { location.reload(); } }); } }";
    html += "function refreshCode() { fetch('/api/pairing-start', { method: 'POST' }).then(r => r.json()).then(data => { if (data.success) { document.getElementById('pairingCode').textContent = data.code; } }); }";
    html += "function unpair() { if (!confirm('Unpair?')) return; fetch('/api/unpair', { method: 'POST' }).then(r => r.json()).then(data => { if (data.success) { location.reload(); } }); }";
    
    html += "capture();";
    html += "if (!isPaired) setInterval(checkPairing, 5000);";
    html += "</script></body></html>";
    
    server.send(200, "text/html", html);
}

void handleCapture() {
    camera_fb_t* fb = captureFrame();
    if (!fb) {
        Serial.println("[Capture] ERROR: captureFrame returned NULL");
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }
    
    WiFiClient client = server.client();
    
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: image/jpeg\r\n";
    response += "Content-Length: " + String(fb->len) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    
    client.print(response);
    client.write(fb->buf, fb->len);
    
    returnFrame(fb);
}

void handleUpload() {
    // Show capture flash for visual feedback
    displayCaptureFlash();
    
    // Show scanning animation
    displayScanning();
    
    camera_fb_t* fb = captureFrame();
    if (!fb) {
        displayError("Capture Failed");
        strcpy(lastCaptureStatus, "Capture Error");
        displayCameraDebug(totalItemsUploaded, lastCaptureStatus, lastCaptureTimestamp);
        
        JsonDocument doc;
        doc["success"] = false;
        doc["error"] = "Camera capture failed";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    displayUploading(50);
    strcpy(lastCaptureStatus, "Uploading...");
    
    char* response = uploadImage(fb->buf, fb->len);
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
        displayCameraDebug(totalItemsUploaded, lastCaptureStatus, lastCaptureTimestamp);
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
        displayCameraDebug(totalItemsUploaded, lastCaptureStatus, lastCaptureTimestamp);
    }
}

void handlePairingStart() {
    char* code = startPairing();
    
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
        char* token = checkPairingStatus(pairingCode);
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
    
    char* code = startPairing();
    
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

// ============================================
// Setup
// ============================================

void setup() {
    delay(5000);
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=================================");
    Serial.println("=== ResearchMate Smart Pen ===");
    Serial.println("=== TFT Display + Camera ===");
    Serial.println("=================================\n");
    
    // Capture button (GPIO 19)
    pinMode(CAPTURE_BUTTON_PIN, INPUT_PULLUP);
    // attachInterrupt removed - using polling in loop for better stability
    Serial.println("Capture button initialized (GPIO 19)");
    
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
    
    // Cloud
    Serial.println("[3/4] Initializing cloud...");
    displayStatus("Cloud init...");
    initCloud();
    
    delay(500);
    
    // WiFi
    Serial.println("[4/4] Connecting WiFi...");
    displayStatus("WiFi connecting...");
    led.setPixelColor(0, led.Color(255, 165, 0));
    led.show();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[OK] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        displayWiFiInfo(WiFi.localIP().toString().c_str(), WIFI_SSID);
        led.setPixelColor(0, led.Color(0, 0, 255));
        led.show();
        delay(2000);
        
        // Check pairing
        if (getAuthToken()) {
            Serial.println("[OK] Already paired!");
            isPaired = true;
            displaySuccess();
            led.setPixelColor(0, led.Color(0, 255, 0));
            led.show();
        } else {
            Serial.println("[!] Not paired - starting pairing...");
            char* code = startPairing();
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
    server.begin();
    
    Serial.println("\n=== READY ===");
    Serial.printf("Open: http://%s\n", WiFi.localIP().toString().c_str());
}

void loop() {
    server.handleClient();
    
    // Handle button press (TRIGGER ON RELEASE or HOLD)
    // Polling with debounce for stability
    bool currentButtonState = digitalRead(CAPTURE_BUTTON_PIN);
    
    // Check for button press (falling edge: HIGH -> LOW)
    if (lastButtonState == HIGH && currentButtonState == LOW) {
        buttonPressStart = millis();
        Serial.println("[Button] Pressed...");
    }
    
    // Check for valid press (held for > 50ms)
    if (lastButtonState == LOW && currentButtonState == LOW) {
        if (millis() - buttonPressStart > 50) { 
            // Button is definitely pressed
        }
    }
    
    // Trigger on RELEASE (rising edge: LOW -> HIGH) to avoid repeating
    if (lastButtonState == LOW && currentButtonState == HIGH) {
        if (millis() - buttonPressStart > 50) { // Debounce threshold
            Serial.println("[Button] Released - Triggering Capture!");
            handleUpload();
        }
    }
    
    lastButtonState = currentButtonState;
    
    // Periodic debug display update (every 5 seconds)
    static unsigned long lastDebugUpdate = 0;
    if (millis() - lastDebugUpdate > 5000) {
        displayCameraDebug(totalItemsUploaded, lastCaptureStatus, lastCaptureTimestamp);
        lastDebugUpdate = millis();
    }
    
    // Periodic pairing check
    if (!isPaired && (millis() - lastPairingCheck) > PAIRING_CHECK_INTERVAL) {
        lastPairingCheck = millis();
        if (pairingCode[0] != '\0') {
            char* token = checkPairingStatus(pairingCode);
            if (token) {
                isPaired = true;
                Serial.println("[OK] Device paired!");
                displaySuccess();
                delay(500);
                displayCameraDebug(totalItemsUploaded, lastCaptureStatus, lastCaptureTimestamp);
                led.setPixelColor(0, led.Color(0, 255, 0));
                led.show();
            }
        }
    }
    
    delay(10);
}