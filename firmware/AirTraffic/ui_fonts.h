// =============================================================================
//  ui_fonts.h  —  Loads the smooth fonts once so every screen can use them
// =============================================================================
#pragma once

#include "board_config.h"
#include "theme.h"

namespace ui {

// Call once at start-up.
bool loadFonts();

// The font to pass to sprite.setFont(...).
const lgfx::IFont* font(theme::Font which);

}  // namespace ui
