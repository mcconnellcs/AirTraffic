#include "app_settings.h"

#include <Preferences.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include <stdlib.h>

namespace settings {

namespace {
constexpr const char* kNamespace = "airtraffic";
}

AppSettings load() {
  Preferences prefs;
  prefs.begin(kNamespace, true);  // true = read-only
  AppSettings s{};
  s.autoLocation = prefs.getBool("autoLoc", true);
  s.home = {prefs.getDouble("lat", 0.0), prefs.getDouble("lon", 0.0)};
  s.rangeNm = prefs.getUShort("range", kDefaultRangeNm);
  s.units = prefs.getBool("metric", false) ? fmt::Units::Metric : fmt::Units::Aviation;
  s.clock24h = prefs.getBool("clock24", false);
  s.brightness = prefs.getUChar("bright", kDefaultBrightness);
  prefs.end();

  bool validRange = false;
  for (uint16_t r : kRangeChoicesNm) validRange |= r == s.rangeNm;
  if (!validRange) s.rangeNm = kDefaultRangeNm;
  if (s.brightness < 20) s.brightness = 20;  // never fully dark by accident
  return s;
}

void save(const AppSettings& s) {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putBool("autoLoc", s.autoLocation);
  prefs.putDouble("lat", s.home.lat);
  prefs.putDouble("lon", s.home.lon);
  prefs.putUShort("range", s.rangeNm);
  prefs.putBool("metric", s.units == fmt::Units::Metric);
  prefs.putBool("clock24", s.clock24h);
  prefs.putUChar("bright", s.brightness);
  prefs.end();
}

void eraseAll() {
  WiFi.disconnect(true, true);  // forget the saved Wi-Fi network
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();
}

}  // namespace settings
