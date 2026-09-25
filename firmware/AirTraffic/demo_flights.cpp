#include "demo_flights.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

namespace demo {

namespace {

constexpr double kPathNm = 60.0;  // each plane flies a 60 nm line, then starts over

struct Plan {
  const char* callsign;
  const char* reg;
  const char* type;
  const char* desc;
  const char* owner;
  float startBearing, startDistNm, headingDeg, gsKt;
  int32_t altFt;
  int16_t vrateFpm;
  const char* squawk;
  const char* category;
  bool onGround;
};

// A mix that shows off everything: airliners at every altitude, a helicopter,
// small planes, a heavy jet, one taxiing on the ground and one emergency.
const Plan kPlans[] = {
    {"UAL2232", "N37502", "B739", "BOEING 737-900ER", "UNITED AIRLINES", 200, 18, 20, 430, 34000, 0, "3521", "A3", false},
    {"N21HL", "N21HL", "C172", "CESSNA 172S", "", 330, 6, 95, 105, 2500, 300, "1200", "A1", false},
    {"JBU1371", "N979JB", "A321", "AIRBUS A321-231", "JETBLUE AIRWAYS", 60, 30, 235, 300, 11000, -1800, "2743", "A3", false},
    {"MEDIC7", "N911MD", "EC35", "AIRBUS EC135", "MEDICAL AIR", 120, 4, 300, 120, 900, 0, "1200", "A7", false},
    {"DAL857", "N855DN", "B739", "BOEING 737-900ER", "DELTA AIR LINES", 250, 35, 70, 455, 37000, 0, "6412", "A3", false},
    {"FDX1408", "N286FE", "B77L", "BOEING 777F", "FEDEX", 15, 24, 190, 280, 8000, -1500, "4022", "A5", false},
    {"SWA3899", "N8710M", "B38M", "BOEING 737 MAX 8", "SOUTHWEST AIRLINES", 280, 12, 350, 250, 6000, 2200, "5117", "A3", false},
    {"AAL2968", "N930AN", "B738", "BOEING 737-800", "AMERICAN AIRLINES", 175, 9, 210, 15, 0, 0, "2201", "A3", true},
    {"N425MG", "N425MG", "SR22", "CIRRUS SR22", "", 45, 14, 140, 165, 4500, 0, "1200", "A1", false},
    {"RPA3535", "N412YX", "E75L", "EMBRAER ERJ-175", "REPUBLIC AIRWAYS", 95, 38, 260, 420, 24000, -900, "1625", "A3", false},
    {"FFT2310", "N336FR", "A20N", "AIRBUS A320NEO", "FRONTIER AIRLINES", 300, 26, 110, 330, 15000, -2500, "7700", "A3", false},
    {"AAY167", "N253NV", "A320", "AIRBUS A320-214", "ALLEGIANT AIR", 140, 33, 315, 440, 31000, 0, "3106", "A3", false},
    {"BAW223", "G-XWBA", "A35K", "AIRBUS A350-1000", "BRITISH AIRWAYS", 355, 39, 40, 490, 39000, 0, "2000", "A5", false},
    {"N5150", "N5150", "PA28", "PIPER PA-28", "", 225, 7, 60, 95, 1500, 0, "1200", "A1", false},
};
constexpr size_t kPlanCount = sizeof(kPlans) / sizeof(kPlans[0]);

struct RoutePlan {
  const char* callsign;
  const char* airline;
  const char* airlineIcao;
  Airport origin;
  Airport dest;
};

const RoutePlan kRoutes[] = {
    {"UAL2232", "United Airlines", "UAL", {"MIA", "KMIA", "Miami", "Miami International", {25.7959, -80.2870}}, {"EWR", "KEWR", "New York", "Newark Liberty International", {40.6925, -74.1687}}},
    {"JBU1371", "JetBlue Airways", "JBU", {"BOS", "KBOS", "Boston", "Logan International", {42.3656, -71.0096}}, {"FLL", "KFLL", "Fort Lauderdale", "Fort Lauderdale-Hollywood International", {26.0726, -80.1527}}},
    {"DAL857", "Delta Air Lines", "DAL", {"ATL", "KATL", "Atlanta", "Hartsfield-Jackson Atlanta International", {33.6407, -84.4277}}, {"BWI", "KBWI", "Baltimore", "Baltimore/Washington International", {39.1754, -76.6684}}},
    {"FDX1408", "FedEx", "FDX", {"IND", "KIND", "Indianapolis", "Indianapolis International", {39.7173, -86.2944}}, {"MIA", "KMIA", "Miami", "Miami International", {25.7959, -80.2870}}},
    {"SWA3899", "Southwest Airlines", "SWA", {"TPA", "KTPA", "Tampa", "Tampa International", {27.9755, -82.5332}}, {"BWI", "KBWI", "Baltimore", "Baltimore/Washington International", {39.1754, -76.6684}}},
    {"AAL2968", "American Airlines", "AAL", {"CHS", "KCHS", "Charleston", "Charleston International", {32.8986, -80.0405}}, {"CLT", "KCLT", "Charlotte", "Charlotte Douglas International", {35.2140, -80.9431}}},
    {"RPA3535", "Republic Airways", "RPA", {"LGA", "KLGA", "New York", "LaGuardia", {40.7769, -73.8740}}, {"ATL", "KATL", "Atlanta", "Hartsfield-Jackson Atlanta International", {33.6407, -84.4277}}},
    {"FFT2310", "Frontier Airlines", "FFT", {"ORD", "KORD", "Chicago", "O'Hare International", {41.9742, -87.9073}}, {"CHS", "KCHS", "Charleston", "Charleston International", {32.8986, -80.0405}}},
    {"AAY167", "Allegiant Air", "AAY", {"FLL", "KFLL", "Fort Lauderdale", "Fort Lauderdale-Hollywood International", {26.0726, -80.1527}}, {"CVG", "KCVG", "Cincinnati", "Cincinnati/Northern Kentucky International", {39.0489, -84.6678}}},
    {"BAW223", "British Airways", "BAW", {"MIA", "KMIA", "Miami", "Miami International", {25.7959, -80.2870}}, {"LHR", "EGLL", "London", "Heathrow", {51.4700, -0.4543}}},
};

struct AircraftPlan {
  const char* callsign;
  const char* manufacturer;
  const char* model;
};

const AircraftPlan kAircraft[] = {
    {"UAL2232", "Boeing", "737-924ER"},  {"N21HL", "Cessna", "172S Skyhawk"},
    {"JBU1371", "Airbus", "A321-231"},   {"MEDIC7", "Airbus Helicopters", "EC135 P2+"},
    {"DAL857", "Boeing", "737-932ER"},   {"FDX1408", "Boeing", "777-FS2"},
    {"SWA3899", "Boeing", "737 MAX 8"},  {"AAL2968", "Boeing", "737-823"},
    {"N425MG", "Cirrus", "SR22 G6"},     {"RPA3535", "Embraer", "ERJ 170-200 LR"},
    {"FFT2310", "Airbus", "A320-251N"},  {"AAY167", "Airbus", "A320-214"},
    {"BAW223", "Airbus", "A350-1041"},   {"N5150", "Piper", "PA-28-181 Archer"},
};

template <size_t N>
void copyText(char (&dest)[N], const char* src) {
  strncpy(dest, src ? src : "", N - 1);
  dest[N - 1] = '\0';
}

Flight makeFlight(size_t index, const Plan& p, geo::LatLon home, uint32_t elapsedMs) {
  Flight f{};
  snprintf(f.hex, sizeof(f.hex), "demo%02u", static_cast<unsigned>(index + 1));
  copyText(f.callsign, p.callsign);
  copyText(f.reg, p.reg);
  copyText(f.type, p.type);
  copyText(f.desc, p.desc);
  copyText(f.owner, p.owner);
  copyText(f.squawk, p.squawk);
  copyText(f.category, p.category);

  // The plane starts at its start point and flies a straight 60 nm line that
  // is centred there, then jumps back to the line's beginning (it looks like
  // a new plane arriving).
  const double hours = elapsedMs / 3600000.0;
  const double along = fmod(p.gsKt * hours + kPathNm / 2, kPathNm);
  const geo::LatLon start = geo::project(home, p.startBearing, p.startDistNm);
  const geo::LatLon lineStart = geo::project(start, p.headingDeg + 180.0, kPathNm / 2);
  f.pos = p.onGround ? start : geo::project(lineStart, p.headingDeg, along);

  f.altFt = p.altFt;
  f.hasAltitude = true;
  f.onGround = p.onGround;
  f.gsKt = p.gsKt;
  f.trackDeg = p.headingDeg;
  f.hasTrack = true;
  f.vrateFpm = p.vrateFpm;
  f.distNm = static_cast<float>(geo::distanceNm(home, f.pos));
  f.bearingDeg = static_cast<float>(geo::bearingDeg(home, f.pos));
  return f;
}

}  // namespace

std::vector<Flight> flights(geo::LatLon home, uint32_t elapsedMs) {
  std::vector<Flight> result;
  result.reserve(kPlanCount);
  for (size_t i = 0; i < kPlanCount; i++) result.push_back(makeFlight(i, kPlans[i], home, elapsedMs));
  std::sort(result.begin(), result.end(),
            [](const Flight& a, const Flight& b) { return a.distNm < b.distNm; });
  return result;
}

RouteInfo route(const char* callsign) {
  RouteInfo r{};
  for (const RoutePlan& plan : kRoutes) {
    if (strcmp(plan.callsign, callsign) != 0) continue;
    r.valid = true;
    copyText(r.airline, plan.airline);
    copyText(r.airlineIcao, plan.airlineIcao);
    r.origin = plan.origin;
    r.dest = plan.dest;
    break;
  }
  return r;
}

AircraftInfo aircraft(const char* callsign) {
  AircraftInfo info{};
  for (const AircraftPlan& plan : kAircraft) {
    if (strcmp(plan.callsign, callsign) != 0) continue;
    info.valid = true;
    copyText(info.manufacturer, plan.manufacturer);
    copyText(info.model, plan.model);
    break;
  }
  return info;
}

}  // namespace demo
