#include "doctest.h"
#include "demo_flights.h"

#include <string>

static const Flight* findCallsign(const std::vector<Flight>& list, const char* callsign) {
  for (const Flight& f : list)
    if (std::string(f.callsign) == callsign) return &f;
  return nullptr;
}

TEST_CASE("demo flights are all within radar range and sorted nearest first") {
  auto list = demo::flights(demo::kHome, 0);
  CHECK(list.size() == 14);
  for (size_t i = 0; i < list.size(); i++) {
    CHECK(list[i].distNm <= 45.0f);
    CHECK(std::string(list[i].hex).size() == 6);
    if (i > 0) CHECK(list[i - 1].distNm <= list[i].distNm);
  }
}

TEST_CASE("demo flights move at their ground speed") {
  auto before = demo::flights(demo::kHome, 0);
  auto after = demo::flights(demo::kHome, 60000);
  const Flight* a = findCallsign(before, "UAL2232");
  const Flight* b = findCallsign(after, "UAL2232");
  REQUIRE(a);
  REQUIRE(b);
  // 430 kt for one minute = 7.17 nm
  CHECK(geo::distanceNm(a->pos, b->pos) == doctest::Approx(430.0 / 60.0).epsilon(0.01));
  CHECK(geo::bearingDeg(a->pos, b->pos) == doctest::Approx(20.0).epsilon(0.02));
}

TEST_CASE("demo includes the special cases the screens handle") {
  auto list = demo::flights(demo::kHome, 0);
  int emergencies = 0, onGround = 0, helicopters = 0;
  for (const Flight& f : list) {
    emergencies += std::string(f.squawk) == "7700";
    onGround += f.onGround;
    helicopters += std::string(f.category) == "A7";
  }
  CHECK(emergencies == 1);
  CHECK(onGround == 1);
  CHECK(helicopters == 1);
}

TEST_CASE("a plane on the ground stays put") {
  auto a = demo::flights(demo::kHome, 0);
  auto b = demo::flights(demo::kHome, 300000);
  const Flight* fa = findCallsign(a, "AAL2968");
  const Flight* fb = findCallsign(b, "AAL2968");
  REQUIRE(fa);
  REQUIRE(fb);
  CHECK(geo::distanceNm(fa->pos, fb->pos) < 0.001);
}

TEST_CASE("demo routes exist for airline flights but not private planes") {
  RouteInfo r = demo::route("UAL2232");
  REQUIRE(r.valid);
  CHECK(std::string(r.origin.iata) == "MIA");
  CHECK(std::string(r.dest.city) == "New York");
  CHECK(std::string(r.airline) == "United Airlines");
  CHECK_FALSE(demo::route("N21HL").valid);
  CHECK_FALSE(demo::route("NOPE123").valid);
}

TEST_CASE("demo aircraft facts") {
  AircraftInfo info = demo::aircraft("BAW223");
  REQUIRE(info.valid);
  CHECK(std::string(info.manufacturer) == "Airbus");
  CHECK(std::string(info.photoUrl) == "");
  CHECK_FALSE(demo::aircraft("NOPE123").valid);
}
