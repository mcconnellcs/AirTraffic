#include "parsers.h"

#include <string.h>

#include <algorithm>

#include "format.h"

namespace parsers {

namespace {

// Safe string copy: never writes past the end, always ends with '\0'.
template <size_t N>
void copyText(char (&dest)[N], JsonVariantConst value) {
  const char* text = value.as<const char*>();
  if (text == nullptr) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, text, N - 1);
  dest[N - 1] = '\0';
}

template <size_t N>
void copyTrimmed(char (&dest)[N], JsonVariantConst value) {
  fmt::Label label = fmt::trimmed(value.as<const char*>());
  strncpy(dest, label.c_str(), N - 1);
  dest[N - 1] = '\0';
}

Flight readFlight(JsonObjectConst ac, geo::LatLon home) {
  Flight f{};
  copyText(f.hex, ac["hex"]);
  copyTrimmed(f.callsign, ac["flight"]);
  copyText(f.reg, ac["r"]);
  copyText(f.type, ac["t"]);
  copyText(f.desc, ac["desc"]);
  copyText(f.owner, ac["ownOp"]);
  copyText(f.squawk, ac["squawk"]);
  copyText(f.category, ac["category"]);
  if (f.callsign[0] == '\0') copyTrimmed(f.callsign, ac["r"]);
  if (f.callsign[0] == '\0') copyText(f.callsign, ac["hex"]);

  f.pos = {ac["lat"].as<double>(), ac["lon"].as<double>()};

  JsonVariantConst alt = ac["alt_baro"];
  if (alt.is<const char*>()) {  // the feed sends the word "ground" instead of a number
    f.onGround = strcmp(alt.as<const char*>(), "ground") == 0;
    f.hasAltitude = f.onGround;
    f.altFt = 0;
  } else if (alt.is<int32_t>() || alt.is<float>()) {
    f.altFt = alt.as<int32_t>();
    f.hasAltitude = true;
  }

  f.gsKt = ac["gs"] | 0.0f;
  f.hasTrack = ac["track"].is<float>() || ac["track"].is<int>();
  f.trackDeg = ac["track"] | 0.0f;
  f.vrateFpm = static_cast<int16_t>(ac["baro_rate"] | (ac["geom_rate"] | 0));

  f.distNm = static_cast<float>(geo::distanceNm(home, f.pos));
  f.bearingDeg = static_cast<float>(geo::bearingDeg(home, f.pos));
  return f;
}

Airport readAirport(JsonObjectConst ap) {
  Airport a{};
  copyText(a.iata, ap["iata_code"]);
  copyText(a.icao, ap["icao_code"]);
  copyText(a.city, ap["municipality"]);
  copyText(a.name, ap["name"]);
  a.pos = {ap["latitude"] | 0.0, ap["longitude"] | 0.0};
  return a;
}

}  // namespace

JsonDocument aircraftFilter() {
  JsonDocument filter;
  JsonObject ac = filter["ac"].add<JsonObject>();
  for (const char* key : {"hex", "flight", "r", "t", "desc", "ownOp", "squawk", "category", "lat",
                          "lon", "alt_baro", "gs", "track", "baro_rate", "geom_rate"}) {
    ac[key] = true;
  }
  return filter;
}

AircraftResult parseAircraft(const JsonDocument& doc, geo::LatLon home, double maxRangeNm,
                             size_t maxFlights) {
  AircraftResult result{false, {}, 0};
  JsonArrayConst list = doc["ac"].as<JsonArrayConst>();
  if (list.isNull()) return result;

  std::vector<Flight> flights;
  flights.reserve(list.size());
  for (JsonObjectConst ac : list) {
    if (!ac["lat"].is<double>() || !ac["lon"].is<double>()) continue;  // no position yet
    Flight f = readFlight(ac, home);
    if (f.distNm > maxRangeNm) continue;
    flights.push_back(f);
  }

  std::sort(flights.begin(), flights.end(),
            [](const Flight& a, const Flight& b) { return a.distNm < b.distNm; });
  result.totalInRange = flights.size();
  if (flights.size() > maxFlights) flights.resize(maxFlights);
  result.flights = std::move(flights);
  result.ok = true;
  return result;
}

RouteInfo parseRoute(const JsonDocument& doc) {
  RouteInfo route{};
  JsonObjectConst fr = doc["response"]["flightroute"];
  if (fr.isNull()) return route;
  copyText(route.airline, fr["airline"]["name"]);
  copyText(route.airlineIcao, fr["airline"]["icao"]);
  route.origin = readAirport(fr["origin"]);
  route.dest = readAirport(fr["destination"]);
  route.valid = route.origin.iata[0] != '\0' || route.dest.iata[0] != '\0';
  return route;
}

AircraftInfo parseAircraftInfo(const JsonDocument& doc) {
  AircraftInfo info{};
  JsonObjectConst ac = doc["response"]["aircraft"];
  if (ac.isNull()) return info;
  copyText(info.manufacturer, ac["manufacturer"]);
  copyText(info.model, ac["type"]);
  copyText(info.owner, ac["registered_owner"]);
  copyText(info.photoUrl, ac["url_photo_thumbnail"]);
  info.valid = true;
  return info;
}

Geolocation parseGeolocation(const JsonDocument& doc) {
  Geolocation geo{};
  if (!(doc["success"] | false)) return geo;
  if (!doc["latitude"].is<double>() || !doc["longitude"].is<double>()) return geo;
  geo.pos = {doc["latitude"].as<double>(), doc["longitude"].as<double>()};
  copyText(geo.city, doc["city"]);
  geo.utcOffsetSec = doc["timezone"]["offset"] | 0;
  geo.valid = true;
  return geo;
}

}  // namespace parsers
