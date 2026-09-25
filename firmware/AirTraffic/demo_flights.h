// =============================================================================
//  demo_flights.h  —  Pretend planes, for trying the radar without Wi-Fi
// =============================================================================
//  Type "demo" in the Serial Monitor and the radar fills with these 14
//  aircraft. They fly real-looking paths at real-looking speeds, so all the
//  animation code runs exactly as it would with live data.
// =============================================================================
#pragma once

#include <stdint.h>

#include <vector>

#include "flight.h"

namespace demo {

constexpr geo::LatLon kHome{32.7763, -79.9327};  // Charleston, SC
constexpr const char* kCity = "Demo mode";

// Where every demo plane is `elapsedMs` after the demo started, nearest first.
std::vector<Flight> flights(geo::LatLon home, uint32_t elapsedMs);

// Made-up but plausible route / aircraft facts for a demo callsign.
RouteInfo route(const char* callsign);
AircraftInfo aircraft(const char* callsign);

}  // namespace demo
