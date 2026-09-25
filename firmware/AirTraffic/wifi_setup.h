// =============================================================================
//  wifi_setup.h  —  Connect to Wi-Fi without typing passwords into the code
// =============================================================================
//  First time only: the board creates its own Wi-Fi hotspot called
//  "AirTraffic-Setup". Join it with a phone, a setup page pops up, pick your
//  home Wi-Fi and type its password. The board remembers it from then on.
//
//  Because the password is never in the code, it's safe to share your code
//  on GitHub.
// =============================================================================
#pragma once

#include "app_settings.h"

namespace wifisetup {

enum class State { Connecting, Portal, Connected };

constexpr const char* kHotspotName = "AirTraffic-Setup";

void begin(const AppSettings& current);
State process();     // call every loop()
void startPortal();  // open the setup hotspot on purpose (from the settings menu)

// True once after the user saved new settings on the setup page.
bool takeNewSettings(AppSettings* out);

}  // namespace wifisetup
