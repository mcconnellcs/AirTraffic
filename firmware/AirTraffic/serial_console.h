// =============================================================================
//  serial_console.h  —  Commands you can type into the Serial Monitor
// =============================================================================
//  Open Tools > Serial Monitor (115200 baud), type a word, press Enter:
//
//    help          list the commands
//    demo          fill the radar with pretend planes (no Wi-Fi needed); again = off
//    shot          send a screenshot to your computer (see tools/screenshot.py)
//    tap X Y       pretend to tap the screen at that spot
//    swipe DIR     pretend to swipe: left, right, up or down
//    hold          pretend to press and hold (opens the settings)
// =============================================================================
#pragma once

#include "ui_canvas.h"

namespace console {

void poll(ui::Canvas& canvas);  // call every loop()

}  // namespace console
