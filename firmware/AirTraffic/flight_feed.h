// =============================================================================
//  flight_feed.h  —  Downloads flights in the background
// =============================================================================
//  The ESP32-S3 has TWO processor cores. Core 1 runs our animations; this
//  "task" runs on core 0 and does all the slow internet work, so the screen
//  never freezes while we wait for a website to answer.
//
//  The two cores share data through a "mutex" — a lock that makes sure only
//  one core reads or changes the shared data at a time.
// =============================================================================
#pragma once

#include <stdint.h>

#include <vector>

#include "app_settings.h"
#include "flight.h"

enum class FeedState { WaitingForWifi, Locating, Loading, Live, Error };

struct FeedStatus {
  FeedState state;
  const char* source;       // which website answered last
  const char* lastError;    // human readable, or "" if fine
  uint32_t lastUpdateMs;    // millis() of the last good download
  uint32_t totalInRange;    // planes in range (before trimming to the max)
  bool haveLocation;
  geo::LatLon home;
  char city[32];
};

struct Snapshot {
  uint32_t sequence;             // goes up by one with every download
  uint32_t receivedMs;
  std::vector<Flight> flights;   // nearest first
};

// Extra facts about one plane, fetched when you look at it.
struct Details {
  bool routeDone;
  RouteInfo route;
  bool aircraftDone;
  AircraftInfo aircraft;
};

namespace feed {

constexpr size_t kMaxFlights = 60;

void begin(const AppSettings& settings);
void updateSettings(const AppSettings& settings);

// Copies the newest download if it's newer than `lastSequence`.
bool latest(uint32_t lastSequence, Snapshot* out);

FeedStatus status();

// Ask for route/aircraft/photo details. `focus` = the plane being viewed now.
void wantDetails(const char* hex, const char* callsign, bool focus);
bool details(const char* hex, Details* out);

// The photo of the focused plane (JPEG bytes). Returns false if not ready.
// `use` is called while the photo is locked, so don't keep the pointer.
template <typename Fn>
bool withPhoto(const char* hex, Fn use);

// ---- internal (used by the template above) ----
bool lockPhoto(const char* hex, const uint8_t** data, size_t* length);
void unlockPhoto();

template <typename Fn>
bool withPhoto(const char* hex, Fn use) {
  const uint8_t* data = nullptr;
  size_t length = 0;
  if (!lockPhoto(hex, &data, &length)) return false;
  use(data, length);
  unlockPhoto();
  return true;
}

}  // namespace feed
