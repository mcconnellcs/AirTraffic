#pragma once

#include <stddef.h>
#include <stdint.h>

namespace screenshot {

// Caller supplies room for at least count * 3 bytes (the worst case).
// Keep the sprite's native byte order, matching tools/screenshot.py.
inline size_t encodeRuns(const uint16_t* pixels, size_t count, uint8_t* out) {
  size_t written = 0;
  for (size_t i = 0; i < count;) {
    const uint16_t color = pixels[i];
    uint8_t run = 1;
    while (i + run < count && pixels[i + run] == color && run < 255) run++;
    out[written++] = run;
    out[written++] = color & 0xFF;
    out[written++] = color >> 8;
    i += run;
  }
  return written;
}

}  // namespace screenshot
