// =============================================================================
//  backlight.h  —  The screen's light
// =============================================================================
//  The backlight LEDs behind the glass are switched by GPIO 38: high = on.
//
//  Dimming with PWM (switching the pin on and off thousands of times a second)
//  is the usual trick, but on this board neither LovyanGFX's backlight driver
//  nor the Arduino LEDC driver lit the panel at all, while a plain HIGH does.
//  So for now the light is simply on or off, at full brightness. If you get
//  PWM working on your board, this is the one place to change.
// =============================================================================
#pragma once

#include <Arduino.h>

#include "board_config.h"

namespace backlight {

inline void begin() {
  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, HIGH);
}

// 0 = off, anything else = on (see the note above about dimming).
inline void set(uint8_t brightness) { digitalWrite(PIN_BACKLIGHT, brightness > 0 ? HIGH : LOW); }

}  // namespace backlight
