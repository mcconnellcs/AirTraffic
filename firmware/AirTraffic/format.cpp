#include "format.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace fmt {

namespace {
constexpr float kFeetToMeters = 0.3048f;
constexpr float kKnotsToKmh = 1.852f;
constexpr float kFpmToMps = 0.00508f;
constexpr int32_t kLevelFlightFpm = 100;    // below this we just say "LEVEL"
constexpr int32_t kTrendThresholdFpm = 300;  // below this the arrow stays flat
constexpr int32_t kFlightLevelFrom = 18000;  // US transition altitude
}  // namespace

Label Label::printf(const char* format, ...) {
  Label label{};
  va_list args;
  va_start(args, format);
  vsnprintf(label.text, kCapacity, format, args);
  va_end(args);
  return label;
}

Label thousands(int32_t value) {
  char digits[16];
  snprintf(digits, sizeof(digits), "%ld", static_cast<long>(labs(value)));
  const int len = static_cast<int>(strlen(digits));

  Label label{};
  int out = 0;
  if (value < 0) label.text[out++] = '-';
  for (int i = 0; i < len; i++) {
    if (i > 0 && (len - i) % 3 == 0) label.text[out++] = ',';
    label.text[out++] = digits[i];
  }
  label.text[out] = '\0';
  return label;
}

Label altitude(int32_t feet, bool onGround, Units units) {
  if (onGround) return Label::printf("GROUND");
  if (units == Units::Metric) {
    return Label::printf("%s m", thousands(lroundf(feet * kFeetToMeters)).c_str());
  }
  return Label::printf("%s ft", thousands(feet).c_str());
}

Label shortAltitude(int32_t feet, bool onGround) {
  if (onGround) return Label::printf("GND");
  if (feet >= kFlightLevelFrom) return Label::printf("FL%03ld", static_cast<long>(lroundf(feet / 100.0f)));
  if (feet < 1000) return Label::printf("%ld", static_cast<long>(feet));
  return thousands(lroundf(feet / 100.0f) * 100);
}

Label speed(float knots, Units units) {
  if (units == Units::Metric) return Label::printf("%ld km/h", lroundf(knots * kKnotsToKmh));
  return Label::printf("%ld kt", lroundf(knots));
}

Label distance(float nm, Units units) {
  const float value = units == Units::Metric ? nm * kKnotsToKmh : nm;
  const char* unit = units == Units::Metric ? "km" : "nm";
  if (value < 100.0f) return Label::printf("%.1f %s", value, unit);
  return Label::printf("%ld %s", lroundf(value), unit);
}

Label verticalRate(int32_t fpm, Units units) {
  if (labs(fpm) < kLevelFlightFpm) return Label::printf("LEVEL");
  const char* sign = fpm > 0 ? "+" : "-";
  if (units == Units::Metric) return Label::printf("%s%.1f m/s", sign, fabsf(fpm * kFpmToMps));
  return Label::printf("%s%s fpm", sign, thousands(labs(fpm)).c_str());
}

Label heading(float degrees) {
  long rounded = lroundf(degrees) % 360;
  if (rounded < 0) rounded += 360;
  return Label::printf("%03ld\xC2\xB0", rounded);
}

Label trimmed(const char* text) {
  Label label{};
  if (text == nullptr) return label;
  while (*text && isspace(static_cast<unsigned char>(*text))) text++;
  size_t len = strnlen(text, Label::kCapacity - 1);
  while (len > 0 && isspace(static_cast<unsigned char>(text[len - 1]))) len--;
  memcpy(label.text, text, len);
  label.text[len] = '\0';
  return label;
}

Label clock(int hour, int minute, bool use24h) {
  if (use24h) return Label::printf("%02d:%02d", hour, minute);
  const int h12 = hour % 12 == 0 ? 12 : hour % 12;
  return Label::printf("%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
}

int altitudeBand(int32_t feet, bool onGround) {
  if (onGround) return 0;
  if (feet < 1000) return 1;
  if (feet < 5000) return 2;
  if (feet < 15000) return 3;
  if (feet < 30000) return 4;
  return 5;
}

int verticalTrend(int32_t fpm) {
  if (fpm > kTrendThresholdFpm) return 1;
  if (fpm < -kTrendThresholdFpm) return -1;
  return 0;
}

bool isEmergencySquawk(const char* squawk) {
  if (squawk == nullptr) return false;
  return strcmp(squawk, "7500") == 0 || strcmp(squawk, "7600") == 0 || strcmp(squawk, "7700") == 0;
}

}  // namespace fmt
