#include "ui_fonts.h"

namespace ui {

namespace {

// A smooth font = the raw bytes in flash + LovyanGFX's reader for them.
struct SmoothFont {
  lgfx::PointerWrapper data;
  lgfx::VLWfont font;
};

constexpr size_t kFontCount = static_cast<size_t>(theme::Font::Count);
SmoothFont fonts[kFontCount];

const uint8_t* const kFontData[kFontCount] = {
    font_label, font_body, font_body_b, font_title, font_hud, font_hud_l, font_display, font_huge,
};

}  // namespace

bool loadFonts() {
  bool allOk = true;
  for (size_t i = 0; i < kFontCount; i++) {
    fonts[i].data.set(kFontData[i]);
    if (!fonts[i].font.loadFont(&fonts[i].data)) {
      Serial.printf("[fonts] font %u failed to load\n", static_cast<unsigned>(i));
      allOk = false;
    }
  }
  return allOk;
}

const lgfx::IFont* font(theme::Font which) {
  const size_t index = static_cast<size_t>(which);
  if (index >= kFontCount) return &fonts::Font2;  // built-in fallback
  return &fonts[index].font;
}

}  // namespace ui
