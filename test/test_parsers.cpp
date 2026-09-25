#include "doctest.h"
#include "parsers.h"

#include <fstream>
#include <sstream>
#include <string>

static std::string readFixture(const char* name) {
  std::ifstream in(std::string(FIXTURE_DIR) + "/" + name);
  REQUIRE_MESSAGE(in.good(), "missing fixture ", name);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static JsonDocument parseWithFilter(const std::string& json, const JsonDocument& filter) {
  JsonDocument doc;
  auto err = deserializeJson(doc, json, DeserializationOption::Filter(filter));
  REQUIRE_MESSAGE(!err, err.c_str());
  return doc;
}

static const geo::LatLon kHome{32.7763, -79.9327};

TEST_CASE("parseAircraft reads every flight from a real adsb.lol response") {
  JsonDocument doc = parseWithFilter(readFixture("adsb_lol_point.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  CHECK(result.ok);
  CHECK(result.flights.size() == 10);
}

TEST_CASE("parseAircraft sorts flights nearest first and computes distance + bearing") {
  JsonDocument doc = parseWithFilter(readFixture("adsb_lol_point.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  REQUIRE(result.flights.size() == 10);
  const Flight& first = result.flights.front();
  CHECK(std::string(first.callsign) == "N21HL");
  CHECK(first.distNm == doctest::Approx(6.75).epsilon(0.01));
  CHECK(first.bearingDeg == doctest::Approx(323.2).epsilon(0.01));
  for (size_t i = 1; i < result.flights.size(); i++) {
    CHECK(result.flights[i - 1].distNm <= result.flights[i].distNm);
  }
}

TEST_CASE("parseAircraft copies the fields the screen needs") {
  JsonDocument doc = parseWithFilter(readFixture("adsb_lol_point.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  const Flight* aal = nullptr;
  for (const auto& f : result.flights)
    if (std::string(f.hex) == "abdeec") aal = &f;
  REQUIRE(aal != nullptr);
  CHECK(std::string(aal->callsign) == "AAL2968");
  CHECK(std::string(aal->type).size() > 0);
  CHECK(aal->hasAltitude);
  CHECK_FALSE(aal->onGround);
  CHECK(aal->hasTrack);
  CHECK(aal->gsKt > 0);
}

TEST_CASE("parseAircraft respects maxFlights") {
  JsonDocument doc = parseWithFilter(readFixture("adsb_lol_point.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 3);
  CHECK(result.flights.size() == 3);
  CHECK(result.totalInRange == 10);
}

TEST_CASE("parseAircraft handles the adsb.fi format too") {
  JsonDocument doc = parseWithFilter(readFixture("adsb_fi_point.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  CHECK(result.ok);
  CHECK(result.flights.size() > 0);
}

TEST_CASE("parseAircraft handles tricky real-world data") {
  JsonDocument doc = parseWithFilter(readFixture("edge_cases.json"), parsers::aircraftFilter());
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  REQUIRE(result.ok);

  auto find = [&](const char* hex) -> const Flight* {
    for (const auto& f : result.flights)
      if (std::string(f.hex) == hex) return &f;
    return nullptr;
  };

  SUBCASE("on the ground") {
    const Flight* f = find("a00001");
    REQUIRE(f);
    CHECK(f->onGround);
    CHECK(f->altFt == 0);
  }
  SUBCASE("no position is skipped") { CHECK(find("a00002") == nullptr); }
  SUBCASE("no callsign falls back to registration") {
    const Flight* f = find("a00003");
    REQUIRE(f);
    CHECK(std::string(f->callsign) == "N123AB");
    CHECK(f->vrateFpm == -300);
  }
  SUBCASE("emergency squawk kept") {
    const Flight* f = find("a00004");
    REQUIRE(f);
    CHECK(std::string(f->squawk) == "7700");
  }
  SUBCASE("outside the radius is dropped") { CHECK(find("a00005") == nullptr); }
  SUBCASE("TIS-B style hex with ~ is kept") { CHECK(find("~0fffff") != nullptr); }
  SUBCASE("missing track is flagged, description copied") {
    const Flight* f = find("a00007");
    REQUIRE(f);
    CHECK_FALSE(f->hasTrack);
    CHECK(std::string(f->desc) == "EMBRAER ERJ-170-200 LR");
    CHECK(std::string(f->owner) == "REPUBLIC AIRWAYS INC");
  }
}

TEST_CASE("parseAircraft reports an error when 'ac' is missing") {
  JsonDocument doc;
  deserializeJson(doc, R"({"error":"rate limited"})");
  auto result = parsers::parseAircraft(doc, kHome, 40.0, 50);
  CHECK_FALSE(result.ok);
  CHECK(result.flights.empty());
}

TEST_CASE("parseRoute reads airline, origin and destination") {
  JsonDocument doc;
  REQUIRE(!deserializeJson(doc, readFixture("adsbdb_callsign.json")));
  RouteInfo route = parsers::parseRoute(doc);
  REQUIRE(route.valid);
  CHECK(std::string(route.airline) == "American Airlines");
  CHECK(std::string(route.origin.iata) == "CLT");
  CHECK(std::string(route.origin.city) == "Charlotte");
  CHECK(route.origin.pos.lat == doctest::Approx(35.214));
  CHECK(std::string(route.dest.iata) == "EWR");
  CHECK(std::string(route.dest.city) == "New York");
}

TEST_CASE("parseRoute marks unknown callsigns as invalid") {
  JsonDocument doc;
  REQUIRE(!deserializeJson(doc, readFixture("adsbdb_callsign_unknown.json")));
  CHECK_FALSE(parsers::parseRoute(doc).valid);
}

TEST_CASE("parseAircraftInfo reads maker, owner and photo") {
  JsonDocument doc;
  REQUIRE(!deserializeJson(doc, readFixture("adsbdb_aircraft.json")));
  AircraftInfo info = parsers::parseAircraftInfo(doc);
  REQUIRE(info.valid);
  CHECK(std::string(info.manufacturer) == "Boeing");
  CHECK(std::string(info.owner) == "American Airlines");
  CHECK(std::string(info.photoUrl).find("https://") == 0);
}

TEST_CASE("parseAircraftInfo is invalid for an empty document") {
  JsonDocument doc;
  CHECK_FALSE(parsers::parseAircraftInfo(doc).valid);
}

TEST_CASE("parseGeolocation reads position, city and UTC offset") {
  JsonDocument doc;
  REQUIRE(!deserializeJson(doc, readFixture("ipwhois.json")));
  Geolocation geo = parsers::parseGeolocation(doc);
  REQUIRE(geo.valid);
  CHECK(geo.pos.lat == doctest::Approx(37.339).epsilon(0.001));
  CHECK(std::string(geo.city) == "San Jose");
  CHECK(geo.utcOffsetSec == -25200);
}

TEST_CASE("parseGeolocation is invalid when success is false") {
  JsonDocument doc;
  deserializeJson(doc, R"({"success":false,"message":"Invalid IP"})");
  CHECK_FALSE(parsers::parseGeolocation(doc).valid);
}
