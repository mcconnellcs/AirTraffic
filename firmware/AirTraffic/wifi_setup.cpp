#include "wifi_setup.h"

#include <WiFi.h>
#include <WiFiManager.h>

namespace wifisetup {

namespace {

constexpr int kConnectTimeoutS = 20;

WiFiManager manager;
AppSettings settings{};
bool haveNewSettings = false;

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
  manager.autoConnect(kHotspotName);
}

State process() {
  manager.process();
  if (WiFi.status() == WL_CONNECTED) return State::Connected;
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
