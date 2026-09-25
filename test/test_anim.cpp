#include "doctest.h"
#include "anim.h"

#include <initializer_list>

TEST_CASE("easing curves start at 0 and end at 1") {
  for (auto ease : {anim::linear, anim::easeOutCubic, anim::easeInOutCubic, anim::easeOutBack,
                    anim::easeOutExpo}) {
    CHECK(ease(0.0f) == doctest::Approx(0.0f));
    CHECK(ease(1.0f) == doctest::Approx(1.0f));
  }
}

TEST_CASE("easeOutCubic is fast at first, slow at the end") {
  CHECK(anim::easeOutCubic(0.5f) > 0.5f);
  CHECK(anim::easeInOutCubic(0.5f) == doctest::Approx(0.5f));
}

TEST_CASE("easeOutBack overshoots a little before settling") {
  bool overshoots = false;
  for (int i = 1; i < 100; i++) overshoots |= anim::easeOutBack(i / 100.0f) > 1.0f;
  CHECK(overshoots);
}

TEST_CASE("clamp01 and lerp") {
  CHECK(anim::clamp01(-2.0f) == 0.0f);
  CHECK(anim::clamp01(3.0f) == 1.0f);
  CHECK(anim::lerp(10.0f, 20.0f, 0.25f) == doctest::Approx(12.5f));
}

TEST_CASE("lerpAngle takes the short way round") {
  CHECK(anim::lerpAngle(350.0f, 10.0f, 0.5f) == doctest::Approx(0.0f).epsilon(0.001));
  CHECK(anim::lerpAngle(10.0f, 350.0f, 0.5f) == doctest::Approx(0.0f).epsilon(0.001));
  CHECK(anim::lerpAngle(90.0f, 180.0f, 0.5f) == doctest::Approx(135.0f));
}

TEST_CASE("progress measures how far through an animation we are") {
  CHECK(anim::progress(1000, 1000, 500) == doctest::Approx(0.0f));
  CHECK(anim::progress(1000, 1250, 500) == doctest::Approx(0.5f));
  CHECK(anim::progress(1000, 9999, 500) == doctest::Approx(1.0f));
  CHECK(anim::progress(1000, 500, 500) == doctest::Approx(0.0f));   // before start
  CHECK(anim::progress(1000, 1000, 0) == doctest::Approx(1.0f));    // zero duration
}

TEST_CASE("Tween glides between two values") {
  anim::Tween t = anim::Tween::start(0.0f, 100.0f, 1000, 400, anim::linear);
  CHECK(t.valueAt(1000) == doctest::Approx(0.0f));
  CHECK(t.valueAt(1200) == doctest::Approx(50.0f));
  CHECK(t.valueAt(1400) == doctest::Approx(100.0f));
  CHECK(t.valueAt(5000) == doctest::Approx(100.0f));
  CHECK_FALSE(t.doneAt(1399));
  CHECK(t.doneAt(1400));
}

TEST_CASE("Tween::retarget starts smoothly from where it currently is") {
  anim::Tween t = anim::Tween::start(0.0f, 100.0f, 0, 400, anim::linear);
  anim::Tween t2 = t.retarget(200, 0.0f);
  CHECK(t2.valueAt(200) == doctest::Approx(50.0f));
  CHECK(t2.valueAt(600) == doctest::Approx(0.0f));
  CHECK(t.valueAt(400) == doctest::Approx(100.0f));  // original is untouched
}

TEST_CASE("Tween::still holds a value without animating") {
  anim::Tween t = anim::Tween::still(42.0f);
  CHECK(t.valueAt(0) == doctest::Approx(42.0f));
  CHECK(t.valueAt(123456) == doctest::Approx(42.0f));
  CHECK(t.doneAt(0));
}

TEST_CASE("pulse oscillates between 0 and 1") {
  float lo = 1, hi = 0;
  for (uint32_t ms = 0; ms < 2000; ms += 10) {
    float p = anim::pulse(ms, 1000);
    lo = p < lo ? p : lo;
    hi = p > hi ? p : hi;
  }
  CHECK(lo == doctest::Approx(0.0f).epsilon(0.01));
  CHECK(hi == doctest::Approx(1.0f).epsilon(0.01));
}

TEST_CASE("blend565 mixes two RGB565 colors") {
  CHECK(anim::blend565(0x0000, 0xFFFF, 0.0f) == 0x0000);
  CHECK(anim::blend565(0x0000, 0xFFFF, 1.0f) == 0xFFFF);
  uint16_t mid = anim::blend565(0x0000, 0xF800, 0.5f);  // half red
  CHECK(((mid >> 11) & 0x1F) == 15);
  CHECK((mid & 0x07FF) == 0);
}
