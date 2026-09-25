// =============================================================================
//  screenshot.h  —  Send what's on the screen to your computer over USB
// =============================================================================
//  Type "shot" in the Serial Monitor (or run tools/screenshot.py) and the
//  board sends the current picture. Great for sharing your build!
// =============================================================================
#pragma once

#include "ui_canvas.h"

namespace screenshot {

// Call every loop. Watches the serial port for the "shot" command.
void poll(ui::Canvas& canvas);

}  // namespace screenshot
