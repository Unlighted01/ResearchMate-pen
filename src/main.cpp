#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "camera/camera.h"
#include "cloud/cloud.h"
#include "config.h"

WebServer server(80);
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// Global state
static char lastCapturedFilename[64] = {0};
static char pairingCode[16] = {0};
static bool isPaired = false;
static unsigned long lastPairingCheck = 0;
static const unsigned long PAIRING_CHECK_INTERVAL = 5000; // Check every 5 seconds

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
    html += ".btn-cyan:hover { background: #00a8cc; }";
    html += ".btn-green { background: #00ff41; color: #000; }";
    html += ".btn-green:hover { background: #00cc33; }";
    html += ".btn-dark { background: #0f3460; color: #fff; border: 2px solid #00d4ff; }";
    html += ".btn-dark:hover { background: #1a4d7a; }";
    html += ".btn-red { background: #ff6b6b; color: #fff; }";
    html += ".btn-red:hover { background: #ff5252; }";
    html += ".btn-orange { background: #ffd700; color: #000; }";
    html += ".btn-orange:hover { background: #ffb700; }";
    html += ".btn-purple { background: #9d4edd; color: #fff; grid-column: 1 / -1; }";
    html += ".btn-purple:hover { background: #7b2cbf; }";
    html += ".btn-purple:disabled { background: #555; cursor: not-allowed; opacity: 0.5; }";
    html += ".info { text-align: center; color: #999; font-size: 12px; margin-top: 10px; }";
    html += ".status { background: #0f3460; padding: 15px; border-left: 4px solid #00ff41; border-radius: 5px; margin: 20px 0; text-align: center; }";
    html += ".status.unpaired { border-left: 4px solid #ffd700; }";
    html += "#uploadStatus { margin-top: 20px; padding: 15px; border-radius: 5px; background: #0f3460; color: #999; text-align: center; font-size: 12px; display: none; }";
    html += "#uploadStatus.success { border-left: 4px solid #00ff41; color: #00ff41; }";
    html += "#uploadStatus.error { border-left: 4px solid #ff6b6b; color: #ff6b6b; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>ResearchMate Smart Pen</h1>";
    html += "<p class='subtitle'>OV2640 Camera - 2MP</p>";
    
    if (isPaired) {
        html += "<div class='status'>[OK] Connected to ResearchMate - " + pairingStatus + " <button class='btn-dark' style='padding: 5px 15px; margin-left: 10px; font-size: 11px;' onclick='unpair()'>UNPAIR</button></div>";
    } else {
        html += "<div class='status unpaired'>[PAIRING] " + pairingStatus + "</div>";
    }
    
    html += pairingCodeHtml;
    html += "<p style='text-align: center; color: #999; font-size: 12px;'>Your scans will sync automatically to the cloud</p>";
    html += "<img id='camera' src='/capture' alt='Camera Feed'>";
    
    html += "<div class='buttons'>";
    html += "<button class='btn-cyan' onclick='capture()'>CAPTURE</button>";
    html += "<button class='btn-dark' onclick='stopStream()'>STOP</button>";
    html += "<button class='btn-red' onclick='toggleStream()'>STREAM</button>";
    html += "<button class='btn-orange' onclick='toggleFlash()'>FLASH</button>";
    html += "<button class='btn-purple' id='uploadBtn' onclick='upload()'>UPLOAD TO CLOUD</button>";
    html += "</div>";
    
    html += "<div id='uploadStatus'></div>";
    
    html += "<div style='text-align: center; margin-top: 20px;'>";
    html += "<label><input type='checkbox' id='autorefresh' onchange='toggleAuto()'> <span id='autoLabel'>Auto-refresh OFF</span></label>";
    html += String("<div class='info'>Resolution: 640x480 | Format: JPEG | Status: ") + (isPaired ? "READY" : "PAIRING") + "</div>";
    html += "</div>";
    
    html += "<script>";
    html += "var autoRefresh = false;";
    html += "var refreshInterval = null;";
    html += "var isPaired = " + String(isPaired ? "true" : "false") + ";";
    html += "var lastUploadResult = null;";
    
    html += "function capture() { document.getElementById('camera').src = '/capture?t=' + Date.now(); }";
    
    html += "function toggleAuto() {";
    html += "  autoRefresh = document.getElementById('autorefresh').checked;";
    html += "  document.getElementById('autoLabel').textContent = autoRefresh ? 'Auto-refresh ON' : 'Auto-refresh OFF';";
    html += "  if (autoRefresh) {";
    html += "    refreshInterval = setInterval(capture, 500);";
    html += "  } else {";
    html += "    clearInterval(refreshInterval);";
    html += "  }";
    html += "}";
    
    html += "function stopStream() { if (autoRefresh) toggleAuto(); }";
    html += "function toggleStream() { capture(); }";
    html += "function toggleFlash() { alert('Flash activated!'); }";
    
    html += "function upload() {";
    html += "  if (!isPaired) {";
    html += "    showStatus('Not paired yet! Waiting for pairing confirmation...', 'error');";
    html += "    return;";
    html += "  }";
    html += "  document.getElementById('uploadBtn').disabled = true;";
    html += "  showStatus('Uploading to cloud...', 'pending');";
    html += "  fetch('/api/upload', { method: 'POST' })";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      if (data.success) {";
    html += "        showStatus('[OK] Uploaded! OCR: ' + data.ocr_text.substring(0, 50) + '...', 'success');";
    html += "      } else {";
    html += "        showStatus('[X] Upload failed: ' + data.error, 'error');";
    html += "      }";
    html += "      document.getElementById('uploadBtn').disabled = false;";
    html += "    })";
    html += "    .catch(e => {";
    html += "      showStatus('[X] Error: ' + e.message, 'error');";
    html += "      document.getElementById('uploadBtn').disabled = false;";
    html += "    });";
    html += "}";
    
    html += "function showStatus(msg, type) {";
    html += "  var el = document.getElementById('uploadStatus');";
    html += "  el.textContent = msg;";
    html += "  el.className = type;";
    html += "  el.style.display = 'block';";
    html += "  setTimeout(() => { el.style.display = 'none'; }, 5000);";
    html += "}";
    
    html += "function checkPairing() {";
    html += "  if (!isPaired) {";
    html += "    fetch('/api/pairing-status')";
    html += "      .then(r => r.json())";
    html += "      .then(data => {";
    html += "        if (data.paired) {";
    html += "          isPaired = true;";
    html += "          location.reload();";
    html += "        }";
    html += "      });";
    html += "  }";
    html += "}";
    
    html += "function refreshCode() {";
    html += "  fetch('/api/pairing-start', { method: 'POST' })";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      if (data.success) {";
    html += "        document.getElementById('pairingCode').textContent = data.code;";
    html += "        showStatus('New code: ' + data.code, 'success');";
    html += "      } else {";
    html += "        showStatus('Failed to refresh code', 'error');";
    html += "      }";
    html += "    });";
    html += "}";
    
    html += "function unpair() {";
    html += "  if (!confirm('Unpair from current account?')) return;";
    html += "  fetch('/api/unpair', { method: 'POST' })";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      if (data.success) {";
    html += "        showStatus('Unpaired! New code: ' + data.new_code, 'success');";
    html += "        setTimeout(() => location.reload(), 1500);";
    html += "      } else {";
    html += "        showStatus('Unpair failed', 'error');";
    html += "      }";
    html += "    });";
    html += "}";
    
    html += "capture();";
    html += "if (!isPaired) setInterval(checkPairing, 5000);";
    html += "</script>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleCapture() {
    camera_fb_t* fb = captureFrame();
    if (!fb) {
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }
    
    // Properly send JPEG image
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
    camera_fb_t* fb = captureFrame();
    if (!fb) {
        JsonDocument doc;
        doc["success"] = false;
        doc["error"] = "Camera capture failed";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    Serial.println("[API] Starting image upload...");
    
    char* response = uploadImage(fb->buf, fb->len);
    returnFrame(fb);

    if (response) {
        server.send(200, "application/json", response);
        Serial.println("[API] Upload successful!");
        led.setPixelColor(0, led.Color(0, 255, 0));
        led.show();
    } else {
        JsonDocument doc;
        doc["success"] = false;
        doc["error"] = "Supabase upload failed";
        String responseStr;
        serializeJson(doc, responseStr);
        server.send(500, "application/json", responseStr);
        Serial.println("[API] Upload failed!");
        led.setPixelColor(0, led.Color(255, 0, 0));
        led.show();
    }
}

void handlePairingStart() {
    Serial.println("[Pairing] Starting pairing from web UI");
    char* code = startPairing();
    
    JsonDocument doc;
    if (code) {
        doc["success"] = true;
        doc["code"] = code;
        strncpy(pairingCode, code, sizeof(pairingCode) - 1);
    } else {
        doc["success"] = false;
        doc["error"] = "Failed to generate pairing code";
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handlePairingStatus() {
    Serial.println("[Pairing] Checking pairing status");
    
    JsonDocument doc;
    if (pairingCode[0] != '\0') {
        char* token = checkPairingStatus(pairingCode);
        if (token) {
            isPaired = true;
            doc["paired"] = true;
            doc["auth_token"] = token;
            led.setPixelColor(0, led.Color(0, 255, 0));
            led.show();
        } else {
            doc["paired"] = false;
        }
    } else {
        doc["paired"] = false;
        doc["error"] = "No pairing code available";
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleUnpair() {
    Serial.println("[API] Unpair requested");
    
    clearAuthToken();
    isPaired = false;
    pairingCode[0] = '\0';
    
    char* code = startPairing();
    
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Device unpaired";
    if (code) {
        strncpy(pairingCode, code, sizeof(pairingCode) - 1);
        doc["new_code"] = code;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    
    led.setPixelColor(0, led.Color(255, 165, 0));
    led.show();
}

void setup() {
    delay(5000);
    
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n");
    Serial.println("=================================");
    Serial.println("=== ResearchMate Smart Pen ===");
    Serial.println("=================================");
    Serial.println("ESP32-S3-WROOM with OV2640\n");
    Serial.println("Serial initialized!");
    
    // Initialize LED
    Serial.println("Initializing LED...");
    led.begin();
    led.setBrightness(50);
    led.setPixelColor(0, led.Color(0, 255, 255));
    led.show();
    Serial.println("LED initialized (Cyan)");
    
    // Initialize camera
    Serial.println("\nInitializing camera...");
    if (!initCamera()) {
        Serial.println("\n!!! ERROR: Camera initialization failed !!!");
        Serial.println("Possible causes:");
        Serial.println("  - Camera module not connected properly");
        Serial.println("  - Wrong GPIO pin configuration");
        Serial.println("  - I2C (SCCB) communication failure");
        Serial.println("  - Power supply issue");
        Serial.println("\nContinuing without camera...");
        
        led.setPixelColor(0, led.Color(255, 0, 0));
        led.show();
    } else {
        Serial.println("Camera initialized successfully!");
        led.setPixelColor(0, led.Color(0, 255, 0));
        led.show();
    }
    
    // Initialize cloud module
    Serial.println("\nInitializing cloud module...");
    initCloud();
    Serial.println("Cloud module ready!");
    
    // Connect WiFi
    Serial.print("\nConnecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    led.setPixelColor(0, led.Color(255, 165, 0));
    led.show();
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected! IP: ");
        Serial.println(WiFi.localIP());
        led.setPixelColor(0, led.Color(0, 0, 255));
        led.show();
        
        // Check if already paired
        if (getAuthToken()) {
            Serial.println("\n[Setup] Already paired! Auth token loaded from NVS.");
            isPaired = true;
            led.setPixelColor(0, led.Color(0, 255, 0));
            led.show();
        } else {
            Serial.println("\n[Setup] Not paired yet, starting pairing...");
            char* code = startPairing();
            if (code) {
                strncpy(pairingCode, code, sizeof(pairingCode) - 1);
                Serial.print("Pairing code: ");
                Serial.println(pairingCode);
            }
        }
    } else {
        Serial.println("WiFi connection failed!");
        led.setPixelColor(0, led.Color(255, 0, 0));
        led.show();
    }
    
    // Setup web server
    server.on("/", handleRoot);
    server.on("/capture", handleCapture);
    server.on("/api/upload", HTTP_POST, handleUpload);
    server.on("/api/pairing-start", HTTP_POST, handlePairingStart);
    server.on("/api/pairing-status", HTTP_GET, handlePairingStatus);
    server.on("/api/unpair", HTTP_POST, handleUnpair);
    server.begin();
    
    Serial.println("Web server started!");
    Serial.print("Open http://");
    Serial.print(WiFi.localIP());
    Serial.println("/ in your browser");
}

void loop() {
    server.handleClient();
    
    // Periodic pairing check
    if (!isPaired && (millis() - lastPairingCheck) > PAIRING_CHECK_INTERVAL) {
        lastPairingCheck = millis();
        if (pairingCode[0] != '\0') {
            char* token = checkPairingStatus(pairingCode);
            if (token) {
                isPaired = true;
                Serial.println("[Loop] Device is now paired!");
                led.setPixelColor(0, led.Color(0, 255, 0));
                led.show();
            }
        }
    }
    
    delay(10);
}