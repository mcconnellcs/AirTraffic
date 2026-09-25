// =============================================================================
//  screen_system.cpp  —  Start-up animation, Wi-Fi setup and the settings menu
// =============================================================================
#include <math.h>

#include "ui_screens.h"
#include "wifi_setup.h"

namespace ui {

namespace {

constexpr float kPi = 3.14159265f;
constexpr int kQrSize = 168;
LGFX_Sprite qrSprite;

// ---- Boot ---------------------------------------------------------------------

void drawRipples(Gfx& g, int cx, int cy, uint32_t t) {
  for (int i = 0; i < 3; i++) {
    const float p = anim::progress(0, (t + i * 900) % 2700, 2700);
    const float r = 24 + p * 190;
    g.drawCircle(cx, cy, r, anim::blend565(theme::kBackground, theme::kAccent, 0.35f * (1 - p)));
  }
}

void drawOrbitingPlane(Gfx& g, int cx, int cy, uint32_t t) {
  // The plane circles the center; its contrail is where it was a moment ago.
  const float orbit = 92.0f;
  const float speed = 0.0016f;  // radians per ms
  for (int i = 18; i >= 1; i--) {
    const float a0 = (t - i * 40) * speed, a1 = (t - (i - 1) * 40) * speed;
    const float fade = 1.0f - i / 18.0f;
    wedgeLine(g, cx + orbit * cosf(a0), cy + orbit * sinf(a0), cx + orbit * cosf(a1),
                    cy + orbit * sinf(a1), 0.5f + 1.5f * fade, 0.5f + 1.6f * fade,
                    anim::blend565(theme::kBackground, theme::kAccent, 0.8f * fade));
  }
  const float a = t * speed;
  const float headingDeg = a * 180.0f / kPi + 180.0f;  // tangent to the circle
  planeIcon(g, cx + orbit * cosf(a), cy + orbit * sinf(a), headingDeg, 30, theme::kText);
  g.fillSmoothCircle(cx, cy, 5, theme::kAccent);
}

void drawWordmark(Gfx& g, int y, uint32_t t) {
  static const char kWord[] = "AIRTRAFFIC";
  const int total = textWidth(g, kWord, theme::Font::Display);
  int x = SCREEN_W / 2 - total / 2;
  char letter[2] = {0, 0};
  for (int i = 0; kWord[i]; i++) {
    letter[0] = kWord[i];
    const float p = anim::easeOutBack(anim::progress(300 + i * 70, t, 520));
    const float fade = anim::clamp01(anim::progress(300 + i * 70, t, 300));
    const int w = textWidth(g, letter, theme::Font::Display);
    text(g, letter, x, y + static_cast<int>((1.0f - p) * 18), theme::Font::Display,
         anim::blend565(theme::kBackground, theme::kText, fade));
    x += w;
  }
  const float tag = anim::easeOutCubic(anim::progress(1200, t, 700));
  text(g, "P E R S O N A L   F L I G H T   R A D A R", SCREEN_W / 2, y + 58, theme::Font::Label,
       anim::blend565(theme::kBackground, theme::kAccent, tag), Align::Center);
}

void drawBootStatus(Gfx& g, const UiState& s, uint32_t t) {
  const int y = 404;
  if (!rowsVisible(g, y - 4, 50)) return;
  const float show = anim::progress(1500, t, 500);
  char msg[48];
  const int dots = (t / 400) % 4;
  snprintf(msg, sizeof(msg), "%s%.*s", s.bootMessage ? s.bootMessage : "", dots, "...");
  text(g, msg, SCREEN_W / 2 - textWidth(g, s.bootMessage ? s.bootMessage : "", theme::Font::Body) / 2, y,
       theme::Font::Body, anim::blend565(theme::kBackground, theme::kTextDim, show));
  // "Busy" bar: a bright band sliding back and forth.
  const int barX = 140, barW = 200, barY = y + 34;
  g.fillSmoothRoundRect(barX, barY, barW, 4, 2, anim::blend565(theme::kBackground, theme::kGrid, show));
  const float p = anim::easeInOutCubic(anim::pulse(t, 1800));
  const int bandW = 60;
  g.fillSmoothRoundRect(barX + static_cast<int>(p * (barW - bandW)), barY, bandW, 4, 2,
                        anim::blend565(theme::kBackground, theme::kAccent, show));
}

// ---- Settings sheet -------------------------------------------------------------

constexpr int kSheetX = 30, kSheetY = 120, kSheetW = 420, kSheetH = 280;

struct Button {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

constexpr Button kUnits{kSheetX + 150, kSheetY + 64, 246, 40};
constexpr Button kClock{kSheetX + 150, kSheetY + 124, 246, 40};
constexpr Button kWifi{kSheetX + 24, kSheetY + 190, 220, 50};
constexpr Button kDone{kSheetX + 262, kSheetY + 190, 134, 50};

void segmented(Gfx& g, const Button& b, const char* left, const char* right, bool rightActive) {
  roundedRect(g, b.x, b.y, b.w, b.h, 12, theme::kBackground);
  const int half = b.w / 2;
  roundedRect(g, b.x + (rightActive ? half : 0) + 3, b.y + 3, half - 6, b.h - 6, 10, theme::kAccentDeep);
  text(g, left, b.x + half / 2, b.y + 11, theme::Font::Label, rightActive ? theme::kTextMuted : theme::kAccent, Align::Center);
  text(g, right, b.x + half + half / 2, b.y + 11, theme::Font::Label, rightActive ? theme::kAccent : theme::kTextMuted, Align::Center);
}

void pillButton(Gfx& g, const Button& b, const char* label, bool primary) {
  roundedRect(g, b.x, b.y, b.w, b.h, b.h / 2, primary ? theme::kAccent : theme::kSurfaceHi);
  text(g, label, b.x + b.w / 2, b.y + b.h / 2 - 9, theme::Font::BodyBold,
       primary ? theme::kBackground : theme::kText, Align::Center);
}

}  // namespace

bool initScreens() {
  qrSprite.setPsram(true);
  qrSprite.setColorDepth(16);
  if (!qrSprite.createSprite(kQrSize, kQrSize)) return false;
  qrSprite.fillScreen(TFT_WHITE);
  char payload[64];
  snprintf(payload, sizeof(payload), "WIFI:T:nopass;S:%s;;", wifisetup::kHotspotName);
  qrSprite.qrcode(payload, 8, 8, kQrSize - 16, 3);
  return buildRadarLayer() && buildListLayer();
}

void drawBoot(Gfx& g, const UiState& s) {
  const uint32_t t = s.now - s.bootStartMs;
  g.fillScreen(theme::kBackground);
  const int cx = SCREEN_W / 2, cy = 170;
  drawRipples(g, cx, cy, t);
  drawOrbitingPlane(g, cx, cy, t);
  drawWordmark(g, 290, t);
  drawBootStatus(g, s, t);
}

void drawSetup(Gfx& g, const UiState& s) {
  g.fillScreen(theme::kBackground);

  // Wi-Fi waves pulsing out from a dot.
  const int wx = SCREEN_W / 2, wy = 78;
  for (int i = 0; i < 3; i++) {
    const float glow = anim::pulse(s.now + i * 250, 1500);
    g.drawArc(wx, wy, 14 + i * 12, 11 + i * 12, 225, 315,
              anim::blend565(theme::kGrid, theme::kAccent, glow));
  }
  g.fillSmoothCircle(wx, wy, 4, theme::kAccent);

  text(g, "Let's get connected", SCREEN_W / 2, 94, theme::Font::Title, theme::kText, Align::Center);
  text(g, "AirTraffic needs your Wi-Fi to see planes.", SCREEN_W / 2, 124, theme::Font::Label,
       theme::kTextDim, Align::Center);

  // QR code on a white card (phones read dark-on-light best).
  const int qx = 26, qy = 168;
  if (rowsVisible(g, qy - 6, kQrSize + 12)) {
    g.fillSmoothRoundRect(qx - 6, qy - 6, kQrSize + 12, kQrSize + 12, 16, TFT_WHITE);
    qrSprite.pushSprite(&g, qx, qy);
  }

  struct Step { const char* title; const char* detail; };
  const Step steps[3] = {
      {"Join the Wi-Fi", wifisetup::kHotspotName},
      {"A setup page opens", "or visit 192.168.4.1"},
      {"Pick your home Wi-Fi", "and type its password"},
  };
  for (int i = 0; i < 3; i++) {
    const int y = 172 + i * 62;
    const float in = anim::easeOutCubic(anim::progress(s.bootStartMs + 200 + i * 150, s.now, 500));
    const int x = 226 + static_cast<int>((1.0f - in) * 30);
    if (rowsVisible(g, y, 56)) {
      g.fillSmoothCircle(x + 14, y + 14, 14, anim::blend565(theme::kBackground, theme::kAccentDeep, in));
      char n[2] = {static_cast<char>('1' + i), 0};
      text(g, n, x + 14, y + 4, theme::Font::BodyBold, anim::blend565(theme::kBackground, theme::kAccent, in), Align::Center);
      text(g, steps[i].title, x + 38, y, theme::Font::BodyBold, anim::blend565(theme::kBackground, theme::kText, in));
      text(g, steps[i].detail, x + 38, y + 22, i == 0 ? theme::Font::Hud : theme::Font::Label,
           anim::blend565(theme::kBackground, i == 0 ? theme::kAccent : theme::kTextDim, in));
    }
  }
  text(g, "Your password is only saved on this device.", SCREEN_W / 2, 440, theme::Font::Label,
       theme::kTextMuted, Align::Center);
}

void drawSettings(Gfx& g, const UiState& s) {
  if (s.settingsAnim <= 0.001f) return;
  blendRect(g, 0, 0, SCREEN_W, SCREEN_H, theme::kBackground, 0.7f * s.settingsAnim);
  const int dy = static_cast<int>((1.0f - anim::easeOutCubic(s.settingsAnim)) * 60);
  if (!rowsVisible(g, kSheetY + dy - 12, kSheetH + 24)) return;

  // Everything below is drawn relative to the sheet, shifted by dy while animating.
  glassPanel(g, kSheetX, kSheetY + dy, kSheetW, kSheetH, 24, theme::kSurface);
  text(g, "Settings", kSheetX + 24, kSheetY + dy + 18, theme::Font::Title, theme::kText);

  auto shift = [dy](Button b) { return Button{b.x, b.y + dy, b.w, b.h}; };
  const int labelX = kSheetX + 24;

  text(g, "Units", labelX, kUnits.y + dy + 11, theme::Font::Body, theme::kTextDim);
  segmented(g, shift(kUnits), "FT \xC2\xB7 KT", "M \xC2\xB7 KM/H", s.settings.units == fmt::Units::Metric);
  text(g, "Clock", labelX, kClock.y + dy + 11, theme::Font::Body, theme::kTextDim);
  segmented(g, shift(kClock), "12 HOUR", "24 HOUR", s.settings.clock24h);

  pillButton(g, shift(kWifi), "Wi-Fi & location", false);
  pillButton(g, shift(kDone), "Done", true);
}

SettingsAction hitSettings(int x, int y) {
  if (kUnits.contains(x, y)) return SettingsAction::Units;
  if (kClock.contains(x, y)) return SettingsAction::Clock;
  if (kWifi.contains(x, y)) return SettingsAction::WifiSetup;
  if (kDone.contains(x, y)) return SettingsAction::Close;
  const Button sheet{kSheetX, kSheetY, kSheetW, kSheetH};
  return sheet.contains(x, y) ? SettingsAction::None : SettingsAction::Close;
}

}  // namespace ui
