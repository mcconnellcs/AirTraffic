#include "gesture.h"

#include <stdlib.h>

Gesture GestureDetector::update(bool touching, int x, int y, uint32_t nowMs) {
  if (touching && !touching_) {  // finger just landed
    touching_ = true;
    longPressFired_ = false;
    movedTooFar_ = false;
    startX_ = lastX_ = x;
    startY_ = lastY_ = y;
    startMs_ = nowMs;
    return {GestureType::None, x, y};
  }

  if (touching) {  // finger is still down
    lastX_ = x;
    lastY_ = y;
    if (abs(x - startX_) > kTapSlopPx || abs(y - startY_) > kTapSlopPx) movedTooFar_ = true;
    if (!movedTooFar_ && !longPressFired_ && nowMs - startMs_ >= kLongPressMs) {
      longPressFired_ = true;
      return {GestureType::LongPress, startX_, startY_};
    }
    return {GestureType::None, startX_, startY_};
  }

  if (touching_) return finish();  // finger just lifted
  return {GestureType::None, 0, 0};
}

Gesture GestureDetector::finish() {
  touching_ = false;
  if (longPressFired_) return {GestureType::None, startX_, startY_};

  const int dx = lastX_ - startX_;
  const int dy = lastY_ - startY_;
  if (!movedTooFar_) return {GestureType::Tap, startX_, startY_};

  if (abs(dx) >= kSwipeMinPx && abs(dx) > abs(dy)) {
    return {dx < 0 ? GestureType::SwipeLeft : GestureType::SwipeRight, startX_, startY_};
  }
  if (abs(dy) >= kSwipeMinPx && abs(dy) > abs(dx)) {
    return {dy < 0 ? GestureType::SwipeUp : GestureType::SwipeDown, startX_, startY_};
  }
  return {GestureType::None, startX_, startY_};
}
