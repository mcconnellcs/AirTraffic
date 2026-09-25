// =============================================================================
//  screen_card.cpp  —  The flight card that slides up when you tap a plane
// =============================================================================
#include <math.h>
#include <string.h>

#include "format.h"
#include "ui_screens.h"

namespace ui {

namespace {

constexpr int kPad = 22;
constexpr int kPhotoW = 150, kPhotoH = 100;
constexpr uint32_t kCountUpMs = 900;  // numbers "roll up" to their value when the card opens

// Values count up from zero when the card opens — a small touch that feels alive.
float countUp(const UiState& s, float value) {
  return value * anim::easeOutExpo(anim::progress(s.cardOpenedMs, s.now, kCountUpMs));
}

// Hide the square corners of the photo so it looks rounded.
void roundCorners(Gfx& g, int x, int y, int w, int h, int r, uint16_t color) {
  for (int row = 0; row < r; row++) {
    const float dy = r - row - 0.5f;
    const int cut = r - static_cast<int>(sqrtf(r * r - dy * dy) + 0.5f);
    if (cut <= 0) continue;
    g.drawFastHLine(x, y + row, cut, color);
    g.drawFastHLine(x + w - cut, y + row, cut, color);
    g.drawFastHLine(x, y + h - 1 - row, cut, color);
    g.drawFastHLine(x + w - cut, y + h - 1 - row, cut, color);
  }
}

void drawPhotoBox(Gfx& g, const UiState& s, const Flight& f, int x, int y) {
  if (!rowsVisible(g, y, kPhotoH)) return;
  if (s.photo != nullptr) {
    const int px = x + (kPhotoW - s.photo->width()) / 2;
    const int py = y + (kPhotoH - s.photo->height()) / 2;
    g.fillRect(x, y, kPhotoW, kPhotoH, theme::kSurfaceHi);
    const_cast<LGFX_Sprite*>(s.photo)->pushSprite(&g, px, py);
    const float fade = 1.0f - anim::easeOutCubic(anim::progress(s.photoReadyMs, s.now, 500));
    if (fade > 0.01f) blendRect(g, x, y, kPhotoW, kPhotoH, theme::kSurfaceHi, fade);
    roundCorners(g, x, y, kPhotoW, kPhotoH, 12, theme::kSurface);
    return;
  }
  // No photo (yet): a big plane silhouette and the type code instead.
  g.fillSmoothRoundRect(x, y, kPhotoW, kPhotoH, 12, theme::kSurfaceHi);
  const uint16_t c = theme::kAltitude[fmt::altitudeBand(f.altFt, f.onGround)];
  planeIcon(g, x + kPhotoW / 2, y + 42, 45, 52, anim::blend565(theme::kSurfaceHi, c, 0.8f));
  text(g, f.type[0] ? f.type : "----", x + kPhotoW / 2, y + 74, theme::Font::Hud, theme::kTextDim,
       Align::Center);
}

void drawRoute(Gfx& g, const UiState& s, const Flight& f, int y) {
  if (!rowsVisible(g, y, 70)) return;
  const RouteInfo& route = s.details.route;
  if (!s.haveDetails || !s.details.routeDone) {
    const uint16_t c = anim::blend565(theme::kSurface, theme::kTextMuted, 0.5f + 0.5f * anim::pulse(s.now, 1000));
    text(g, "Looking up route...", kPad, y + 20, theme::Font::Body, c);
    return;
  }
  if (!route.valid) {
    text(g, "Route not published", kPad, y + 12, theme::Font::Body, theme::kTextDim);
    text(g, "Private, military or training flights often don't share one.", kPad, y + 36,
         theme::Font::Label, theme::kTextMuted);
    return;
  }

  const int left = kPad, right = SCREEN_W - kPad;
  text(g, route.origin.iata, left, y, theme::Font::HudLarge, theme::kText);
  text(g, route.dest.iata, right, y, theme::Font::HudLarge, theme::kText, Align::Right);
  text(g, route.origin.city, left, y + 36, theme::Font::Label, theme::kTextDim);
  text(g, route.dest.city, right, y + 36, theme::Font::Label, theme::kTextDim, Align::Right);

  // Progress line with a plane that glides to how far along the trip it is.
  const int lineL = left + 76, lineR = right - 76, lineY = y + 17;
  const float progress = static_cast<float>(geo::routeProgress(route.origin.pos, f.pos, route.dest.pos));
  const float shown = countUp(s, progress);
  const int px = lineL + static_cast<int>((lineR - lineL) * shown);
  for (int x = lineL; x < lineR; x += 8) g.drawFastHLine(x, lineY, 4, theme::kGrid);  // dashed
  g.drawGradientHLine(lineL, lineY, px - lineL, theme::kAccentDeep, theme::kAccent);
  g.fillSmoothCircle(lineL, lineY, 4, theme::kAccent);
  g.drawCircle(lineR, lineY, 4, theme::kGridBright);
  planeIcon(g, px, lineY, 90, 22, theme::kText);

  char pct[8];
  snprintf(pct, sizeof(pct), "%d%%", static_cast<int>(progress * 100 + 0.5f));
  text(g, pct, (lineL + lineR) / 2, y + 30, theme::Font::Label, theme::kTextMuted, Align::Center);
}

struct Stat {
  const char* label;
  fmt::Label value;
  uint16_t color;
  int trend;  // -1/0/+1 arrow, or 2 = none
};

void drawStat(Gfx& g, const Stat& st, int x, int y) {
  if (!rowsVisible(g, y, 56)) return;
  text(g, st.label, x, y, theme::Font::Label, theme::kTextMuted);
  const int w = text(g, st.value.c_str(), x, y + 17, theme::Font::HudLarge, st.color);
  if (st.trend != 2) trendArrow(g, x + w + 10, y + 33, st.trend, st.color);
}

void drawStats(Gfx& g, const UiState& s, const Flight& f, geo::LatLon pos, int y) {
  const fmt::Units u = s.settings.units;
  const float dist = static_cast<float>(geo::distanceNm(s.home, pos));
  const float brg = static_cast<float>(geo::bearingDeg(s.home, pos));
  const int trend = fmt::verticalTrend(f.vrateFpm);
  const uint16_t altColor = theme::kAltitude[fmt::altitudeBand(f.altFt, f.onGround)];

  fmt::Label alt = fmt::altitude(static_cast<int32_t>(countUp(s, f.altFt)), f.onGround, u);
  fmt::Label spd = fmt::speed(countUp(s, f.gsKt), u);
  fmt::Label dst = fmt::distance(countUp(s, dist), u);
  fmt::Label dstDir = fmt::Label::printf("%s %s", dst.c_str(), geo::cardinal(brg));
  const bool emergency = fmt::isEmergencySquawk(f.squawk);

  const Stat stats[6] = {
      {"ALTITUDE", alt, altColor, trend},
      {"SPEED", spd, theme::kText, 2},
      {"DISTANCE", dstDir, theme::kText, 2},
      {"HEADING", f.hasTrack ? fmt::heading(f.trackDeg) : fmt::Label::printf("---"), theme::kText, 2},
      {"CLIMB", fmt::verticalRate(f.vrateFpm, u), trend > 0 ? theme::kAccent : trend < 0 ? theme::kWarning : theme::kText, 2},
      {"SQUAWK", fmt::Label::printf("%s", f.squawk[0] ? f.squawk : "----"), emergency ? theme::kEmergency : theme::kText, 2},
  };
  const int colW = (SCREEN_W - 2 * kPad) / 3;
  for (int i = 0; i < 6; i++) {
    drawStat(g, stats[i], kPad + (i % 3) * colW, y + (i / 3) * 62);
  }
}

}  // namespace

void drawCard(Gfx& g, const UiState& s) {
  if (s.cardOpen <= 0.001f) return;
  const Track* t = selectedTrack(s);
  if (t == nullptr) return;
  const Flight& f = t->latest;
  const geo::LatLon pos = s.sky->positionAt(*t, s.now);

  // Dim everything behind the card, then slide the card up from the bottom.
  blendRect(g, 0, 0, SCREEN_W, SCREEN_H, theme::kBackground, 0.55f * s.cardOpen);
  const int top = kCardTop + static_cast<int>((SCREEN_H - kCardTop) * (1.0f - s.cardOpen));
  glassPanel(g, 0, top, SCREEN_W, SCREEN_H - top + 30, 26, theme::kSurface);
  if (rowsVisible(g, top + 8, 6)) g.fillSmoothRoundRect(SCREEN_W / 2 - 22, top + 9, 44, 5, 2, theme::kGridBright);

  // Header: callsign, airline, type.
  const int y = top + 24;
  text(g, f.callsign, kPad, y, theme::Font::Display, theme::kText);
  const char* airline = s.haveDetails && s.details.route.valid && s.details.route.airline[0]
                            ? s.details.route.airline
                        : f.owner[0] ? f.owner
                        : s.haveDetails && s.details.aircraft.owner[0] ? s.details.aircraft.owner
                                                                        : "Unknown operator";
  text(g, airline, kPad, y + 50, theme::Font::BodyBold, theme::kAccent);
  const char* model = f.desc[0] ? f.desc
                      : s.haveDetails && s.details.aircraft.model[0] ? s.details.aircraft.model
                                                                     : f.type;
  char line[64];
  snprintf(line, sizeof(line), "%s%s%s", model, f.reg[0] ? "  \xC2\xB7  " : "", f.reg);
  text(g, line, kPad, y + 72, theme::Font::Label, theme::kTextDim);
  drawPhotoBox(g, s, f, SCREEN_W - kPad - kPhotoW, y);

  if (fmt::isEmergencySquawk(f.squawk) && rowsVisible(g, y - 22, 20)) {
    g.fillSmoothRoundRect(kPad, y - 20, 150, 18, 9, theme::kEmergency);
    text(g, "EMERGENCY SQUAWK", kPad + 75, y - 19, theme::Font::Label, theme::kText, Align::Center);
  }

  drawRoute(g, s, f, y + 116);
  drawStats(g, s, f, pos, y + 188);
}

bool decodePhoto(LGFX_Sprite& sprite, const uint8_t* jpeg, size_t length) {
  // Photos are about 200x133 px; shrink to fit our 150x100 box.
  sprite.deleteSprite();
  sprite.setPsram(true);
  sprite.setColorDepth(16);
  if (!sprite.createSprite(kPhotoW, kPhotoH)) return false;
  sprite.fillScreen(theme::kSurfaceHi);
  return sprite.drawJpg(jpeg, length, 0, 0, kPhotoW, kPhotoH, 0, 0, 0.0f, 0.0f,
                        textdatum_t::middle_center);
}

}  // namespace ui
