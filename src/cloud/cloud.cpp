#include "cloud.h"
#include "../storage/storage.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>


// NVS for persistent storage
static Preferences prefs;

#define LOG_DEBUG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// ============================================
// Global State
// ============================================
static char g_authToken[256] = {0};
static char g_pairingCode[16] = {0};
static unsigned long g_pairingStartTime = 0;
static const unsigned long PAIRING_TIMEOUT = 5 * 60 * 1000; // 5 minutes

// Response buffer (reusable for JSON responses)
static char g_responseBuffer[8192] = {0};

// ============================================
// Helper Functions
// ============================================

// Returns a unique ID like "ResearchMate-SmartPen-A1B2C3" using MAC address
const char *getUniquePenID() {
  static char uniqueId[48] = {0};
  if (uniqueId[0] == '\0') {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(uniqueId, sizeof(uniqueId), "%s-%02X%02X%02X", DEVICE_NAME, mac[3], mac[4], mac[5]);
  }
  return uniqueId;
}

// Generate random 6-digit code - [LEGACY / LOCAL ONLY]
static void generatePairingCode() {
  uint32_t code = 100000 + (esp_random() % 900000);
  snprintf(g_pairingCode, sizeof(g_pairingCode), "%06d", code);
  LOG_DEBUG("[Pairing] Generated code: %s", g_pairingCode);
}

// Make HTTP request to Supabase Edge Function
// contentType: "application/json" for JSON, "image/bmp" for binary
static bool httpRequest(const char *method, const char *endpoint,
                        const char *contentType, const uint8_t *body,
                        size_t bodySize, char *responseOut,
                        size_t responseMaxSize) {
  if (!WiFi.isConnected()) {
    LOG_ERROR("WiFi not connected!");
    return false;
  }

  HTTPClient http;
  String url = String(SUPABASE_URL) + endpoint;

  LOG_DEBUG("[HTTP] %s %s (size: %d)", method, url.c_str(), bodySize);

  // CRITICAL: Disable TLS session caching. After a wipe/soft reboot, stale TLS 
  // session keys will cause the SSL handshake to fail for the new pairing request.
  http.setReuse(false);
  http.begin(url);

  // Increase timeout since Supabase edge functions can take >5s to cold start
  // and image binary uploads require stable connections
  http.setTimeout(30000);

  http.addHeader("Content-Type", contentType);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("apikey", SUPABASE_ANON_KEY);

  int httpCode = -1;

  if (strcmp(method, "POST") == 0) {
    httpCode = http.POST((uint8_t *)body, bodySize);
  } else if (strcmp(method, "GET") == 0) {
    httpCode = http.GET();
  }

  bool success = false;
  if (httpCode > 0) {
    LOG_DEBUG("[HTTP] Response code: %d", httpCode);

    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      String payload = http.getString();
      size_t copySize = min(payload.length(), responseMaxSize - 1);
      payload.toCharArray(responseOut, copySize + 1);
      responseOut[copySize] = '\0';
      success = true;
      LOG_DEBUG("[HTTP] Response: %s", responseOut);
    } else {
      LOG_ERROR("[HTTP] Unexpected response code: %d", httpCode);
      String payload = http.getString();
      LOG_ERROR("[HTTP] Error body: %s", payload.c_str());
    }
  } else {
    LOG_ERROR("[HTTP] Connection failed: %s",
              http.errorToString(httpCode).c_str());
  }

  http.end();
  return success;
}

// ============================================
// Pairing Flow
// ============================================

bool initCloud() {
  LOG_DEBUG("[Cloud] Initializing cloud module");

  // Open NVS namespace for reading
  prefs.begin("smartpen", false);

  // Load saved auth token
  String savedToken = prefs.getString("auth_token", "");
  if (savedToken.length() > 0) {
    strncpy(g_authToken, savedToken.c_str(), sizeof(g_authToken) - 1);
    g_authToken[sizeof(g_authToken) - 1] = '\0';
    LOG_DEBUG("[Cloud] Loaded saved auth token: %s", g_authToken);
  } else {
    LOG_DEBUG("[Cloud] No saved auth token found");
    g_authToken[0] = '\0';
  }

  prefs.end();
  return true;
}

char *startPairing() {
  LOG_DEBUG("[Pairing] Starting pairing process");

  if (!WiFi.isConnected()) {
    LOG_ERROR("[Pairing] WiFi not connected!");
    return NULL;
  }

  // Check if we have a valid code that hasn't expired (5 minute cooldown)
  unsigned long elapsed = millis() - g_pairingStartTime;
  if (g_pairingCode[0] != '\0' && elapsed < PAIRING_TIMEOUT) {
    unsigned long remaining = (PAIRING_TIMEOUT - elapsed) / 1000;
    LOG_DEBUG("[Pairing] Using existing code: %s (expires in %lu sec)",
              g_pairingCode, remaining);
    return g_pairingCode;
  }

  g_pairingStartTime = millis();

  // Send request to edge function - server will generate the code
  JsonDocument doc;
  doc["action"] = "start";
  doc["pen_id"] = getUniquePenID();

  String jsonStr;
  serializeJson(doc, jsonStr);

  char response[2048] = {0};
  bool ok = httpRequest("POST", "/functions/v1/smart-pen", "application/json",
                        (uint8_t *)jsonStr.c_str(), jsonStr.length(), response,
                        sizeof(response));

  if (ok) {
    JsonDocument respDoc;
    DeserializationError error = deserializeJson(respDoc, response);

    if (!error && respDoc["success"]) {
      // Use the code from server response, not local
      const char *serverCode = respDoc["code"];
      if (serverCode) {
        strncpy(g_pairingCode, serverCode, sizeof(g_pairingCode) - 1);
        g_pairingCode[sizeof(g_pairingCode) - 1] = '\0';
        Serial.printf("[Pairing] Code from server: %s (Pen: %s)\n", g_pairingCode, getUniquePenID());
        return g_pairingCode;
      }
    } else {
      Serial.printf("[Pairing] Server error: %s\n", response);
    }
  } else {
    Serial.println("[Pairing] HTTP POST failed!");
  }

  return NULL;
}

char *checkPairingStatus(const char *code) {
  LOG_DEBUG("[Pairing] Checking status for code: %s", code);

  if (!WiFi.isConnected()) {
    LOG_ERROR("[Pairing] WiFi not connected!");
    return NULL;
  }

  // Check if pairing was confirmed
  JsonDocument doc;
  doc["action"] = "status";
  doc["pen_id"] = getUniquePenID();
  doc["code"] = code;

  String jsonStr;
  serializeJson(doc, jsonStr);

  char response[1024] = {0};
  bool ok = httpRequest("POST", "/functions/v1/smart-pen", "application/json",
                        (uint8_t *)jsonStr.c_str(), jsonStr.length(), response,
                        sizeof(response));

  if (ok) {
    JsonDocument respDoc;
    DeserializationError error = deserializeJson(respDoc, response);

    if (!error && respDoc["paired"]) {
      const char *token = respDoc["auth_token"];
      if (token) {
        setAuthToken(token);
        LOG_DEBUG("[Pairing] Successfully paired! Token: %s", g_authToken);
        return g_authToken;
      }
    }
  }

  return NULL;
}

void setAuthToken(const char *token) {
  if (token) {
    strncpy(g_authToken, token, sizeof(g_authToken) - 1);
    g_authToken[sizeof(g_authToken) - 1] = '\0';
    LOG_DEBUG("[Auth] Token set: %s", g_authToken);

    // Save token to NVS for persistence across reboots
    prefs.begin("smartpen", false);
    prefs.putString("auth_token", token);
    prefs.end();
    LOG_DEBUG("[Auth] Token saved to NVS");
  }
}

const char *getAuthToken() {
  return (g_authToken[0] != '\0') ? g_authToken : NULL;
}

void clearAuthToken() {
  g_authToken[0] = '\0';
  g_pairingCode[0] = '\0';
  g_pairingStartTime = 0;

  // Clear from NVS
  prefs.begin("smartpen", false);
  prefs.remove("auth_token");
  prefs.end();

  LOG_DEBUG("[Auth] Token cleared - device unpaired");
}

// ============================================
// Image Upload
// ============================================

char *uploadImage(const uint8_t *imageData, size_t imageSize) {
  LOG_DEBUG("[Upload] Starting image upload (%d bytes)", imageSize);

  if (!WiFi.isConnected()) {
    LOG_ERROR("[Upload] WiFi not connected! Stashing to SD Queue.");
    String filename = saveImageToSD(imageData, imageSize);
    if (filename.length() > 0) {
      snprintf(g_responseBuffer, sizeof(g_responseBuffer),
               "{\"success\":true,\"queued\":true,\"file\":\"%s\"}",
               filename.c_str());
      return g_responseBuffer;
    } else {
      return NULL; // SD also failed
    }
  }

  const char *token = getAuthToken();
  if (!token) {
    LOG_ERROR("[Upload] NOT PAIRED! No auth token available.");
    LOG_ERROR("[Upload] Please pair the device first via the web interface.");
    return NULL;
  }

  LOG_DEBUG("[Upload] Auth token: %s", token);

  // Build endpoint with auth token
  String endpoint = String("/functions/v1/smart-pen?token=") + getAuthToken();

  // Send image as binary data - use global buffer to avoid stack overflow
  memset(g_responseBuffer, 0, sizeof(g_responseBuffer));
  bool ok = httpRequest("POST", endpoint.c_str(), "image/jpeg", imageData,
                        imageSize, g_responseBuffer, sizeof(g_responseBuffer));

  if (ok) {
    // Parse response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, g_responseBuffer);

    if (!error && doc["success"]) {
      LOG_DEBUG("[Upload] Upload successful!");

      // Safely log OCR results with null checks
      const char *ocrText = doc["ocr_text"] | "(none)";
      const char *summary = doc["summary"] | "(none)";

      LOG_DEBUG("[OCR] Text length: %d chars", strlen(ocrText));
      LOG_DEBUG("[Summary] %s", summary);

      if (doc["item_id"]) {
        LOG_DEBUG("[Item] ID: %d", doc["item_id"].as<int>());
      }

      // Copy full response to buffer
      serializeJson(doc, g_responseBuffer, sizeof(g_responseBuffer));
      return g_responseBuffer;
    } else {
      LOG_ERROR("[Upload] Server error response");
      return NULL;
    }
  }

  LOG_ERROR("[Upload] HTTP request failed");
  return NULL;
}


// Make HTTP request from a file stream to save memory during SD sync
static bool httpRequestStream(const char *endpoint, File &file, size_t fileSize, char *responseOut, size_t responseMaxSize) {
  if (!WiFi.isConnected()) return false;

  HTTPClient http;
  String url = String(SUPABASE_URL) + endpoint;
  
  LOG_DEBUG("[HTTP] POST Stream %s (size: %d)", url.c_str(), fileSize);
  
  // We MUST use a secure client for Supabase HTTPS routing
  http.setReuse(false);
  http.setTimeout(30000); // 30s timeout for slow SD reads and chunk uploads
  http.begin(url);
  
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("apikey", SUPABASE_ANON_KEY);

  // Send the file stream directly without buffering it all into RAM
  int httpCode = http.sendRequest("POST", &file, fileSize);

  bool success = false;
  if (httpCode > 0) {
    LOG_DEBUG("[HTTP] Response code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      String payload = http.getString();
      size_t copySize = min(payload.length(), responseMaxSize - 1);
      payload.toCharArray(responseOut, copySize + 1);
      responseOut[copySize] = '\0';
      success = true;
    } else {
      LOG_ERROR("[HTTP] Server Rejected File: %s", http.getString().c_str());
    }
  } else {
    LOG_ERROR("[HTTP] Stream Connection failed: %s", http.errorToString(httpCode).c_str());
  }

  http.end();
  return success;
}

void syncPendingQueue() {
  if (!WiFi.isConnected()) return;
  const char *token = getAuthToken();
  if (!token) return;

  String filename = getNextPendingUpload();
  if (filename.length() == 0) return; // Queue empty

  LOG_DEBUG("[Sync] Found pending offline upload: %s", filename.c_str());

  // Instead of readImageFromSD which mallocs the whole file, just open it!
  File file = SD.open(filename.c_str(), FILE_READ);
  if (!file || file.size() == 0) {
    LOG_ERROR("[Sync] Corrupt SD file, deleting: %s", filename.c_str());
    if (file) file.close();
    deleteImageFromSD(filename);
    return;
  }
  
  size_t fileSize = file.size();

  // Build endpoint with auth token
  String endpoint = String("/functions/v1/smart-pen?token=") + token;
  memset(g_responseBuffer, 0, sizeof(g_responseBuffer));

  // Stream directly from SD to WiFi
  bool ok = httpRequestStream(endpoint.c_str(), file, fileSize, g_responseBuffer, sizeof(g_responseBuffer));
  
  file.close(); // Very important to close before deleting!

  if (ok) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, g_responseBuffer);

    if (!error && doc["success"]) {
      LOG_DEBUG("[Sync] Successfully pushed %s, deleting from SD", filename.c_str());
      deleteImageFromSD(filename);
    } else {
      LOG_ERROR("[Sync] Server rejected the file, keeping it in queue for now.");
    }
  } else {
    LOG_ERROR("[Sync] HTTP connection failed during stream sync round");
  }
}