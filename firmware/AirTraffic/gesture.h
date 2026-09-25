// =============================================================================
//  gesture.h  —  Turn raw finger positions into taps, swipes and long-presses
// =============================================================================
//  The touch chip only tells us "a finger is at (x, y)" many times a second.
//  This class watches those samples over time and decides what the finger did.
// =============================================================================
#pragma once

#include <stdint.h>

enum class GestureType { None, Tap, LongPress, SwipeLeft, SwipeRight, SwipeUp, SwipeDown };

struct Gesture {
  GestureType type;
  int x;  // where the finger first touched
  int y;
};

class GestureDetector {
 public:
  static constexpr int kTapSlopPx = 12;         // a tap may wobble this much
  static constexpr int kSwipeMinPx = 70;        // a swipe must travel this far
  static constexpr uint32_t kLongPressMs = 800;

  // Call this every frame. Returns a gesture when one finishes.
  Gesture update(bool touching, int x, int y, uint32_t nowMs);

  bool isTouching() const { return touching_; }
  int dragX() const { return touching_ ? lastX_ - startX_ : 0; }
  int dragY() const { return touching_ ? lastY_ - startY_ : 0; }

 private:
  bool touching_ = false;
  bool longPressFired_ = false;
  bool movedTooFar_ = false;
  int startX_ = 0, startY_ = 0, lastX_ = 0, lastY_ = 0;
  uint32_t startMs_ = 0;

  Gesture finish();
};
