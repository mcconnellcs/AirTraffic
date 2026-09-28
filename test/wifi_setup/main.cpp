// Run the real setup controller against simulated Wi-Fi/portal APIs.
// These checks exercise our state logic, not the radio or WiFiManager internals.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "WiFiManager.h"
#include "wifi_setup.h"

namespace settings {
AppSettings savedSettings{};
void save(const AppSettings& s) { savedSettings = s; }
}

TEST_CASE("setup remains visible online and saves the latest screen settings") {
  AppSettings initial{true, {}, 25, fmt::Units::Aviation, false, 200};
  WiFiManager::saved = false;
  WiFiManager::portal = false;
  WiFi.connection = 0;
  wifisetup::begin(initial);
  CHECK(wifisetup::process() == wifisetup::State::Portal);

  // Simulate completing the first connection, then changing screen settings.
  WiFiManager::portal = false;
  WiFi.connection = WL_CONNECTED;
  CHECK(wifisetup::process() == wifisetup::State::Connected);
  AppSettings current{false, {40.25, -74.5}, 50, fmt::Units::Metric, true, 180};
  wifisetup::startPortal(current);
  CHECK(wifisetup::process() == wifisetup::State::Portal);
  CHECK(std::string(WiFiManager::params.at("lat")->getValue()) == "40.2500");
  CHECK(std::string(WiFiManager::params.at("lon")->getValue()) == "-74.5000");
  CHECK(std::string(WiFiManager::params.at("units")->getValue()) == "metric");
  WiFiManager::saveParams();
  AppSettings result{};
  REQUIRE(wifisetup::takeNewSettings(&result));
  CHECK_FALSE(wifisetup::takeNewSettings(&result));
  CHECK(result.rangeNm == 50);
  CHECK(result.clock24h);
  CHECK(result.brightness == 180);
  CHECK(result.units == fmt::Units::Metric);
  CHECK_FALSE(result.autoLocation);
  CHECK(result.home.lat == doctest::Approx(40.25));

  // A subsequent automatic-location session must clear old coordinates.
  WiFiManager::portal = false;
  current.autoLocation = true;
  wifisetup::startPortal(current);
  CHECK(std::string(WiFiManager::params.at("lat")->getValue()).empty());
  CHECK(std::string(WiFiManager::params.at("lon")->getValue()).empty());
  WiFiManager::saveParams();
  REQUIRE(wifisetup::takeNewSettings(&result));
  CHECK(result.autoLocation);
}

TEST_CASE("unavailable saved Wi-Fi opens setup after thirty seconds") {
  WiFiManager::saved = true;
  WiFiManager::portal = false;
  WiFi.connection = 0;
  fakeNow = 100;
  wifisetup::begin({true, {}, 25, fmt::Units::Aviation, false, 200});
  CHECK(wifisetup::process() == wifisetup::State::Connecting);
  fakeNow += 30001;
  CHECK(wifisetup::process() == wifisetup::State::Portal);
}
