// =============================================================================
//  ui_text.cpp  —  Draws smooth text straight into the strip (very fast)
// =============================================================================
//  A VLW font file is a table of letters. For each one it stores its size,
//  where it sits relative to the text baseline, and a little grey-scale
//  picture: 0 = no ink, 255 = full ink, anything between = a soft edge.
//  Drawing text is just stamping those pictures next to each other.
// =============================================================================
#include "ui_canvas.h"
#include "ui_fonts.h"
#include "ui_pixels.h"

namespace ui {

namespace {

constexpr int kHeaderBytes = 24;
constexpr int kGlyphHeaderBytes = 28;

uint32_t be32(const uint8_t* p) {  // fonts store numbers "big end first"
  return (static_cast<uint32_t>(p[0]) << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

struct Glyph {
  int width, height, advance, dx, dy;
  const uint8_t* alpha;  // width*height bytes, or nullptr for a blank glyph
};

// Reads one UTF-8 character (so "°" and "→" work) and moves the pointer on.
uint16_t nextCodePoint(const char*& s) {
  const uint8_t c = static_cast<uint8_t>(*s++);
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0 && *s) return ((c & 0x1F) << 6) | (static_cast<uint8_t>(*s++) & 0x3F);
  if ((c & 0xF0) == 0xE0 && s[0] && s[1]) {
    const uint16_t code = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(s[0]) & 0x3F) << 6) |
                          (static_cast<uint8_t>(s[1]) & 0x3F);
    s += 2;
    return code;
  }
  return '?';
}

// Which entry in the font's table is this character? -1 if the font lacks it.
// The letters are sorted by code, so a binary search finds one quickly.
int glyphIndex(const lgfx::VLWfont& f, uint16_t code) {
  int lo = 0, hi = static_cast<int>(f.gCount) - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) / 2;
    if (f.gUnicode[mid] < code) {
      lo = mid + 1;
    } else if (f.gUnicode[mid] > code) {
      hi = mid - 1;
    } else {
      return mid;
    }
  }
  return -1;
}

Glyph readGlyph(const lgfx::VLWfont& f, const uint8_t* data, int index) {
  Glyph g;
  const uint8_t* header = data + kHeaderBytes + index * kGlyphHeaderBytes;
  g.height = static_cast<int>(be32(header + 4));
  g.width = static_cast<int>(be32(header + 8));
  g.advance = static_cast<int>(be32(header + 12));
  g.dy = static_cast<int32_t>(be32(header + 16));
  g.dx = static_cast<int32_t>(be32(header + 20));
  g.alpha = g.width > 0 && g.height > 0 ? data + f.gBitmap[index] : nullptr;
  return g;
}

void stampGlyph(Gfx& g, const Glyph& glyph, int x, int y, const Ink& ink) {
  const ClipBox box = clipBox(g, x, y, glyph.width, glyph.height);
  if (box.empty() || glyph.alpha == nullptr) return;
  for (int row = box.y0; row < box.y1; row++) {
    const uint8_t* src = glyph.alpha + (row - y) * glyph.width + (box.x0 - x);
    uint16_t* dst = pixelAt(g, box.x0, row);
    for (int col = box.x0; col < box.x1; col++, src++, dst++) {
      if (*src) blendPixel(dst, ink, alpha256(*src));
    }
  }
}

}  // namespace

int textWidth(Gfx&, const char* str, theme::Font font) {
  const lgfx::VLWfont& f = *vlw(font);
  int width = 0;
  while (*str) {
    const int index = glyphIndex(f, nextCodePoint(str));
    width += index >= 0 ? f.gxAdvance[index] : f.spaceWidth;
  }
  return width;
}

int fontHeight(Gfx&, theme::Font font) { return vlw(font)->yAdvance; }

int text(Gfx& g, const char* str, int x, int y, theme::Font font, uint16_t color, Align align) {
  const lgfx::VLWfont& f = *vlw(font);
  const int width = textWidth(g, str, font);
  if (align == Align::Center) x -= width / 2;
  if (align == Align::Right) x -= width;
  if (!rowsVisible(g, y, f.yAdvance)) return width;

  const uint8_t* data = fontData(font);
  const Ink ink(color);
  while (*str) {
    const int index = glyphIndex(f, nextCodePoint(str));
    if (index < 0) {
      x += f.spaceWidth;
      continue;
    }
    const Glyph glyph = readGlyph(f, data, index);
    stampGlyph(g, glyph, x + glyph.dx, y + (f.maxAscent - glyph.dy), ink);
    x += glyph.advance;
  }
  return width;
}

}  // namespace ui
