#include "ui_canvas.h"

#include <esp_heap_caps.h>
#include <math.h>

#include <algorithm>

#include "anim.h"
#include "ui_fonts.h"

namespace ui {

namespace {

constexpr int kIconSize = 64;  // the plane is drawn big once, then shrunk smoothly
LGFX_Sprite planeSprite;

inline uint16_t swapBytes(uint16_t v) { return static_cast<uint16_t>((v << 8) | (v >> 8)); }

}  // namespace

// ---- Canvas -----------------------------------------------------------------

bool Canvas::begin(LGFX* display) {
  display_ = display;
  const size_t bytes = SCREEN_W * kStripRows * sizeof(uint16_t);
  strip_ = static_cast<uint16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (strip_ == nullptr) return false;
  gfx_.setColorDepth(16);
  return true;
}

void Canvas::beginLayer(int y0, int offsetX) {
  // Point the sprite at a "virtual" 480x480 picture whose rows y0..y0+47
  // land exactly on our small strip, shifted sideways by offsetX.
  // The clip rectangle makes sure nothing is ever written outside the strip.
  uint16_t* virtualBase = strip_ - y0 * SCREEN_W + offsetX;
  gfx_.setBuffer(virtualBase, SCREEN_W, SCREEN_H, 16);
  const int left = offsetX < 0 ? -offsetX : 0;
  const int right = offsetX > 0 ? SCREEN_W - offsetX : SCREEN_W;
  gfx_.setClipRect(left, y0, right - left, kStripRows);
}

void Canvas::endStrip(int y0) {
  display_->pushImage(0, y0, SCREEN_W, kStripRows, reinterpret_cast<lgfx::swap565_t*>(strip_));
  if (capture_ != nullptr) {
    memcpy(capture_ + y0 * SCREEN_W, strip_, SCREEN_W * kStripRows * sizeof(uint16_t));
    if (y0 + kStripRows >= SCREEN_H) capture_ = nullptr;  // frame finished
  }
}

// ---- Helpers ----------------------------------------------------------------

bool rowsVisible(Gfx& g, int y, int h) {
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  return y < cy + ch && y + h > cy;
}

int text(Gfx& g, const char* str, int x, int y, theme::Font font, uint16_t color, Align align) {
  g.setFont(ui::font(font));
  g.setTextColor(color);  // no background color = blend smoothly onto what's there
  g.setTextDatum(align == Align::Left    ? textdatum_t::top_left
                 : align == Align::Right ? textdatum_t::top_right
                                         : textdatum_t::top_center);
  if (!rowsVisible(g, y - 4, g.fontHeight() + 8)) return g.textWidth(str);
  return g.drawString(str, x, y);
}

int textWidth(Gfx& g, const char* str, theme::Font font) {
  g.setFont(ui::font(font));
  return g.textWidth(str);
}

int fontHeight(Gfx& g, theme::Font font) {
  g.setFont(ui::font(font));
  return g.fontHeight();
}

void wedgeLine(Gfx& g, float ax, float ay, float bx, float by, float ra, float rb, uint16_t color) {
  // Work out the box the line could touch, then shrink it to the current strip.
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  const int x0 = std::max<int>(cx, static_cast<int>(floorf(std::min(ax - ra, bx - rb))));
  const int x1 = std::min<int>(cx + cw - 1, static_cast<int>(ceilf(std::max(ax + ra, bx + rb))));
  const int y0 = std::max<int>(cy, static_cast<int>(floorf(std::min(ay - ra, by - rb))));
  const int y1 = std::min<int>(cy + ch - 1, static_cast<int>(ceilf(std::max(ay + ra, by + rb))));
  if (x0 > x1 || y0 > y1) return;

  const float dx = bx - ax, dy = by - ay;
  const float len2 = dx * dx + dy * dy;
  const uint32_t cr = (color >> 11) & 0x1F, cg = (color >> 5) & 0x3F, cb = color & 0x1F;
  uint16_t* pixels = static_cast<uint16_t*>(g.getBuffer());

  for (int yp = y0; yp <= y1; yp++) {
    uint16_t* row = pixels + yp * SCREEN_W;
    bool insideLine = false;
    for (int xp = x0; xp <= x1; xp++) {
      // Nearest point on the line to this pixel, and the line's thickness there.
      float t = len2 > 1e-6f ? ((xp - ax) * dx + (yp - ay) * dy) / len2 : 0.0f;
      t = anim::clamp01(t);
      const float px = ax + t * dx, py = ay + t * dy;
      const float r = ra + (rb - ra) * t;
      const float dist = sqrtf((xp - px) * (xp - px) + (yp - py) * (yp - py));
      float coverage = r + 0.5f - dist;  // how much of this pixel the line covers
      if (coverage <= 0.0f) {
        if (insideLine) break;  // we've crossed the line; nothing more on this row
        continue;
      }
      insideLine = true;
      if (coverage > 1.0f) coverage = 1.0f;
      const uint32_t a = static_cast<uint32_t>(coverage * 32.0f + 0.5f), inv = 32 - a;
      const uint16_t v = swapBytes(row[xp]);
      const uint32_t rr = (((v >> 11) & 0x1F) * inv + cr * a) >> 5;
      const uint32_t gg = (((v >> 5) & 0x3F) * inv + cg * a) >> 5;
      const uint32_t bb = ((v & 0x1F) * inv + cb * a) >> 5;
      row[xp] = swapBytes(static_cast<uint16_t>((rr << 11) | (gg << 5) | bb));
    }
  }
}

void blendRect(Gfx& g, int x, int y, int w, int h, uint16_t color, float amount) {
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  const int x0 = x > cx ? x : cx, x1 = (x + w < cx + cw) ? x + w : cx + cw;
  const int y0 = y > cy ? y : cy, y1 = (y + h < cy + ch) ? y + h : cy + ch;
  if (x0 >= x1 || y0 >= y1) return;

  const uint32_t a = static_cast<uint32_t>(anim::clamp01(amount) * 32.0f + 0.5f);  // 0..32
  const uint32_t inv = 32 - a;
  const uint32_t cr = (color >> 11) & 0x1F, cg = (color >> 5) & 0x3F, cb = color & 0x1F;
  uint16_t* pixels = static_cast<uint16_t*>(g.getBuffer());
  for (int row = y0; row < y1; row++) {
    uint16_t* p = pixels + row * SCREEN_W + x0;
    for (int col = x0; col < x1; col++, p++) {
      const uint16_t v = swapBytes(*p);
      const uint32_t r = (((v >> 11) & 0x1F) * inv + cr * a) >> 5;
      const uint32_t gr = (((v >> 5) & 0x3F) * inv + cg * a) >> 5;
      const uint32_t b = ((v & 0x1F) * inv + cb * a) >> 5;
      *p = swapBytes(static_cast<uint16_t>((r << 11) | (gr << 5) | b));
    }
  }
}

void glassPanel(Gfx& g, int x, int y, int w, int h, int radius, uint16_t fill) {
  if (!rowsVisible(g, y - 12, h + 24)) return;
  // Soft shadow: a few slightly bigger, darker outlines behind the panel.
  for (int i = 3; i >= 1; i--) {
    g.fillSmoothRoundRect(x - i * 2, y - i + 4, w + i * 4, h + i * 2, radius + i * 2,
                          anim::blend565(theme::kBackground, 0x0000, 0.25f * (4 - i)));
  }
  g.fillSmoothRoundRect(x, y, w, h, radius, fill);
  // Highlight: a thin brighter line along the top, like light catching glass.
  g.drawGradientHLine(x + radius, y, w - 2 * radius, anim::blend565(fill, theme::kText, 0.10f),
                      anim::blend565(fill, theme::kText, 0.22f));
}

void glowDot(Gfx& g, float x, float y, float radius, uint16_t color, uint16_t background,
             float strength) {
  if (!rowsVisible(g, static_cast<int>(y - radius * 3), static_cast<int>(radius * 6))) return;
  const float s = anim::clamp01(strength);
  for (int i = 3; i >= 1; i--) {
    const float r = radius * (1.0f + i * 0.55f);
    g.fillSmoothCircle(x, y, r, anim::blend565(background, color, s * 0.16f * (4 - i)));
  }
  g.fillSmoothCircle(x, y, radius, anim::blend565(background, color, 0.25f + 0.75f * s));
}

bool initIcons() {
  // A 1-bit (two color) sprite: color 0 = see-through, color 1 = the plane.
  planeSprite.setPsram(false);
  planeSprite.setColorDepth(1);
  if (!planeSprite.createSprite(kIconSize, kIconSize)) return false;
  planeSprite.fillScreen(0);
  const int c = kIconSize / 2;
  planeSprite.fillRoundRect(c - 4, 2, 8, 58, 4, 1);                  // body
  planeSprite.fillTriangle(c, 18, 1, 38, 63, 38, 1);                  // wings
  planeSprite.fillRect(1, 36, 62, 4, 1);
  planeSprite.fillTriangle(c, 46, c - 15, 60, c + 15, 60, 1);         // tail
  planeSprite.fillRect(c - 15, 58, 30, 3, 1);
  planeSprite.setPivot(c, c);
  return true;
}

void planeIcon(Gfx& g, float x, float y, float headingDeg, float size, uint16_t color) {
  if (!rowsVisible(g, static_cast<int>(y - size), static_cast<int>(size * 2))) return;
  planeSprite.setPaletteColor(1, color);
  const float zoom = size / kIconSize;
  planeSprite.pushRotateZoomWithAA(&g, x, y, headingDeg, zoom, zoom, 0);
}

void trendArrow(Gfx& g, int x, int y, int trend, uint16_t color) {
  if (trend > 0) {
    g.fillTriangle(x, y - 6, x - 5, y + 3, x + 5, y + 3, color);
  } else if (trend < 0) {
    g.fillTriangle(x, y + 6, x - 5, y - 3, x + 5, y - 3, color);
  } else {
    g.fillRoundRect(x - 5, y - 1, 10, 3, 1, color);
  }
}

void wifiBars(Gfx& g, int x, int y, int rssi, uint16_t on, uint16_t off) {
  const int level = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi >= -85 ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    const int h = 4 + i * 3;
    g.fillRoundRect(x + i * 5, y + 13 - h, 3, h, 1, i < level ? on : off);
  }
}

}  // namespace ui
