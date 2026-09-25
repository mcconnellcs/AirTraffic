// =============================================================================
//  app.h  —  The "brain": decides which screen to show and reacts to touches
// =============================================================================
#pragma once

namespace app {

void begin();  // runs once at power-on
void tick();   // runs over and over, about 30 times a second

}  // namespace app
