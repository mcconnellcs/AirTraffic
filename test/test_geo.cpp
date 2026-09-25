#include "doctest.h"
#include "geo.h"

using geo::LatLon;

static const LatLon kHome{32.7763, -79.9327};  // Charleston, SC

TEST_CASE("distanceNm matches the distance reported by the ADS-B API") {
  // Reference values are the "dst" field adsb.lol computed for the same point.
  CHECK(geo::distanceNm(kHome, {32.620651, -80.67854}) == doctest::Approx(38.746).epsilon(0.003));
  CHECK(geo::distanceNm(kHome, {32.866241, -80.012933}) == doctest::Approx(6.748).epsilon(0.003));
  CHECK(geo::distanceNm(kHome, {33.344559, -79.559418}) == doctest::Approx(38.961).epsilon(0.003));
}

TEST_CASE("distanceNm is zero for the same point and symmetric") {
  CHECK(geo::distanceNm(kHome, kHome) == doctest::Approx(0.0));
  LatLon p{40.6925, -74.1687};
  CHECK(geo::distanceNm(kHome, p) == doctest::Approx(geo::distanceNm(p, kHome)));
}

TEST_CASE("one degree of latitude is 60 nautical miles") {
  CHECK(geo::distanceNm({0, 0}, {1, 0}) == doctest::Approx(60.04).epsilon(0.002));
}

TEST_CASE("bearingDeg matches the direction reported by the ADS-B API") {
  CHECK(geo::bearingDeg(kHome, {32.620651, -80.67854}) == doctest::Approx(256.3).epsilon(0.003));
  CHECK(geo::bearingDeg(kHome, {33.289319, -79.83888}) == doctest::Approx(8.7).epsilon(0.02));
  CHECK(geo::bearingDeg(kHome, {32.636215, -79.632367}) == doctest::Approx(118.9).epsilon(0.003));
}

TEST_CASE("bearingDeg for the four compass directions") {
  CHECK(geo::bearingDeg({0, 0}, {1, 0}) == doctest::Approx(0.0));
  CHECK(geo::bearingDeg({0, 0}, {0, 1}) == doctest::Approx(90.0));
  CHECK(geo::bearingDeg({0, 0}, {-1, 0}) == doctest::Approx(180.0));
  CHECK(geo::bearingDeg({0, 0}, {0, -1}) == doctest::Approx(270.0));
}

TEST_CASE("project moves a point and round-trips with distance/bearing") {
  LatLon moved = geo::project(kHome, 45.0, 10.0);
  CHECK(geo::distanceNm(kHome, moved) == doctest::Approx(10.0).epsilon(0.001));
  CHECK(geo::bearingDeg(kHome, moved) == doctest::Approx(45.0).epsilon(0.001));
}

TEST_CASE("project with zero distance returns the same point") {
  LatLon same = geo::project(kHome, 123.0, 0.0);
  CHECK(same.lat == doctest::Approx(kHome.lat));
  CHECK(same.lon == doctest::Approx(kHome.lon));
}

TEST_CASE("normalizeDeg wraps any angle into 0..360") {
  CHECK(geo::normalizeDeg(0) == doctest::Approx(0));
  CHECK(geo::normalizeDeg(360) == doctest::Approx(0));
  CHECK(geo::normalizeDeg(-90) == doctest::Approx(270));
  CHECK(geo::normalizeDeg(725) == doctest::Approx(5));
}

TEST_CASE("angleDiffDeg gives the shortest signed turn") {
  CHECK(geo::angleDiffDeg(10, 350) == doctest::Approx(-20));
  CHECK(geo::angleDiffDeg(350, 10) == doctest::Approx(20));
  CHECK(geo::angleDiffDeg(90, 270) == doctest::Approx(180).epsilon(0.001));
  CHECK(geo::angleDiffDeg(45, 45) == doctest::Approx(0));
}

TEST_CASE("cardinal names the 8 compass points") {
  CHECK(std::string(geo::cardinal(0)) == "N");
  CHECK(std::string(geo::cardinal(22)) == "N");
  CHECK(std::string(geo::cardinal(23)) == "NE");
  CHECK(std::string(geo::cardinal(90)) == "E");
  CHECK(std::string(geo::cardinal(200)) == "S");
  CHECK(std::string(geo::cardinal(256)) == "W");
  CHECK(std::string(geo::cardinal(315)) == "NW");
  CHECK(std::string(geo::cardinal(359)) == "N");
  CHECK(std::string(geo::cardinal(-45)) == "NW");
}

TEST_CASE("toRadar puts north up and scales distance to the radius") {
  auto north = geo::toRadar(0, 10, 240, 240, 200, 20);
  CHECK(north.x == doctest::Approx(240));
  CHECK(north.y == doctest::Approx(140));
  auto east = geo::toRadar(90, 20, 240, 240, 200, 20);
  CHECK(east.x == doctest::Approx(440));
  CHECK(east.y == doctest::Approx(240));
  auto south = geo::toRadar(180, 5, 240, 240, 200, 20);
  CHECK(south.y == doctest::Approx(290));
}

TEST_CASE("routeProgress is 0 at origin, 1 at destination, ~0.5 in the middle") {
  LatLon clt{35.214, -80.943}, ewr{40.6925, -74.1687};
  CHECK(geo::routeProgress(clt, clt, ewr) == doctest::Approx(0.0));
  CHECK(geo::routeProgress(clt, ewr, ewr) == doctest::Approx(1.0));
  LatLon mid = geo::project(clt, geo::bearingDeg(clt, ewr), geo::distanceNm(clt, ewr) / 2);
  CHECK(geo::routeProgress(clt, mid, ewr) == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("routeProgress is safe when origin equals destination") {
  CHECK(geo::routeProgress(kHome, kHome, kHome) == doctest::Approx(0.0));
}
