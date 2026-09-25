#include "sky_model.h"

#include <string.h>

#include "anim.h"

// ---- Trail ----------------------------------------------------------------

void Trail::push(geo::LatLon p) {
  points_[head_] = p;
  head_ = (head_ + 1) % kCapacity;
  if (count_ < kCapacity) count_++;
}

geo::LatLon Trail::at(int i) const {
  const int oldest = (head_ - count_ + kCapacity) % kCapacity;
  return points_[(oldest + i) % kCapacity];
}

// ---- SkyModel -------------------------------------------------------------

namespace {
constexpr double kMsPerHour = 3600.0 * 1000.0;
}

geo::LatLon SkyModel::predict(const Track& track, uint32_t nowMs) const {
  const Flight& f = track.latest;
  if (!f.hasTrack || f.gsKt <= 0 || f.onGround) return f.pos;
  uint32_t elapsed = nowMs > track.reportMs ? nowMs - track.reportMs : 0;
  if (elapsed > kMaxExtrapolateMs) elapsed = kMaxExtrapolateMs;
  return geo::project(f.pos, f.trackDeg, f.gsKt * (elapsed / kMsPerHour));
}

geo::LatLon SkyModel::positionAt(const Track& track, uint32_t nowMs) const {
  const geo::LatLon target = predict(track, nowMs);
  if (!track.blending) return target;
  const float t = anim::easeOutCubic(anim::progress(track.reportMs, nowMs, kBlendMs));
  if (t >= 1.0f) return target;
  // Move the old guess along too, so the blend doesn't slow the plane down.
  const geo::LatLon startNow = {
      track.blendFrom.lat + (target.lat - predict(track, track.reportMs).lat),
      track.blendFrom.lon + (target.lon - predict(track, track.reportMs).lon)};
  return {startNow.lat + (target.lat - startNow.lat) * t,
          startNow.lon + (target.lon - startNow.lon) * t};
}

void SkyModel::applySnapshot(const std::vector<Flight>& flights, uint32_t nowMs) {
  std::vector<Track> next;
  next.reserve(flights.size() + tracks_.size());

  for (const Flight& f : flights) {
    const Track* old = find(f.hex);
    Track t{};
    if (old != nullptr) {
      t = *old;
      t.blendFrom = positionAt(*old, nowMs);
      t.blending = true;
    } else {
      t.firstSeenMs = nowMs;
      t.blending = false;
    }
    t.latest = f;
    t.reportMs = nowMs;
    t.lostAtMs = 0;
    t.trail.push(f.pos);
    next.push_back(t);
  }

  // Planes missing from this download start fading out (we keep them briefly).
  for (const Track& old : tracks_) {
    bool stillHere = false;
    for (const Flight& f : flights) stillHere |= strcmp(f.hex, old.latest.hex) == 0;
    if (stillHere) continue;
    Track gone = old;
    if (gone.lostAtMs == 0) gone.lostAtMs = nowMs;
    next.push_back(gone);
  }

  tracks_ = std::move(next);
}

void SkyModel::prune(uint32_t nowMs) {
  std::vector<Track> kept;
  kept.reserve(tracks_.size());
  for (const Track& t : tracks_) {
    if (t.lostAtMs != 0 && nowMs - t.lostAtMs > kFadeMs) continue;
    kept.push_back(t);
  }
  tracks_ = std::move(kept);
}

float SkyModel::opacityAt(const Track& track, uint32_t nowMs) const {
  const float in = anim::progress(track.firstSeenMs, nowMs, kFadeMs);
  if (track.lostAtMs == 0) return in;
  return in * (1.0f - anim::progress(track.lostAtMs, nowMs, kFadeMs));
}

bool SkyModel::isNew(const Track& track, uint32_t nowMs) const {
  return nowMs - track.firstSeenMs < kNewMs;
}

const Track* SkyModel::find(const char* hex) const {
  for (const Track& t : tracks_) {
    if (strcmp(t.latest.hex, hex) == 0) return &t;
  }
  return nullptr;
}

const Track* SkyModel::nearest(geo::LatLon home, uint32_t nowMs) const {
  const Track* best = nullptr;
  double bestNm = 1e9;
  for (const Track& t : tracks_) {
    if (t.lostAtMs != 0) continue;
    const double d = geo::distanceNm(home, positionAt(t, nowMs));
    if (d < bestNm) {
      bestNm = d;
      best = &t;
    }
  }
  return best;
}

int SkyModel::activeCount() const {
  int count = 0;
  for (const Track& t : tracks_) count += t.lostAtMs == 0 ? 1 : 0;
  return count;
}
