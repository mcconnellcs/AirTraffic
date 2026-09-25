#include "geo.h"

#include <math.h>

namespace geo {

namespace {
constexpr double kPi = 3.14159265358979323846;
double toRad(double deg) { return deg * kPi / 180.0; }
double toDeg(double rad) { return rad * 180.0 / kPi; }
}  // namespace

double distanceNm(LatLon a, LatLon b) {
  const double lat1 = toRad(a.lat), lat2 = toRad(b.lat);
  const double dLat = lat2 - lat1;
  const double dLon = toRad(b.lon - a.lon);
  const double h = sin(dLat / 2) * sin(dLat / 2) +
                   cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);
  return 2 * kEarthRadiusNm * asin(fmin(1.0, sqrt(h)));
}

double bearingDeg(LatLon from, LatLon to) {
  const double lat1 = toRad(from.lat), lat2 = toRad(to.lat);
  const double dLon = toRad(to.lon - from.lon);
  const double y = sin(dLon) * cos(lat2);
  const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  return normalizeDeg(toDeg(atan2(y, x)));
}

LatLon project(LatLon from, double bearing, double distance) {
  const double angular = distance / kEarthRadiusNm;
  const double brg = toRad(bearing);
  const double lat1 = toRad(from.lat), lon1 = toRad(from.lon);
  const double lat2 = asin(sin(lat1) * cos(angular) + cos(lat1) * sin(angular) * cos(brg));
  const double lon2 =
      lon1 + atan2(sin(brg) * sin(angular) * cos(lat1), cos(angular) - sin(lat1) * sin(lat2));
  return {toDeg(lat2), toDeg(lon2)};
}

double normalizeDeg(double deg) {
  double wrapped = fmod(deg, 360.0);
  if (wrapped < 0) wrapped += 360.0;
  return wrapped;
}

double angleDiffDeg(double from, double to) {
  double diff = normalizeDeg(to - from);
  return diff > 180.0 ? diff - 360.0 : diff;
}

const char* cardinal(double bearing) {
  static const char* const kNames[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  const int index = static_cast<int>(floor(normalizeDeg(bearing) / 45.0 + 0.5)) % 8;
  return kNames[index];
}

ScreenPoint toRadar(double bearing, double distance, float centerX, float centerY,
                    float radiusPx, double rangeNm) {
  const double r = distance / rangeNm * radiusPx;
  const double a = toRad(bearing);
  return {static_cast<float>(centerX + r * sin(a)), static_cast<float>(centerY - r * cos(a))};
}

double routeProgress(LatLon origin, LatLon position, LatLon dest) {
  const double flown = distanceNm(origin, position);
  const double remaining = distanceNm(position, dest);
  const double total = flown + remaining;
  return total < 0.001 ? 0.0 : flown / total;
}

}  // namespace geo
