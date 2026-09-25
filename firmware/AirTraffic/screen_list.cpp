// =============================================================================
//  screen_list.cpp  —  All nearby flights as a list, nearest first
// =============================================================================
#include "format.h"
#include "ui_screens.h"

namespace ui {

namespace {

constexpr uint32_t kRowStaggerMs = 45;   // each row arrives a little after the one above
constexpr uint32_t kRowSlideMs = 420;

Layer listLayer;  // background + top bar + title, drawn once
char countText[32];  // made once per frame

void drawRow(Gfx& g, const UiState& s, const Blip& b, int index, int y) {
  if (!rowsVisible(g, y, kListRowH)) return;
  const Flight& f = b.track->latest;

  // Rows slide in from the right, one after another, when the list opens.
  const float appear = anim::easeOutCubic(
      anim::progress(s.listEnteredMs + index * kRowStaggerMs, s.now, kRowSlideMs));
  const int x = 16 + static_cast<int>((1.0f - appear) * 60);
  const float fade = appear * b.opacity;
  auto ink = [&](uint16_t c) { return anim::blend565(theme::kBackground, c, fade); };

  g.fillSmoothRoundRect(x, y + 4, SCREEN_W - 32, kListRowH - 8, 14,
                        ink(b.selected ? theme::kSurfaceHi : theme::kSurface));
  g.fillSmoothRoundRect(x + 8, y + 16, 4, kListRowH - 32, 2, ink(b.emergency ? theme::kEmergency : b.color));

  text(g, f.callsign, x + 24, y + 11, theme::Font::Hud, ink(theme::kText));

  // Second line: route if we know it, otherwise the aircraft type.
  Details d{};
  char sub[48];
  if (feed::details(f.hex, &d) && d.route.valid) {
    snprintf(sub, sizeof(sub), "%s \xE2\x86\x92 %s  \xC2\xB7  %s", d.route.origin.iata, d.route.dest.iata, f.type);
  } else {
    snprintf(sub, sizeof(sub), "%s", f.desc[0] ? f.desc : f.type[0] ? f.type : "Unknown type");
  }
  text(g, sub, x + 24, y + 34, theme::Font::Label, ink(theme::kTextDim));

  // Right side: altitude with climb arrow, then distance with direction.
  const fmt::Label alt = fmt::shortAltitude(f.altFt, f.onGround);
  const int altRight = x + 330;
  text(g, alt.c_str(), altRight, y + 13, theme::Font::Hud, ink(b.color), Align::Right);
  trendArrow(g, altRight + 12, y + 25, fmt::verticalTrend(f.vrateFpm), ink(theme::kTextDim));
  text(g, "ALT", altRight, y + 36, theme::Font::Label, ink(theme::kTextMuted), Align::Right);

  const float dist = static_cast<float>(geo::distanceNm(s.home, f.pos));
  const fmt::Label dst = fmt::distance(dist, s.settings.units);
  text(g, dst.c_str(), x + 420, y + 13, theme::Font::Body, ink(theme::kText), Align::Right);
  planeIcon(g, x + 432, y + 42, f.bearingDeg, 16, ink(theme::kTextMuted));  // which way it is from home
  text(g, geo::cardinal(f.bearingDeg), x + 420, y + 36, theme::Font::Label, ink(theme::kTextMuted), Align::Right);
}

}  // namespace

bool buildListLayer() {
  LGFX_Sprite picture;
  if (!createLayerSprite(picture)) return false;
  picture.fillScreen(theme::kBackground);
  drawStatusBarStatic(picture);
  text(picture, "NEARBY FLIGHTS", 20, kStatusBarH + 14, theme::Font::Hud, theme::kText);
  const bool ok = listLayer.encode(picture);
  picture.deleteSprite();
  return ok;
}

int listVisibleRows() { return (SCREEN_H - kListTop - 20) / kListRowH; }

void drawList(Gfx& g, const UiState& s) {
  if (!rowsVisible(g, 0, s.coveredFromY)) return;  // the card hides this strip completely
  listLayer.paint(g);
  if (rowsVisible(g, kStatusBarH, 40)) {
    snprintf(countText, sizeof(countText), "%d in %u NM", s.sky->activeCount(), s.settings.rangeNm);
  }
  text(g, countText, 460, kStatusBarH + 17, theme::Font::Label, theme::kTextDim, Align::Right);

  const int first = static_cast<int>(s.listScroll);
  int shown = 0;
  for (int i = first; i < static_cast<int>(s.blips.size()) && shown <= listVisibleRows(); i++, shown++) {
    const int y = kListTop + static_cast<int>((i - s.listScroll) * kListRowH);
    drawRow(g, s, s.blips[i], i - first, y);
  }

  if (s.blips.empty()) {
    text(g, s.feed.state == FeedState::Live ? "No aircraft in range right now" : "Waiting for flight data...",
         SCREEN_W / 2, 240, theme::Font::Body, theme::kTextDim, Align::Center);
  }

  // Fade the bottom edge so rows disappear softly under the page dots.
  for (int i = 0; i < 6; i++) blendRect(g, 0, SCREEN_H - 36 + i * 6, SCREEN_W, 6, theme::kBackground, 0.2f + i * 0.16f);
  drawStatusBarDynamic(g, s);
  drawPageDots(g, 1, 2);
}

int hitList(const UiState& s, int x, int y) {
  (void)x;
  if (y < kListTop) return -1;
  const int index = static_cast<int>(s.listScroll + static_cast<float>(y - kListTop) / kListRowH);
  return index < static_cast<int>(s.blips.size()) ? index : -1;
}

}  // namespace ui
