// =============================================================================
//  ui_screens.h  —  Everything the screens need to know, and how to draw them
// =============================================================================
#pragma once

#include <vector>

#include "anim.h"
#include "app_settings.h"
#include "flight_feed.h"
#include "sky_model.h"
#include "ui_canvas.h"

namespace ui {

// ---- Layout (pixels) -----------------------------------------------------------
constexpr int kStatusBarH = 50;
constexpr int kRadarCX = 240;
constexpr int kRadarCY = 264;
constexpr int kRadarR = 194;
constexpr int kCardTop = 142;  // where the flight card stops when fully open

enum class Screen { Boot, Setup, Radar, List };

// One aircraft, ready to draw (positions are worked out once per frame).
struct Blip {
  const Track* track;
  float x, y;           // screen position
  float heading;
  float opacity;        // 0..1 for fading in/out
  float sweepGlow;      // 0..1, bright just after the radar sweep passes
  uint16_t color;
  bool selected;
  bool emergency;
  bool fresh;           // just appeared
  bool labelled;
  float trailX[Trail::kCapacity + 1];
  float trailY[Trail::kCapacity + 1];
  int trailCount;
};

// Everything the screens read. Filled in by AirTraffic.ino every frame.
struct UiState {
  uint32_t now;
  AppSettings settings;
  FeedStatus feed;
  const SkyModel* sky;
  geo::LatLon home;

  int wifiRssi;
  bool timeValid;
  int hour, minute;

  Screen screen;
  uint32_t bootStartMs;
  const char* bootMessage;

  float displayRangeNm;      // animates when the range changes
  float sweepDeg;
  std::vector<Blip> blips;   // nearest first

  bool hasSelection;
  char selectedHex[8];
  float cardOpen;            // 0 closed .. 1 open (animated)
  uint32_t cardOpenedMs;
  bool haveDetails;
  Details details;
  const LGFX_Sprite* photo;  // decoded aircraft photo, or nullptr
  uint32_t photoReadyMs;

  float listScroll;          // rows scrolled (animated)
  uint32_t listEnteredMs;

  bool settingsOpen;
  float settingsAnim;        // 0..1

  bool ambient;              // idle "desk display" mode
};

// ---- Per-frame preparation -------------------------------------------------------
void prepareBlips(UiState& s);
const Track* selectedTrack(const UiState& s);

// ---- Drawing (called once per strip) ---------------------------------------------
void drawBoot(Gfx& g, const UiState& s);
void drawSetup(Gfx& g, const UiState& s);
void drawRadar(Gfx& g, const UiState& s);
void drawCard(Gfx& g, const UiState& s);
void drawList(Gfx& g, const UiState& s);
void drawSettings(Gfx& g, const UiState& s);
void drawStatusBar(Gfx& g, const UiState& s);
void drawPageDots(Gfx& g, int active, int count);

// ---- Touch -------------------------------------------------------------------
// Which plane (index into s.blips) is at x,y on the radar, or -1.
int hitRadar(const UiState& s, int x, int y);
// Which list row is at x,y, or -1.
int hitList(const UiState& s, int x, int y);
bool hitRangeButton(int x, int y);

enum class SettingsAction { None, Close, BrightnessDown, BrightnessUp, Units, Clock, WifiSetup };
SettingsAction hitSettings(int x, int y);

// Things built once at start-up (QR code picture).
bool initScreens();

// Decodes a JPEG photo into a sprite sized for the flight card.
bool decodePhoto(LGFX_Sprite& sprite, const uint8_t* jpeg, size_t length);

constexpr int kListRowH = 62;
constexpr int kListTop = 108;
int listVisibleRows();

}  // namespace ui
