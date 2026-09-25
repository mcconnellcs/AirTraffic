// =============================================================================
//  ui_pixels.h  —  Tiny helpers for touching pixels directly (fast!)
// =============================================================================
//  The graphics library is convenient but every call has overhead. For the
//  things we draw hundreds of times per frame (text, glows, the sweep) we
//  write straight into the strip's memory instead. Used only inside ui_*.cpp.
//
//  Pixels are stored as RGB565 with the two bytes swapped (that's the order
//  the screen wants), so we un-swap, mix, and swap back.
// =============================================================================
#pragma once

#include <stdint.h>

#include "board_config.h"

namespace ui {

inline uint16_t swapBytes(uint16_t v) { return static_cast<uint16_t>((v << 8) | (v >> 8)); }

// A color split into the parts blendPixel() needs, worked out once per shape.
struct Ink {
  uint32_t rb;  // red and blue, still in their RGB565 positions
  uint32_t g5;  // green, still in its RGB565 position
  explicit Ink(uint16_t color565) : rb(color565 & 0xF81F), g5(color565 & 0x07E0) {}
};

// Mix the ink into one pixel. alpha 0 = leave it, 256 = replace it completely.
//
// Speed trick: red and blue sit in separate bit fields with a gap between
// them, so they can be multiplied together in one go (they never overflow
// into each other with a 5-bit alpha). Two multiplies instead of three.
inline void blendPixel(uint16_t* p, const Ink& ink, uint32_t alpha256) {
  const uint32_t a = alpha256 >> 3;  // 0..32
  if (a == 0) return;
  const uint32_t inv = 32 - a;
  const uint32_t v = swapBytes(*p);
  const uint32_t rb = ((v & 0xF81F) * inv + ink.rb * a) >> 5;
  const uint32_t g = ((v & 0x07E0) * inv + ink.g5 * a) >> 5;
  *p = swapBytes(static_cast<uint16_t>((rb & 0xF81F) | (g & 0x07E0)));
}

// 0..255 -> 0..256 (so 255 means "fully opaque", not 99.6%).
inline uint32_t alpha256(uint32_t alpha255) { return alpha255 + (alpha255 >> 7); }

// A rectangle clipped to what's drawable right now. Returns false if empty.
struct ClipBox {
  int x0, y0, x1, y1;  // inclusive .. exclusive
  bool empty() const { return x0 >= x1 || y0 >= y1; }
};

inline ClipBox clipBox(LGFX_Sprite& g, int x, int y, int w, int h) {
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  return {x > cx ? x : static_cast<int>(cx), y > cy ? y : static_cast<int>(cy),
          (x + w < cx + cw) ? x + w : static_cast<int>(cx + cw),
          (y + h < cy + ch) ? y + h : static_cast<int>(cy + ch)};
}

inline uint16_t* pixelAt(LGFX_Sprite& g, int x, int y) {
  return static_cast<uint16_t*>(g.getBuffer()) + y * SCREEN_W + x;
}

}  // namespace ui
