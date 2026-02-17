#ifndef CLOUD_H
#define CLOUD_H

#include <cstdint>
#include <cstddef>

// ============================================
// Cloud Initialization & Pairing
// ============================================
bool initCloud();

// Start pairing: returns 6-digit code
char* startPairing();

// Check if pairing was confirmed (returns auth token if paired)
char* checkPairingStatus(const char* code);

// ============================================
// Image Upload & Sync
// ============================================
// Upload captured image to Supabase
// Returns: JSON response with image_url, ocr_text, summary
char* uploadImage(const uint8_t* imageData, size_t imageSize);

// Retry failed uploads from queue
void syncPendingQueue();

// Get current auth token (returns NULL if not paired)
const char* getAuthToken();

// Set auth token (usually from pairing confirmation)
void setAuthToken(const char* token);

// Clear auth token (unpair from current account)
void clearAuthToken();

#endif