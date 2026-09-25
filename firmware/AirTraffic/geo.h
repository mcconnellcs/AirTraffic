// =============================================================================
//  geo.h  —  Earth math: distances, directions and radar positions
// =============================================================================
//  The Earth is (almost) a ball, so the distance between two points isn't a
//  straight line on a flat map. The "haversine" formula measures along the
//  curved surface. Pilots measure distance in nautical miles (nm):
//  1 nm = 1 minute of latitude = 1.852 km.
// =============================================================================
#pragma once

namespace geo {

struct LatLon {
  double lat;  // degrees, + is north
  double lon;  // degrees, + is east
};

struct ScreenPoint {
  float x;
  float y;
};

constexpr double kEarthRadiusNm = 3440.065;

// How far apart two places are, in nautical miles.
double distanceNm(LatLon a, LatLon b);

// Which way to face at `from` to look toward `to` (0 = north, 90 = east).
double bearingDeg(LatLon from, LatLon to);

// Start at `from`, travel `distanceNm` toward `bearingDeg`: where do you end up?
LatLon project(LatLon from, double bearingDeg, double distanceNm);

// Wrap any angle into 0..360.
double normalizeDeg(double deg);

// Shortest turn from angle a to angle b, between -180 and +180.
double angleDiffDeg(double from, double to);

// "N", "NE", "E", ... for a bearing.
const char* cardinal(double bearingDeg);

// Position on a round radar screen: north is up, the edge is `rangeNm` away.
ScreenPoint toRadar(double bearingDeg, double distanceNm, float centerX, float centerY,
                    float radiusPx, double rangeNm);

// Same result, computed the quick way. Treats the area around `home` as flat,
// which is accurate to well under 1% out to 100 nm and avoids slow trig maths.
// Used every frame for every plane and every trail point.
ScreenPoint toRadarFast(LatLon home, LatLon p, float centerX, float centerY, float radiusPx,
                        double rangeNm);

// How far along its trip a plane is: 0 = just left, 1 = arrived.
double routeProgress(LatLon origin, LatLon position, LatLon dest);

}  // namespace geo
