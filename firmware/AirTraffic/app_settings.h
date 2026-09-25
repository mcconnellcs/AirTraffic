// =============================================================================
//  app_settings.h  —  Your choices, saved so they survive turning it off
// =============================================================================
//  The ESP32 has a tiny "notebook" in its flash memory called NVS
//  (Non-Volatile Storage). The Preferences library reads and writes it.
// =============================================================================
#pragma once

#include <stdint.h>

#include "format.h"
#include "geo.h"

// ---- Things you might like to change -----------------------------------------
constexpr uint32_t kRefreshMs = 10000;      // download new positions every 10 s
constexpr uint16_t kRangeChoicesNm[] = {10, 25, 50, 100};
constexpr uint16_t kDefaultRangeNm = 25;
constexpr uint8_t kDefaultBrightness = 200;  // 0..255
constexpr uint32_t kAmbientAfterMs = 60000;  // show nearest plane after 1 min idle
// -----------------------------------------------------------------------------

struct AppSettings {
  bool autoLocation;   // true = find our location from the internet address
  geo::LatLon home;    // used when autoLocation is false
  uint16_t rangeNm;    // radar radius
  fmt::Units units;
  bool clock24h;
  uint8_t brightness;
};

namespace settings {

AppSettings load();
void save(const AppSettings& s);
void eraseAll();  // forget everything, including Wi-Fi

// Validates text typed into the setup page. Returns false if it isn't a number
// in range, leaving `out` untouched.
bool parseCoordinate(const char* text, double minValue, double maxValue, double* out);

uint16_t nextRange(uint16_t current);

}  // namespace settings
