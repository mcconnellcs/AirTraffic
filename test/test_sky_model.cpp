#include "doctest.h"
#include "sky_model.h"

#include <string>

#include <cstring>

static const geo::LatLon kHome{32.7763, -79.9327};

static Flight makeFlight(const char* hex, double lat, double lon, float gs = 0, float track = 0) {
  Flight f{};
  std::strncpy(f.hex, hex, sizeof(f.hex) - 1);
  std::strncpy(f.callsign, hex, sizeof(f.callsign) - 1);
  f.pos = {lat, lon};
  f.gsKt = gs;
  f.trackDeg = track;
  f.hasTrack = gs > 0;
  f.hasAltitude = true;
  f.altFt = 10000;
  return f;
}

TEST_CASE("a new snapshot creates one track per aircraft") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9), makeFlight("bbb", 32.7, -80.0)}, 1000);
  CHECK(sky.tracks().size() == 2);
  REQUIRE(sky.find("aaa") != nullptr);
  CHECK(sky.find("zzz") == nullptr);
}

TEST_CASE("positions glide forward between updates using speed and heading") {
  SkyModel sky;
  // 360 kt heading due north = 6 nm per minute = 0.1 nm per second
  sky.applySnapshot({makeFlight("aaa", 32.0, -80.0, 360, 0)}, 0);
  const Track* t = sky.find("aaa");
  REQUIRE(t);
  geo::LatLon p0 = sky.positionAt(*t, 0);
  geo::LatLon p10 = sky.positionAt(*t, 10000);
  CHECK(geo::distanceNm(p0, p10) == doctest::Approx(1.0).epsilon(0.01));
  CHECK(geo::bearingDeg(p0, p10) == doctest::Approx(0.0).epsilon(0.01));
}

TEST_CASE("prediction stops after the max extrapolation time") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.0, -80.0, 360, 90)}, 0);
  const Track* t = sky.find("aaa");
  geo::LatLon far = sky.positionAt(*t, 10 * 60 * 1000);
  double maxNm = 360.0 / 3600.0 * (SkyModel::kMaxExtrapolateMs / 1000.0);
  CHECK(geo::distanceNm({32.0, -80.0}, far) <= maxNm + 0.01);
}

TEST_CASE("a new report blends smoothly instead of jumping") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.0, -80.0, 360, 0)}, 0);
  geo::LatLon before = sky.positionAt(*sky.find("aaa"), 10000);
  // New report is 0.5 nm east of where we predicted.
  geo::LatLon reported = geo::project(before, 90, 0.5);
  sky.applySnapshot({makeFlight("aaa", reported.lat, reported.lon, 360, 0)}, 10000);
  const Track* t = sky.find("aaa");
  geo::LatLon justAfter = sky.positionAt(*t, 10000);
  CHECK(geo::distanceNm(before, justAfter) < 0.01);  // no visible jump
  geo::LatLon settled = sky.positionAt(*t, 10000 + SkyModel::kBlendMs);
  geo::LatLon expected = geo::project(reported, 0, 360.0 / 3600.0 * SkyModel::kBlendMs / 1000.0);
  CHECK(geo::distanceNm(settled, expected) < 0.01);
}

TEST_CASE("trails remember the last reported positions") {
  SkyModel sky;
  for (int i = 0; i < Trail::kCapacity + 5; i++) {
    sky.applySnapshot({makeFlight("aaa", 32.0 + i * 0.01, -80.0)}, i * 10000);
  }
  const Track* t = sky.find("aaa");
  REQUIRE(t);
  CHECK(t->trail.size() == Trail::kCapacity);
  CHECK(t->trail.at(t->trail.size() - 1).lat == doctest::Approx(32.0 + (Trail::kCapacity + 4) * 0.01));
  CHECK(t->trail.at(0).lat == doctest::Approx(32.0 + 5 * 0.01));
}

TEST_CASE("new aircraft fade in, missing aircraft fade out then disappear") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9)}, 0);
  const Track* t = sky.find("aaa");
  CHECK(sky.opacityAt(*t, 0) == doctest::Approx(0.0f));
  CHECK(sky.opacityAt(*t, SkyModel::kFadeMs) == doctest::Approx(1.0f));
  CHECK(sky.isNew(*t, 100));

  sky.applySnapshot({}, 20000);
  t = sky.find("aaa");
  REQUIRE(t);  // still here, fading out
  CHECK(t->lostAtMs == 20000);
  CHECK(sky.opacityAt(*t, 20000 + SkyModel::kFadeMs / 2) == doctest::Approx(0.5f).epsilon(0.05));

  sky.prune(20000 + SkyModel::kFadeMs + 1);
  CHECK(sky.find("aaa") == nullptr);
}

TEST_CASE("an aircraft that comes back is no longer marked lost") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9)}, 0);
  sky.applySnapshot({}, 10000);
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9)}, 11000);
  CHECK(sky.find("aaa")->lostAtMs == 0);
}

TEST_CASE("nearest returns the closest active aircraft to home") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("far", 33.3, -79.9), makeFlight("near", 32.78, -79.93)}, 0);
  const Track* n = sky.nearest(kHome, 0);
  REQUIRE(n);
  CHECK(std::string(n->latest.hex) == "near");
}

TEST_CASE("nearest is null when the sky is empty") {
  SkyModel sky;
  CHECK(sky.nearest(kHome, 0) == nullptr);
}

TEST_CASE("activeCount ignores aircraft that are fading out") {
  SkyModel sky;
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9), makeFlight("bbb", 32.7, -79.9)}, 0);
  sky.applySnapshot({makeFlight("aaa", 32.8, -79.9)}, 10000);
  CHECK(sky.tracks().size() == 2);
  CHECK(sky.activeCount() == 1);
}
