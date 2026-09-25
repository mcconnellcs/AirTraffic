// =============================================================================
//  ui_fonts.h  —  Loads the smooth fonts once so every screen can use them
// =============================================================================
#pragma once

#include "board_config.h"
#include "theme.h"

namespace ui {

// Call once at start-up.
bool loadFonts();

// The loaded font (its letter table) and the raw font bytes in flash.
const lgfx::VLWfont* vlw(theme::Font which);
const uint8_t* fontData(theme::Font which);

}  // namespace ui
