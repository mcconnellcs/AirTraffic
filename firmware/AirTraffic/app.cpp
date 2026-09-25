#include "app.h"

#include <WiFi.h>
#include <time.h>

#include "backlight.h"
#include "board_config.h"
#include "flight_feed.h"
#include "gesture.h"
#include "serial_console.h"
#include "sky_model.h"
#include "ui_fonts.h"
#include "ui_screens.h"
#include "wifi_setup.h"

namespace app {

namespace {

// ---- Timing (milliseconds) ----------------------------------------------------
constexpr uint32_t kMinBootMs = 2800;       // let the intro animation play
constexpr uint32_t kSweepPeriodMs = 4000;   // one radar sweep turn
constexpr uint32_t kCardOpenMs = 480;
constexpr uint32_t kCardCloseMs = 320;
constexpr uint32_t kPageSlideMs = 420;
constexpr uint32_t kRangeZoomMs = 700;
constexpr uint32_t kAmbientCycleMs = 12000;
constexpr int kAmbientNearest = 5;
constexpr uint32_t kFrameMs = 33;           // ~30 frames per second is plenty

LGFX display;
ui::Canvas canvas;
SkyModel sky;
GestureDetector gestures;
ui::UiState ui{};
AppSettings settings{};
LGFX_Sprite photo;
LGFX_Sprite logo;

uint32_t snapshotSequence = 0;
anim::Tween cardTween = anim::Tween::still(0);
anim::Tween pageTween = anim::Tween::still(0);    // 0 = radar, 1 = list
anim::Tween rangeTween = anim::Tween::still(kDefaultRangeNm);
anim::Tween scrollTween = anim::Tween::still(0);
anim::Tween settingsTween = anim::Tween::still(0);
bool photoDecoded = false;
bool logoDecoded = false;
Gesture injected{GestureType::None, 0, 0};
uint32_t lastTouchMs = 0, lastAmbientMs = 0, fpsStartMs = 0, frames = 0;
int ambientIndex = 0;

// ---- Small helpers ------------------------------------------------------------

bool cardIsOpen() { return cardTween.to > 0.5f; }

void openCard(const Track& t, uint32_t now) {
  strncpy(ui.selectedHex, t.latest.hex, sizeof(ui.selectedHex) - 1);
  ui.hasSelection = true;
  ui.cardOpenedMs = now;
  photoDecoded = false;
  photo.deleteSprite();
  ui.photo = nullptr;
  logoDecoded = false;
  logo.deleteSprite();
  ui.logo = nullptr;
  feed::wantDetails(t.latest.hex, t.latest.callsign, true);
  if (!cardIsOpen()) cardTween = anim::Tween::start(cardTween.valueAt(now), 1, now, kCardOpenMs);
}

void closeCard(uint32_t now) {
  cardTween = anim::Tween::start(cardTween.valueAt(now), 0, now, kCardCloseMs, anim::easeInOutCubic);
}

void showPage(float page, uint32_t now) {
  if (page == pageTween.to) return;
  pageTween = anim::Tween::start(pageTween.valueAt(now), page, now, kPageSlideMs, anim::easeInOutCubic);
  ui.screen = page > 0.5f ? ui::Screen::List : ui::Screen::Radar;
  if (ui.screen == ui::Screen::List) ui.listEnteredMs = now + kPageSlideMs / 3;
}

void applySettings(const AppSettings& next, uint32_t now) {
  if (next.rangeNm != settings.rangeNm) {
    rangeTween = anim::Tween::start(rangeTween.valueAt(now), next.rangeNm, now, kRangeZoomMs,
                                    anim::easeInOutCubic);
  }
  settings = next;
  settings::save(settings);
  feed::updateSettings(settings);
  backlight::set(settings.brightness);
}

// Selects the next / previous plane (by distance) while the card is open.
void stepSelection(int direction, uint32_t now) {
  if (ui.blips.empty()) return;
  int current = 0;
  for (size_t i = 0; i < ui.blips.size(); i++) {
    if (ui.blips[i].selected) current = static_cast<int>(i);
  }
  const int n = static_cast<int>(ui.blips.size());
  openCard(*ui.blips[(current + direction + n) % n].track, now);
}

// ---- Touch --------------------------------------------------------------------

void onSettingsTap(const Gesture& g, uint32_t now) {
  AppSettings next = settings;
  switch (ui::hitSettings(g.x, g.y)) {
    case ui::SettingsAction::Units:
      next.units = settings.units == fmt::Units::Metric ? fmt::Units::Aviation : fmt::Units::Metric;
      break;
    case ui::SettingsAction::Clock:
      next.clock24h = !settings.clock24h;
      break;
    case ui::SettingsAction::WifiSetup:
      ui.settingsOpen = false;
      settingsTween = anim::Tween::still(0);
      wifisetup::startPortal();
      ui.screen = ui::Screen::Setup;
      ui.bootStartMs = now;
      return;
    case ui::SettingsAction::Close:
      ui.settingsOpen = false;
      settingsTween = anim::Tween::start(settingsTween.valueAt(now), 0, now, 260);
      return;
    case ui::SettingsAction::None:
      return;
  }
  applySettings(next, now);
}

void onGesture(const Gesture& g, uint32_t now) {
  if (ui.settingsOpen) {
    if (g.type == GestureType::Tap) onSettingsTap(g, now);
    return;
  }
  if (g.type == GestureType::LongPress) {
    ui.settingsOpen = true;
    settingsTween = anim::Tween::start(settingsTween.valueAt(now), 1, now, 380);
    return;
  }

  if (cardIsOpen()) {
    switch (g.type) {
      case GestureType::Tap:
        if (g.y < ui::kCardTop) closeCard(now);
        break;
      case GestureType::SwipeDown: closeCard(now); break;
      case GestureType::SwipeLeft: stepSelection(+1, now); break;
      case GestureType::SwipeRight: stepSelection(-1, now); break;
      default: break;
    }
    return;
  }

  if (ui.screen == ui::Screen::Radar) {
    if (g.type == GestureType::SwipeLeft) showPage(1, now);
    if (g.type != GestureType::Tap) return;
    if (ui::hitRangeButton(g.x, g.y)) {
      AppSettings next = settings;
      next.rangeNm = settings::nextRange(settings.rangeNm);
      applySettings(next, now);
      return;
    }
    const int hit = ui::hitRadar(ui, g.x, g.y);
    if (hit >= 0) openCard(*ui.blips[hit].track, now);
    return;
  }

  if (ui.screen == ui::Screen::List) {
    const float maxScroll = std::max(0, static_cast<int>(ui.blips.size()) - ui::listVisibleRows());
    float target = scrollTween.to;
    switch (g.type) {
      case GestureType::SwipeRight: showPage(0, now); return;
      case GestureType::SwipeUp: target = std::min(maxScroll, target + 3); break;
      case GestureType::SwipeDown: target = std::max(0.0f, target - 3); break;
      case GestureType::Tap: {
        const int hit = ui::hitList(ui, g.x, g.y);
        if (hit >= 0) openCard(*ui.blips[hit].track, now);
        return;
      }
      default: return;
    }
    scrollTween = anim::Tween::start(scrollTween.valueAt(now), target, now, 380);
  }
}

void readTouch(uint32_t now) {
  uint16_t rawX = 0, rawY = 0;
  const bool touching = display.getTouch(&rawX, &rawY);
  if (touching) lastTouchMs = now;
  int x = rawX, y = rawY;
  ui::rotateTouch(&x, &y);
  Gesture g = gestures.update(touching, x, y, now);
  if (injected.type != GestureType::None) {  // a pretend touch from the serial console
    g = injected;
    injected.type = GestureType::None;
    lastTouchMs = now;
  }
  if (g.type == GestureType::None) return;

  if (ui.ambient) {  // any touch wakes up from the idle "desk display" mode
    ui.ambient = false;
    closeCard(now);
    return;
  }
  onGesture(g, now);
}

// ---- Idle "desk display" mode ---------------------------------------------------

void updateAmbient(uint32_t now) {
  const bool idle = now - lastTouchMs > kAmbientAfterMs;
  if (!idle || ui.screen != ui::Screen::Radar || ui.settingsOpen || ui.blips.empty()) return;
  if (ui.ambient && now - lastAmbientMs < kAmbientCycleMs) return;
  ui.ambient = true;
  lastAmbientMs = now;
  const int count = std::min(kAmbientNearest, static_cast<int>(ui.blips.size()));
  ambientIndex = (ambientIndex + 1) % count;
  openCard(*ui.blips[ambientIndex].track, now);
}

// ---- Screens and data -------------------------------------------------------------

void updateScreenFlow(wifisetup::State wifi, uint32_t now) {
  const bool bootDone = now - ui.bootStartMs >= kMinBootMs;
  const bool online = wifi == wifisetup::State::Connected || feed::isDemo();
  switch (ui.screen) {
    case ui::Screen::Boot:
      ui.bootMessage = !online                 ? "Connecting to Wi-Fi"
                       : !ui.feed.haveLocation ? "Finding your location"
                                               : "Scanning the sky";
      if (!online && wifi == wifisetup::State::Portal && now - ui.bootStartMs > 1500) {
        ui.screen = ui::Screen::Setup;
        ui.bootStartMs = now;
      } else if (bootDone && (ui.feed.state == FeedState::Live || ui.feed.state == FeedState::Error)) {
        ui.screen = ui::Screen::Radar;
        lastTouchMs = now;
      }
      break;
    case ui::Screen::Setup:
      if (online) {
        ui.screen = ui::Screen::Boot;
        ui.bootStartMs = now;
      }
      break;
    default:
      break;
  }
}

void updateData(uint32_t now) {
  Snapshot snap;
  if (feed::latest(snapshotSequence, &snap)) {
    snapshotSequence = snap.sequence;
    sky.applySnapshot(snap.flights, now);
  }
  sky.prune(now);
  ui.feed = feed::status();
  ui.home = ui.feed.home;

  if (ui.hasSelection && sky.find(ui.selectedHex) == nullptr) {  // plane left the area
    ui.hasSelection = false;
    closeCard(now);
  }
  if (ui.hasSelection) {
    ui.haveDetails = feed::details(ui.selectedHex, &ui.details);
    if (!photoDecoded) {
      // Decode once when the photo arrives; a failed decode is not retried.
      photoDecoded = feed::withPhoto(ui.selectedHex, [](const uint8_t* data, size_t length) {
        if (ui::decodePhoto(photo, data, length)) {
          ui.photo = &photo;
        } else {
          Serial.printf("[app] photo for %s: %u bytes, could not decode\n", ui.selectedHex, static_cast<unsigned>(length));
        }
      });
      if (ui.photo != nullptr) ui.photoReadyMs = now;
    }
    if (!logoDecoded && ui.haveDetails && ui.details.routeDone) {
      const RouteInfo& route = ui.details.route;
      if (!route.valid || route.airlineIcao[0] == '\0') {
        logoDecoded = true;  // no airline, no logo
      } else {
        const feed::LogoState state = feed::withLogo(route.airlineIcao, [](const uint8_t* data, size_t length) {
          if (ui::decodeLogo(logo, data, length)) ui.logo = &logo;
        });
        if (state != feed::LogoState::Pending) {
          logoDecoded = true;
          if (ui.logo != nullptr) ui.logoReadyMs = now;
        }
      }
    }
  }
}

void updateUiState(uint32_t now) {
  ui.now = now;
  ui.settings = settings;
  ui.sky = &sky;
  ui.wifiRssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -100;
  ui.sweepDeg = static_cast<float>(now % kSweepPeriodMs) / kSweepPeriodMs * 360.0f;
  ui.displayRangeNm = rangeTween.valueAt(now);
  ui.cardOpen = cardTween.valueAt(now);
  // Once the card is fully open it hides everything below its top edge.
  ui.coveredFromY = ui.cardOpen >= 0.999f ? ui::kCardTop + 40 : SCREEN_H;
  ui.listScroll = scrollTween.valueAt(now);
  ui.settingsAnim = settingsTween.valueAt(now);

  const time_t t = time(nullptr);
  ui.timeValid = t > 1700000000;  // the clock has been set from the internet
  if (ui.timeValid) {
    struct tm local;
    localtime_r(&t, &local);
    ui.hour = local.tm_hour;
    ui.minute = local.tm_min;
  }
}

void render(uint32_t now) {
  if (ui.screen == ui::Screen::Boot) {
    canvas.render([](ui::Gfx& g) { ui::drawBoot(g, ui); });
    return;
  }
  if (ui.screen == ui::Screen::Setup) {
    canvas.render([](ui::Gfx& g) { ui::drawSetup(g, ui); });
    return;
  }
  const int offsetX = -static_cast<int>(pageTween.valueAt(now) * SCREEN_W + 0.5f);
  canvas.renderPages(
      offsetX, [](ui::Gfx& g) { ui::drawRadar(g, ui); }, [](ui::Gfx& g) { ui::drawList(g, ui); },
      [](ui::Gfx& g) {
        ui::drawCard(g, ui);
        ui::drawSettings(g, ui);
      });
}

void logFps(uint32_t now) {
  frames++;
  if (now - fpsStartMs < 5000) return;
  Serial.printf("[app] %.1f fps, %d planes, heap %u KB, psram %u KB, stack left %u B\n",
                frames * 1000.0f / (now - fpsStartMs), sky.activeCount(),
                ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024,
                uxTaskGetStackHighWaterMark(nullptr));
  frames = 0;
  fpsStartMs = now;
}

}  // namespace

void injectGesture(const Gesture& gesture) { injected = gesture; }

void begin() {
  Serial.begin(115200);
  Serial.println("\n=== AirTraffic ===");

  settings = settings::load();
  display.init();
  display.fillScreen(theme::kBackground);
  backlight::begin();
  backlight::set(settings.brightness);

  bool ok = ui::loadFonts();
  ok &= ui::initIcons();
  ok &= ui::initScreens();
  ok &= canvas.begin(&display);
  if (!ok) Serial.println("[app] start-up problem: check PSRAM is set to 'OPI PSRAM'");

  rangeTween = anim::Tween::still(settings.rangeNm);
  ui.screen = ui::Screen::Boot;
  ui.bootStartMs = millis();
  ui.sky = &sky;

  wifisetup::begin(settings);
  feed::begin(settings);
}

void tick() {
  const uint32_t now = millis();
  console::poll(canvas);

  const wifisetup::State wifi = wifisetup::process();
  AppSettings fromPortal;
  if (wifisetup::takeNewSettings(&fromPortal)) applySettings(fromPortal, now);

  updateData(now);
  updateScreenFlow(wifi, now);
  updateUiState(now);
  // Blips hold pointers into the sky model, so they are rebuilt every tick
  // right after updateData() and before anything (touch, ambient) uses them.
  ui::prepareBlips(ui);
  if (ui.screen == ui::Screen::Radar || ui.screen == ui::Screen::List) {
    readTouch(now);
    updateAmbient(now);
    for (ui::Blip& b : ui.blips) {  // a touch may have changed the selection
      b.selected = ui.hasSelection && strcmp(b.track->latest.hex, ui.selectedHex) == 0;
    }
    ui.cardOpen = cardTween.valueAt(now);
    ui.settingsAnim = settingsTween.valueAt(now);
  }
  render(now);
  logFps(now);
  // Drawing faster than this only steals time from the downloads on core 0.
  const uint32_t took = millis() - now;
  if (took < kFrameMs) delay(kFrameMs - took);
}

}  // namespace app
