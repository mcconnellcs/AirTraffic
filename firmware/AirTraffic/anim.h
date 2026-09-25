// =============================================================================
//  anim.h  —  The secret sauce for smooth animations
// =============================================================================
//  Real things don't move at a constant speed: a door swings fast, then slows
//  as it closes. "Easing" functions copy that feeling. Each one takes a time
//  from 0.0 (start) to 1.0 (end) and returns how far along the motion is.
//
//  A Tween remembers "go from A to B, starting at time T, taking D ms" so you
//  can ask "what value right now?" on every frame.
// =============================================================================
#pragma once

#include <math.h>
#include <stdint.h>

namespace anim {

using EaseFn = float (*)(float);

inline float clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

inline float linear(float t) { return t; }
inline float easeOutCubic(float t) { float u = 1.0f - t; return 1.0f - u * u * u; }
inline float easeInOutCubic(float t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}
inline float easeOutBack(float t) {  // overshoots slightly, like a spring
  const float c1 = 1.70158f, c3 = c1 + 1.0f;
  return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}
inline float easeOutExpo(float t) { return t >= 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t); }

// Blend two angles the short way round (350° -> 10° goes through 0°, not 180°).
inline float lerpAngle(float from, float to, float t) {
  float diff = fmodf(to - from + 540.0f, 360.0f) - 180.0f;
  float result = fmodf(from + diff * t + 360.0f, 360.0f);
  return result >= 359.9995f ? 0.0f : result;
}

// 0.0 -> 1.0 as time goes from `startMs` to `startMs + durationMs`.
inline float progress(uint32_t startMs, uint32_t nowMs, uint32_t durationMs) {
  if (durationMs == 0) return 1.0f;
  if (nowMs <= startMs) return 0.0f;
  return clamp01(static_cast<float>(nowMs - startMs) / durationMs);
}

// Smooth 0 -> 1 -> 0 wave, repeating every `periodMs` (for glowing / pulsing).
inline float pulse(uint32_t nowMs, uint32_t periodMs) {
  const float phase = static_cast<float>(nowMs % periodMs) / periodMs;
  return 0.5f - 0.5f * cosf(phase * 2.0f * 3.14159265f);
}

// Mix two RGB565 colors: t = 0 gives `a`, t = 1 gives `b`.
inline uint16_t blend565(uint16_t a, uint16_t b, float t) {
  const uint32_t w = static_cast<uint32_t>(clamp01(t) * 256.0f + 0.5f);
  const uint32_t r = (((a >> 11) & 0x1F) * (256 - w) + ((b >> 11) & 0x1F) * w) >> 8;
  const uint32_t g = (((a >> 5) & 0x3F) * (256 - w) + ((b >> 5) & 0x3F) * w) >> 8;
  const uint32_t bl = ((a & 0x1F) * (256 - w) + (b & 0x1F) * w) >> 8;
  return static_cast<uint16_t>((r << 11) | (g << 5) | bl);
}

struct Tween {
  float from;
  float to;
  uint32_t startMs;
  uint32_t durationMs;
  EaseFn ease;

  static Tween start(float from, float to, uint32_t nowMs, uint32_t durationMs,
                     EaseFn ease = easeOutCubic) {
    return {from, to, nowMs, durationMs, ease};
  }
  static Tween still(float value) { return {value, value, 0, 0, linear}; }

  float valueAt(uint32_t nowMs) const {
    return lerp(from, to, ease(progress(startMs, nowMs, durationMs)));
  }
  bool doneAt(uint32_t nowMs) const { return progress(startMs, nowMs, durationMs) >= 1.0f; }

  // A new tween that starts from wherever this one is right now.
  Tween retarget(uint32_t nowMs, float newTo, uint32_t newDurationMs = 0) const {
    return {valueAt(nowMs), newTo, nowMs, newDurationMs ? newDurationMs : durationMs, ease};
  }
};

}  // namespace anim
