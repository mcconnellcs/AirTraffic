// =============================================================================
//  parsers.h  —  Read the JSON text that the flight websites send back
// =============================================================================
//  JSON looks like this:   {"ac": [ {"flight": "AAL1699", "alt_baro": 35000} ]}
//  ArduinoJson turns it into a "document" we can ask questions of, like
//  doc["ac"][0]["flight"]. These functions copy the answers into our structs.
//
//  They don't use Wi-Fi or the screen, so we can test them on a computer
//  (see the test/ folder).
// =============================================================================
#pragma once

#include <ArduinoJson.h>

#include <vector>

#include "flight.h"

namespace parsers {

struct AircraftResult {
  bool ok;                      // false if the reply wasn't what we expected
  std::vector<Flight> flights;  // nearest first
  size_t totalInRange;          // how many were in range before trimming to maxFlights
};

// Tells ArduinoJson to keep only the fields we use (saves lots of memory).
JsonDocument aircraftFilter();

// Reads an adsb.lol / adsb.fi reply.
AircraftResult parseAircraft(const JsonDocument& doc, geo::LatLon home, double maxRangeNm,
                             size_t maxFlights);

// Reads an adsbdb.com /v0/callsign reply.
RouteInfo parseRoute(const JsonDocument& doc);

// Reads an adsbdb.com /v0/aircraft reply.
AircraftInfo parseAircraftInfo(const JsonDocument& doc);

// Reads an ipwho.is reply (where are we?).
Geolocation parseGeolocation(const JsonDocument& doc);

}  // namespace parsers
