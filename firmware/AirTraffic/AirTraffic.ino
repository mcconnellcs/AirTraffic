// =============================================================================
//   AirTraffic  —  a personal flight radar for your desk
// =============================================================================
//   Board:  ESP32-4848S040 (4" 480x480 touch screen with an ESP32-S3)
//   Data:   live aircraft positions from adsb.lol / adsb.fi (free, no key)
//
//   How it works, in one breath:
//     1. Connect to Wi-Fi                          -> wifi_setup.cpp
//     2. Download nearby planes every 10 seconds   -> flight_feed.cpp
//     3. Guess where each plane is between updates -> sky_model.cpp
//     4. Draw a radar 30+ times a second           -> screen_radar.cpp
//     5. Tap a plane to see where it's going       -> screen_card.cpp
//
//   Arduino IDE settings (Tools menu):
//     Board:            ESP32S3 Dev Module
//     Flash Size:       16MB (128Mb)
//     Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
//     PSRAM:            OPI PSRAM          <- important!
//
//   Made to be tinkered with. Start with theme.h to change the colors.
// =============================================================================

#include "app.h"

// Drawing smooth text needs more working memory ("stack") than Arduino's default 8 KB.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// setup() runs once when the board powers on.
void setup() {
  app::begin();
}

// loop() runs forever, over and over. Each time round draws one frame.
void loop() {
  app::tick();
}
