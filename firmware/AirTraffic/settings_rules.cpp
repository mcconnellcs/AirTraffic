// Pure rules for settings (no hardware needed, so they're unit-tested in test/).
#include <stdlib.h>

#include "app_settings.h"

namespace settings {

bool parseCoordinate(const char* text, double minValue, double maxValue, double* out) {
  if (text == nullptr || *text == '\0') return false;
  char* end = nullptr;
  const double value = strtod(text, &end);
  while (end && *end == ' ') end++;
  if (end == text || (end && *end != '\0')) return false;
  if (value < minValue || value > maxValue) return false;
  *out = value;
  return true;
}

uint16_t nextRange(uint16_t current) {
  const size_t count = sizeof(kRangeChoicesNm) / sizeof(kRangeChoicesNm[0]);
  for (size_t i = 0; i < count; i++) {
    if (kRangeChoicesNm[i] == current) return kRangeChoicesNm[(i + 1) % count];
  }
  return kDefaultRangeNm;
}

}  // namespace settings
