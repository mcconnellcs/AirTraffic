// =============================================================================
//  screen_radar.cpp  —  The main radar picture
// =============================================================================
//  Layers, drawn back to front (like painting):
//    1. the dark scope circle with a soft glow in the middle
//    2. the SWEEP: a bright line with a fading "phosphor" tail behind it
//    3. range rings, compass ticks and N/E/S/W letters
//    4. contrails, then aircraft, then their name tags
//    5. corner info: aircraft count, place, range button, altitude colors
// =============================================================================
#include <math.h>
#include <string.h>

#include <algorithm>

#include "format.h"
#include "ui_screens.h"

namespace ui {

namespace {

constexpr float kPi = 3.14159265f;
constexpr int kSweepSlices = 28;         // how many wedges make the fading tail
constexpr float kSweepTailDeg = 50.0f;   // how long the tail is
constexpr int kMaxLabels = 9;            // name tags on the nearest planes only
constexpr int kRangeButtonX = 14, kRangeButtonY = 424, kRangeButtonW = 96, kRangeButtonH = 44;

float rad(float deg) { return deg * kPi / 180.0f; }
float rimX(float deg, float r) { return kRadarCX + r * sinf(rad(deg)); }
float rimY(float deg, float r) { return kRadarCY - r * cosf(rad(deg)); }

// How brightly the sweep has lit a blip: 1 right after the line passes, then fading.
float sweepGlowFor(float sweepDeg, float bearingDeg) {
  float behind = fmodf(sweepDeg - bearingDeg + 360.0f, 360.0f);  // degrees since the sweep passed
  return expf(-behind / 70.0f);
}

void drawScope(Gfx& g) {
  // Soft glow: a few circles, each a little lighter toward the middle.
  g.fillSmoothCircle(kRadarCX, kRadarCY, kRadarR + 1, theme::kScope);
  for (int i = 1; i <= 4; i++) {
    g.fillSmoothCircle(kRadarCX, kRadarCY, kRadarR - i * 36,
                       anim::blend565(theme::kScope, theme::kAccentDeep, i * 0.06f));
  }
}

void drawSweep(Gfx& g, float sweepDeg) {
  const float step = kSweepTailDeg / kSweepSlices;
  for (int i = 0; i < kSweepSlices; i++) {
    const float a0 = sweepDeg - (i + 1) * step;
    const float a1 = sweepDeg - i * step + 0.4f;  // tiny overlap hides seams
    const float fade = 1.0f - static_cast<float>(i) / kSweepSlices;
    const uint16_t color = anim::blend565(theme::kScope, theme::kAccent, 0.30f * fade * fade);
    g.fillTriangle(kRadarCX, kRadarCY, rimX(a0, kRadarR), rimY(a0, kRadarR), rimX(a1, kRadarR),
                   rimY(a1, kRadarR), color);
  }
  wideLine(g, kRadarCX, kRadarCY, rimX(sweepDeg, kRadarR), rimY(sweepDeg, kRadarR), 1.6f,
                 theme::kAccent);
}

void drawRings(Gfx& g, float rangeNm) {
  for (int i = 1; i <= 3; i++) {
    const int r = kRadarR * i / 3;
    g.drawCircle(kRadarCX, kRadarCY, r, i == 3 ? theme::kGridBright : theme::kGrid);
  }
  // Cross hairs
  g.drawFastVLine(kRadarCX, kRadarCY - kRadarR, kRadarR * 2, theme::kGrid);
  g.drawFastHLine(kRadarCX - kRadarR, kRadarCY, kRadarR * 2, theme::kGrid);

  // Compass ticks: small every 10°, big every 30°.
  for (int deg = 0; deg < 360; deg += 10) {
    const bool major = deg % 30 == 0;
    const float inner = kRadarR - (major ? 12 : 6);
    g.drawLine(rimX(deg, inner), rimY(deg, inner), rimX(deg, kRadarR), rimY(deg, kRadarR),
               major ? theme::kGridBright : theme::kGrid);
  }
  static const char* const kLetters[] = {"N", "E", "S", "W"};
  for (int i = 0; i < 4; i++) {
    const float deg = i * 90.0f;
    text(g, kLetters[i], rimX(deg, kRadarR - 26), rimY(deg, kRadarR - 26) - 9, theme::Font::Hud,
         i == 0 ? theme::kAccent : theme::kTextDim, Align::Center);
  }

  // Range labels along the lower-right diagonal.
  for (int i = 1; i <= 3; i++) {
    const float r = kRadarR * i / 3.0f;
    char label[12];
    const float nm = rangeNm * i / 3.0f;
    snprintf(label, sizeof(label), i == 3 ? "%.0f NM" : "%.0f", nm);
    text(g, label, rimX(135, r) + 4, rimY(135, r) + 2, theme::Font::Label, theme::kTextMuted);
  }
}

void drawHome(Gfx& g, uint32_t now) {
  const float p = anim::progress(0, now % 2400, 2400);
  const float r = 6 + p * 26;
  g.drawCircle(kRadarCX, kRadarCY, r, anim::blend565(theme::kScope, theme::kHome, 0.5f * (1 - p)));
  g.fillSmoothCircle(kRadarCX, kRadarCY, 4, theme::kHome);
  g.fillSmoothCircle(kRadarCX, kRadarCY, 2, theme::kScope);
}

void drawTrail(Gfx& g, const Blip& b) {
  for (int i = 1; i < b.trailCount; i++) {
    const float t = static_cast<float>(i) / b.trailCount;  // 0 = oldest
    const uint16_t c = anim::blend565(theme::kScope, b.color, 0.55f * t * b.opacity);
    wedgeLine(g, b.trailX[i - 1], b.trailY[i - 1], b.trailX[i], b.trailY[i], 0.4f + 1.2f * t,
                    0.4f + 1.2f * (i + 1.0f) / b.trailCount, c);
  }
}

void drawSelection(Gfx& g, const Blip& b, uint32_t now) {
  // Four rotating corner brackets, like a targeting reticle.
  const float spin = (now % 6000) / 6000.0f * 360.0f;
  const float r = 20 + 2 * anim::pulse(now, 1200);
  for (int i = 0; i < 4; i++) {
    const float a = spin + i * 90.0f;
    g.drawArc(b.x, b.y, r, r - 2, a - 22, a + 22, theme::kAccent);
  }
}

void drawBlip(Gfx& g, const Blip& b, uint32_t now) {
  const float glow = 0.35f + 0.65f * b.sweepGlow;
  if (b.emergency) {
    glowDot(g, b.x, b.y, 9, theme::kEmergency, theme::kScope, 0.4f + 0.6f * anim::pulse(now, 700));
  } else if (b.selected) {
    glowDot(g, b.x, b.y, 8, theme::kAccent, theme::kScope, 0.5f);
  } else if (b.sweepGlow > 0.05f) {
    glowDot(g, b.x, b.y, 7, b.color, theme::kScope, 0.35f * b.sweepGlow * b.opacity);
  }
  if (b.fresh) {  // "ping": an expanding ring when a plane first appears
    const float p = anim::progress(b.track->firstSeenMs, now, 1500);
    if (p < 1.0f) {
      g.drawCircle(b.x, b.y, 8 + p * 22, anim::blend565(theme::kScope, b.color, 1.0f - p));
    }
  }
  const uint16_t color = anim::blend565(theme::kScope, b.color, b.opacity * (b.selected ? 1.0f : glow));
  planeIcon(g, b.x, b.y, b.heading, b.selected ? 26 : 20, b.emergency ? theme::kEmergency : color);
  if (b.selected) drawSelection(g, b, now);
}

void drawLabel(Gfx& g, const Blip& b) {
  const Flight& f = b.track->latest;
  const int x = static_cast<int>(b.x) + 13, y = static_cast<int>(b.y) - 16;
  const uint16_t main = anim::blend565(theme::kScope, b.selected ? theme::kText : theme::kTextDim, b.opacity);
  const uint16_t sub = anim::blend565(theme::kScope, b.color, b.opacity * 0.9f);
  text(g, f.callsign, x, y, theme::Font::Label, main);
  const fmt::Label alt = fmt::shortAltitude(f.altFt, f.onGround);
  text(g, alt.c_str(), x, y + 15, theme::Font::Label, b.emergency ? theme::kEmergency : sub);
}

void drawCorners(Gfx& g, const UiState& s) {
  // Top left: how many aircraft
  char count[8];
  snprintf(count, sizeof(count), "%d", s.sky->activeCount());
  const int w = text(g, count, 18, kStatusBarH + 8, theme::Font::HudLarge, theme::kText);
  text(g, "AIRCRAFT", 22 + w, kStatusBarH + 20, theme::Font::Label, theme::kTextMuted);

  // Top right: place name and a pulsing "live" dot
  text(g, s.feed.city, 462, kStatusBarH + 10, theme::Font::Label, theme::kTextDim, Align::Right);
  const bool live = s.feed.state == FeedState::Live;
  const uint16_t dot = live ? theme::kAccent : theme::kWarning;
  const char* state = live ? "LIVE" : s.feed.state == FeedState::Error ? "RETRYING" : "LOADING";
  const int sw = textWidth(g, state, theme::Font::Label);
  text(g, state, 462, kStatusBarH + 27, theme::Font::Label, dot, Align::Right);
  glowDot(g, 462 - sw - 9, kStatusBarH + 34, 3, dot, theme::kBackground, anim::pulse(s.now, 1600));

  // Bottom left: range button
  if (rowsVisible(g, kRangeButtonY, kRangeButtonH)) {
    g.fillSmoothRoundRect(kRangeButtonX, kRangeButtonY, kRangeButtonW, kRangeButtonH, 12, theme::kSurface);
    g.drawRoundRect(kRangeButtonX, kRangeButtonY, kRangeButtonW, kRangeButtonH, 12, theme::kGrid);
    text(g, "RANGE", kRangeButtonX + 14, kRangeButtonY + 6, theme::Font::Label, theme::kTextMuted);
    char range[12];
    snprintf(range, sizeof(range), "%u NM", s.settings.rangeNm);
    text(g, range, kRangeButtonX + 14, kRangeButtonY + 21, theme::Font::BodyBold, theme::kText);
  }

  // Bottom right: altitude color key
  const int keyX = 352, keyY = 450;
  if (rowsVisible(g, keyY - 16, 30)) {
    text(g, "ALTITUDE FT", 466, keyY - 18, theme::Font::Label, theme::kTextMuted, Align::Right);
    for (int i = 0; i < 6; i++) {
      g.fillSmoothRoundRect(keyX + i * 19, keyY, 16, 5, 2, theme::kAltitude[i]);
    }
  }
}

void drawEmptySky(Gfx& g, const UiState& s) {
  if (s.feed.state != FeedState::Live || s.sky->activeCount() > 0) return;
  const uint16_t c = anim::blend565(theme::kScope, theme::kTextDim, 0.6f + 0.4f * anim::pulse(s.now, 3000));
  text(g, "Quiet skies", kRadarCX, kRadarCY + 40, theme::Font::BodyBold, c, Align::Center);
  char line[40];
  snprintf(line, sizeof(line), "No aircraft within %u NM", s.settings.rangeNm);
  text(g, line, kRadarCX, kRadarCY + 62, theme::Font::Label, theme::kTextMuted, Align::Center);
}

}  // namespace

// ---- Per-frame preparation -------------------------------------------------------

const Track* selectedTrack(const UiState& s) {
  return s.hasSelection ? s.sky->find(s.selectedHex) : nullptr;
}

void prepareBlips(UiState& s) {
  s.blips.clear();
  int labels = 0;

  // Nearest first, so the closest planes get name tags.
  std::vector<const Track*> order;
  for (const Track& t : s.sky->tracks()) order.push_back(&t);
  std::sort(order.begin(), order.end(), [](const Track* a, const Track* b) {
    return a->latest.distNm < b->latest.distNm;
  });

  for (const Track* t : order) {
    const geo::LatLon pos = s.sky->positionAt(*t, s.now);
    const float dist = geo::distanceNm(s.home, pos);
    if (dist > s.displayRangeNm * 1.02f) continue;
    const float brg = geo::bearingDeg(s.home, pos);
    const geo::ScreenPoint p = geo::toRadar(brg, dist, kRadarCX, kRadarCY, kRadarR, s.displayRangeNm);

    Blip b{};
    b.track = t;
    b.x = p.x;
    b.y = p.y;
    b.heading = t->latest.hasTrack ? t->latest.trackDeg : 0;
    b.opacity = s.sky->opacityAt(*t, s.now);
    b.sweepGlow = sweepGlowFor(s.sweepDeg, brg);
    b.color = theme::kAltitude[fmt::altitudeBand(t->latest.altFt, t->latest.onGround)];
    b.selected = s.hasSelection && strcmp(t->latest.hex, s.selectedHex) == 0;
    b.emergency = fmt::isEmergencySquawk(t->latest.squawk);
    b.fresh = s.sky->isNew(*t, s.now);
    b.labelled = b.selected || b.emergency || labels < kMaxLabels;
    if (b.labelled && !b.selected) labels++;

    // Contrail: past reported positions, then where the plane is drawn now.
    for (int i = 0; i < t->trail.size(); i++) {
      const geo::LatLon tp = t->trail.at(i);
      const geo::ScreenPoint q = geo::toRadar(geo::bearingDeg(s.home, tp), geo::distanceNm(s.home, tp),
                                              kRadarCX, kRadarCY, kRadarR, s.displayRangeNm);
      b.trailX[b.trailCount] = q.x;
      b.trailY[b.trailCount] = q.y;
      b.trailCount++;
    }
    b.trailX[b.trailCount] = p.x;
    b.trailY[b.trailCount] = p.y;
    b.trailCount++;
    s.blips.push_back(b);
  }
}

// ---- Drawing ------------------------------------------------------------------

void drawRadar(Gfx& g, const UiState& s) {
  g.fillScreen(theme::kBackground);
  if (rowsVisible(g, kRadarCY - kRadarR, kRadarR * 2 + 1)) {
    drawScope(g);
    drawSweep(g, s.sweepDeg);
    drawRings(g, s.displayRangeNm);
    drawHome(g, s.now);
    for (const Blip& b : s.blips) drawTrail(g, b);
    for (auto it = s.blips.rbegin(); it != s.blips.rend(); ++it) drawBlip(g, *it, s.now);
    for (const Blip& b : s.blips) {
      if (b.labelled) drawLabel(g, b);
    }
    drawEmptySky(g, s);
  }
  drawCorners(g, s);
  drawStatusBar(g, s);
  drawPageDots(g, 0, 2);
}

int hitRadar(const UiState& s, int x, int y) {
  int best = -1;
  float bestD2 = 30.0f * 30.0f;  // fingers are big: accept taps within 30 px
  for (size_t i = 0; i < s.blips.size(); i++) {
    const float dx = s.blips[i].x - x, dy = s.blips[i].y - y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < bestD2) {
      bestD2 = d2;
      best = static_cast<int>(i);
    }
  }
  return best;
}

bool hitRangeButton(int x, int y) {
  return x >= kRangeButtonX && x < kRangeButtonX + kRangeButtonW && y >= kRangeButtonY - 8 &&
         y < kRangeButtonY + kRangeButtonH + 8;
}

}  // namespace ui
