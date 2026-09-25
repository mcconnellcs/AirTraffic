// =============================================================================
//  ui_chrome.cpp  —  The bits that appear on every screen: top bar, page dots
// =============================================================================
//  The bar is split in two: the part that never changes (logo, title, line)
//  is drawn once into each screen's background layer; the part that does
//  (clock, Wi-Fi bars) is drawn every frame.
// =============================================================================
#include "format.h"
#include "ui_screens.h"

namespace ui {

void drawStatusBarStatic(Gfx& g) {
  g.fillRect(0, 0, SCREEN_W, kStatusBarH, theme::kBackground);
  g.drawGradientHLine(0, kStatusBarH - 1, SCREEN_W / 2, theme::kBackground, theme::kGrid);
  g.drawGradientHLine(SCREEN_W / 2, kStatusBarH - 1, SCREEN_W / 2, theme::kGrid, theme::kBackground);
  planeIcon(g, 28, 25, 45, 20, theme::kAccent);
  text(g, "AIRTRAFFIC", 46, 15, theme::Font::Hud, theme::kText);
}

void drawStatusBarDynamic(Gfx& g, const UiState& s) {
  if (!rowsVisible(g, 0, kStatusBarH)) return;
  if (s.timeValid) {
    const fmt::Label clock = fmt::clock(s.hour, s.minute, s.settings.clock24h);
    text(g, clock.c_str(), 432, 15, theme::Font::Hud, theme::kText, Align::Right);
  }
  wifiBars(g, 443, 17, s.wifiRssi, theme::kText, theme::kGrid);
}

void drawStatusBar(Gfx& g, const UiState& s) {
  if (!rowsVisible(g, 0, kStatusBarH)) return;
  drawStatusBarStatic(g);
  drawStatusBarDynamic(g, s);
}

void drawPageDots(Gfx& g, int active, int count) {
  const int y = 470;
  if (!rowsVisible(g, y - 4, 8)) return;
  const int gap = 14;
  const int x0 = SCREEN_W / 2 - (count - 1) * gap / 2;
  for (int i = 0; i < count; i++) {
    if (i == active) {
      g.fillSmoothRoundRect(x0 + i * gap - 7, y - 3, 14, 6, 3, theme::kAccent);
    } else {
      g.fillSmoothCircle(x0 + i * gap, y, 3, theme::kGridBright);
    }
  }
}

}  // namespace ui
