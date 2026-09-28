#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

inline uint32_t fakeNow = 0;
inline uint32_t millis() { return fakeNow; }
struct SerialFake {
  template <typename... Args> void printf(const char*, Args...) {}
};
inline SerialFake Serial;
constexpr int WIFI_STA = 1;
constexpr int WL_CONNECTED = 3;
struct WiFiFake {
  int connection = 0;
  void mode(int) {}
  void setSleep(bool) {}
  int scanNetworks() { return 0; }
  std::string SSID(int) { return ""; }
  int RSSI(int = 0) { return -40; }
  uint8_t* BSSID(int) { static uint8_t value[6]{}; return value; }
  int channel(int) { return 1; }
  std::string BSSIDstr(int) { return ""; }
  void scanDelete() {}
  void begin() {}
  void begin(const char*, const char*, int, uint8_t*) {}
  int status() { return connection; }
};
inline WiFiFake WiFi;
