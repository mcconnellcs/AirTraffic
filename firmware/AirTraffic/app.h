// =============================================================================
//  app.h  —  The "brain": decides which screen to show and reacts to touches
// =============================================================================
#pragma once

#include "gesture.h"

namespace app {

void begin();  // runs once at power-on
void tick();   // runs over and over, about 30 times a second

// Pretend the screen was touched (used by the "tap"/"swipe" serial commands).
void injectGesture(const Gesture& gesture);

}  // namespace app
