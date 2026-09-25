// =============================================================================
//  sky_model.h  —  Keeps track of every plane between downloads
// =============================================================================
//  We only download new positions every ~10 seconds. If we drew planes only
//  at those positions they would "teleport". Instead we use DEAD RECKONING,
//  the same trick sailors used before GPS:
//
//      new position = last position + speed x time, in the direction of travel
//
//  When a fresh download arrives we blend from the guess to the real position
//  over one second, so nothing ever jumps. Planes that appear fade in, and
//  planes that leave fade out.
// =============================================================================
#pragma once

#include <stdint.h>

#include <vector>

#include "flight.h"

// The last few reported positions of one plane (drawn as a fading contrail).
class Trail {
 public:
  static constexpr int kCapacity = 10;

  void push(geo::LatLon p);
  int size() const { return count_; }
  geo::LatLon at(int i) const;  // 0 = oldest

 private:
  geo::LatLon points_[kCapacity] = {};
  int head_ = 0;
  int count_ = 0;
};

struct Track {
  Flight latest;           // most recent report
  uint32_t reportMs;       // when we received it
  uint32_t firstSeenMs;    // when this plane first appeared
  uint32_t lostAtMs;       // 0 while active; otherwise when it vanished
  geo::LatLon blendFrom;   // where we drew it just before the latest report
  bool blending;
  Trail trail;
};

class SkyModel {
 public:
  static constexpr uint32_t kMaxExtrapolateMs = 30000;  // stop guessing after 30 s
  static constexpr uint32_t kBlendMs = 1000;            // smooth correction time
  static constexpr uint32_t kFadeMs = 600;              // fade in / out time
  static constexpr uint32_t kNewMs = 4000;              // "new!" highlight time

  void applySnapshot(const std::vector<Flight>& flights, uint32_t nowMs);
  void prune(uint32_t nowMs);  // remove planes that have finished fading out

  geo::LatLon positionAt(const Track& track, uint32_t nowMs) const;
  float opacityAt(const Track& track, uint32_t nowMs) const;
  bool isNew(const Track& track, uint32_t nowMs) const;

  const std::vector<Track>& tracks() const { return tracks_; }
  const Track* find(const char* hex) const;
  const Track* nearest(geo::LatLon home, uint32_t nowMs) const;
  int activeCount() const;

 private:
  std::vector<Track> tracks_;
  geo::LatLon predict(const Track& track, uint32_t nowMs) const;
};
