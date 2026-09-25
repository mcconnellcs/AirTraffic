#include "doctest.h"
#include "gesture.h"

// Helper: feed a finger path to the detector, one sample every 20 ms.
struct Finger {
  GestureDetector detector;
  uint32_t now = 1000;
  Gesture last{};

  Gesture down(int x, int y) { return step(true, x, y); }
  Gesture move(int x, int y) { return step(true, x, y); }
  Gesture up() { return step(false, 0, 0); }
  Gesture hold(int x, int y, uint32_t ms) {
    Gesture g{};
    for (uint32_t t = 0; t < ms; t += 20) {
      g = step(true, x, y);
      if (g.type != GestureType::None) return g;
    }
    return g;
  }
  Gesture step(bool touching, int x, int y) {
    now += 20;
    return detector.update(touching, x, y, now);
  }
};

TEST_CASE("nothing happens without a touch") {
  Finger f;
  CHECK(f.step(false, 0, 0).type == GestureType::None);
}

TEST_CASE("a quick touch in one place is a tap at that spot") {
  Finger f;
  f.down(100, 200);
  f.move(102, 201);
  Gesture g = f.up();
  CHECK(g.type == GestureType::Tap);
  CHECK(g.x == 100);
  CHECK(g.y == 200);
}

TEST_CASE("dragging sideways is a swipe") {
  Finger f;
  f.down(400, 240);
  f.move(300, 245);
  f.move(200, 250);
  CHECK(f.up().type == GestureType::SwipeLeft);

  Finger g;
  g.down(50, 240);
  g.move(200, 240);
  CHECK(g.up().type == GestureType::SwipeRight);
}

TEST_CASE("dragging up or down is a vertical swipe") {
  Finger f;
  f.down(240, 100);
  f.move(240, 300);
  CHECK(f.up().type == GestureType::SwipeDown);

  Finger g;
  g.down(240, 400);
  g.move(250, 150);
  CHECK(g.up().type == GestureType::SwipeUp);
}

TEST_CASE("holding still fires a long press once, and no tap afterwards") {
  Finger f;
  f.down(240, 240);
  Gesture g = f.hold(240, 240, 1500);
  CHECK(g.type == GestureType::LongPress);
  CHECK(f.hold(240, 240, 500).type == GestureType::None);
  CHECK(f.up().type == GestureType::None);
}

TEST_CASE("a slow wobble that is neither tap nor swipe does nothing") {
  Finger f;
  f.down(240, 240);
  f.move(260, 260);
  CHECK(f.up().type == GestureType::None);
}

TEST_CASE("dragX reports how far the finger has moved while touching") {
  Finger f;
  f.down(300, 240);
  f.move(250, 240);
  CHECK(f.detector.isTouching());
  CHECK(f.detector.dragX() == -50);
  f.up();
  CHECK_FALSE(f.detector.isTouching());
  CHECK(f.detector.dragX() == 0);
}
