// =============================================================================
//  format.h  —  Turn numbers into friendly text like "35,000 ft" or "FL350"
// =============================================================================
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace fmt {

enum class Units { Aviation, Metric };  // Aviation = feet, knots, nautical miles

// A small piece of text that lives on the stack (no memory allocation needed).
struct Label {
  static constexpr size_t kCapacity = 32;
  char text[kCapacity];

  const char* c_str() const { return text; }
  static Label printf(const char* format, ...) __attribute__((format(printf, 1, 2)));
};

Label thousands(int32_t value);                                // 35000 -> "35,000"
Label altitude(int32_t feet, bool onGround, Units units);      // "35,000 ft"
Label shortAltitude(int32_t feet, bool onGround);              // "FL350" / "4,600"
Label speed(float knots, Units units);                         // "445 kt"
Label distance(float nm, Units units);                         // "6.7 nm"
Label verticalRate(int32_t fpm, Units units);                  // "+1,216 fpm"
Label verticalRateNumber(int32_t fpm, Units units);            // "+1,216" (unit shown elsewhere)
Label heading(float degrees);                                  // "029°"
Label trimmed(const char* text);                               // "AAL1699 " -> "AAL1699"
Label clock(int hour, int minute, bool use24h);                // "21:30" / "9:30 PM"

// 0 = on the ground ... 5 = cruising high. Used to pick a color.
int altitudeBand(int32_t feet, bool onGround);

// +1 climbing, -1 descending, 0 level.
int verticalTrend(int32_t fpm);

// 7500 hijack, 7600 radio failure, 7700 general emergency.
bool isEmergencySquawk(const char* squawk);

}  // namespace fmt
