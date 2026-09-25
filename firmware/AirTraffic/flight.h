// =============================================================================
//  flight.h  —  What we know about one aircraft
// =============================================================================
//  Every plane broadcasts its position a couple of times per second using a
//  radio system called ADS-B. Volunteers around the world receive those
//  messages and share them on free websites. We download them as JSON text
//  and copy the useful bits into these simple structs.
// =============================================================================
#pragma once

#include <stdint.h>

#include "geo.h"

struct Flight {
  char hex[8];        // unique radio address of the plane, e.g. "ad727d"
  char callsign[10];  // what air traffic control calls it, e.g. "AAL1699"
  char reg[12];       // registration painted on the tail, e.g. "N966AN"
  char type[6];       // ICAO type code, e.g. "B738" = Boeing 737-800
  char desc[32];      // long type name if the feed has it
  char owner[32];     // operator name if the feed has it
  char squawk[6];     // 4-digit transponder code (7700 = emergency!)
  char category[4];   // size class, e.g. "A3" = large airliner

  geo::LatLon pos;    // where it is
  int32_t altFt;      // barometric altitude in feet
  bool hasAltitude;
  bool onGround;
  float gsKt;         // ground speed in knots (nautical miles per hour)
  float trackDeg;     // direction of travel, 0 = north, 90 = east
  bool hasTrack;
  int16_t vrateFpm;   // climb (+) or descent (-) in feet per minute

  float distNm;       // distance from home
  float bearingDeg;   // direction from home
};

struct Airport {
  char iata[4];       // 3-letter code on your luggage tag, e.g. "CLT"
  char icao[5];       // 4-letter code pilots use, e.g. "KCLT"
  char city[28];
  char name[48];
  geo::LatLon pos;
};

struct RouteInfo {
  bool valid;
  char airline[36];
  char airlineIcao[4];
  Airport origin;
  Airport dest;
};

struct AircraftInfo {
  bool valid;
  char manufacturer[24];
  char model[32];
  char owner[40];
  char photoUrl[128];
};

struct Geolocation {
  bool valid;
  geo::LatLon pos;
  char city[32];
  int32_t utcOffsetSec;
};
