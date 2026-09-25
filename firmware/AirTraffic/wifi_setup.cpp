#include "wifi_setup.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <string.h>

namespace wifisetup {

namespace {

constexpr int kConnectTimeoutS = 20;

constexpr uint32_t kGiveUpConnectingMs = 30000;  // then offer the setup hotspot
constexpr int kWeakSignalDbm = -70;               // below this, look for a better access point
constexpr uint32_t kRoamCheckMs = 10 * 60 * 1000;  // ...but at most every 10 minutes

WiFiManager manager;
AppSettings settings{};
bool haveNewSettings = false;
bool connectingToSaved = false;
uint32_t connectStartMs = 0;
uint32_t lastRoamCheckMs = 0;

// Homes with more than one access point broadcast the same network name from
// each. The ESP32 joins whichever answers first and never moves, so it can sit
// three feet from one AP while talking to another across the house. This
// scans and joins the strongest one instead.
bool connectToStrongest() {
  wifi_config_t saved{};
  if (esp_wifi_get_config(WIFI_IF_STA, &saved) != ESP_OK || saved.sta.ssid[0] == '\0') return false;
  const char* ssid = reinterpret_cast<const char*>(saved.sta.ssid);
  const char* password = reinterpret_cast<const char*>(saved.sta.password);

  const int found = WiFi.scanNetworks();
  int best = -1;
  for (int i = 0; i < found; i++) {
    if (WiFi.SSID(i) == ssid && (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best))) best = i;
  }
  if (best < 0) {
    WiFi.scanDelete();
    return false;
  }
  uint8_t bssid[6];
  memcpy(bssid, WiFi.BSSID(best), sizeof(bssid));
  const int channel = WiFi.channel(best);
  Serial.printf("[wifi] %s: joining the strongest access point, %s on channel %d (%d dBm)\n", ssid,
                WiFi.BSSIDstr(best).c_str(), channel, WiFi.RSSI(best));
  WiFi.scanDelete();
  WiFi.begin(ssid, password, channel, bssid);
  return true;
}

char latText[16] = "";
char lonText[16] = "";
char unitsText[10] = "aviation";

WiFiManagerParameter latParam("lat", "Latitude (leave blank = automatic)", latText, 15);
WiFiManagerParameter lonParam("lon", "Longitude (leave blank = automatic)", lonText, 15);
WiFiManagerParameter unitsParam("units", "Units: aviation or metric", unitsText, 9);

void onSaveParams() {
  AppSettings next = settings;
  double lat = 0, lon = 0;
  const bool latOk = settings::parseCoordinate(latParam.getValue(), -90, 90, &lat);
  const bool lonOk = settings::parseCoordinate(lonParam.getValue(), -180, 180, &lon);
  if (latOk && lonOk) {
    next.autoLocation = false;
    next.home = {lat, lon};
  } else {
    next.autoLocation = true;  // blank or invalid: find our location automatically
  }
  next.units = strcasecmp(unitsParam.getValue(), "metric") == 0 ? fmt::Units::Metric
                                                                : fmt::Units::Aviation;
  settings::save(next);
  settings = next;
  haveNewSettings = true;
  Serial.printf("[wifi] settings saved: %s location, %s units\n",
                next.autoLocation ? "automatic" : "manual",
                next.units == fmt::Units::Metric ? "metric" : "aviation");
}

}  // namespace

void begin(const AppSettings& current) {
  settings = current;
  if (!current.autoLocation) {
    snprintf(latText, sizeof(latText), "%.4f", current.home.lat);
    snprintf(lonText, sizeof(lonText), "%.4f", current.home.lon);
    latParam.setValue(latText, 15);
    lonParam.setValue(lonText, 15);
  }
  unitsParam.setValue(current.units == fmt::Units::Metric ? "metric" : "aviation", 9);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // power saving makes the link laggy and drops HTTPS connections
  manager.setTitle("AirTraffic");
  manager.setClass("invert");  // dark page to match the radar
  manager.addParameter(&latParam);
  manager.addParameter(&lonParam);
  manager.addParameter(&unitsParam);
  manager.setSaveParamsCallback(onSaveParams);
  manager.setSaveConfigCallback(onSaveParams);
  manager.setConfigPortalBlocking(false);  // keep animating while we wait
  manager.setConnectTimeout(kConnectTimeoutS);
  manager.setShowInfoUpdate(false);
  if (manager.getWiFiIsSaved()) {
    // Connect in the background so the start-up animation keeps moving.
    // (WiFiManager's own autoConnect would freeze everything for a few seconds.)
    if (!connectToStrongest()) WiFi.begin();
    connectingToSaved = true;
    connectStartMs = millis();
    lastRoamCheckMs = millis();
  } else {
    manager.autoConnect(kHotspotName);  // nothing saved: opens the setup hotspot
  }
}

State process() {
  manager.process();
  if (WiFi.status() == WL_CONNECTED) {
    if (WiFi.RSSI() < kWeakSignalDbm && millis() - lastRoamCheckMs > kRoamCheckMs) {
      lastRoamCheckMs = millis();
      Serial.printf("[wifi] weak signal (%d dBm), looking for a better access point\n", WiFi.RSSI());
      connectToStrongest();
    }
    return State::Connected;
  }
  if (connectingToSaved && !manager.getConfigPortalActive() &&
      millis() - connectStartMs > kGiveUpConnectingMs) {
    connectingToSaved = false;  // the saved network isn't there: let the user pick another
    manager.startConfigPortal(kHotspotName);
  }
  if (manager.getConfigPortalActive()) return State::Portal;
  return State::Connecting;
}

void startPortal() {
  manager.setConfigPortalBlocking(false);
  manager.startConfigPortal(kHotspotName);
}

bool takeNewSettings(AppSettings* out) {
  if (!haveNewSettings) return false;
  haveNewSettings = false;
  *out = settings;
  return true;
}

}  // namespace wifisetup
