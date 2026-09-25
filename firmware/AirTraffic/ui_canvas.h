// =============================================================================
//  ui_canvas.h  —  How we draw a whole screen smoothly, 30+ times a second
// =============================================================================
//  The problem: a 480x480 picture is 460 KB. That only fits in the big-but-slow
//  PSRAM chip, and copying it around takes too long for smooth animation.
//
//  The trick: draw the screen in 10 horizontal STRIPS of 48 rows. Each strip is
//  only 46 KB, so it fits in the ESP32's small-but-FAST internal memory. We draw
//  everything into the strip, copy it to the screen, then move to the next one.
//
//  Your drawing code doesn't need to know about strips: it always uses normal
//  screen coordinates (0..479). Anything outside the current strip is skipped.
// =============================================================================
#pragma once

#include "board_config.h"
#include "theme.h"

namespace ui {

using Gfx = LGFX_Sprite;

enum class Align { Left, Center, Right };

class Canvas {
 public:
  static constexpr int kStripRows = 48;

  bool begin(LGFX* display);

  // Draws one full frame by calling `draw(gfx)` once for every strip.
  template <typename DrawFn>
  void render(DrawFn draw) {
    for (int y0 = 0; y0 < SCREEN_H; y0 += kStripRows) {
      beginLayer(y0, 0);
      draw(gfx_);
      endStrip(y0);
    }
  }

  // Draws two pages side by side, the left one shifted by `offsetX`
  // (0 = only left visible, -480 = only right visible), then `overlay` on top
  // of everything without any shift (pop-up cards and menus).
  template <typename DrawA, typename DrawB, typename DrawC>
  void renderPages(int offsetX, DrawA drawLeft, DrawB drawRight, DrawC overlay) {
    for (int y0 = 0; y0 < SCREEN_H; y0 += kStripRows) {
      if (offsetX > -SCREEN_W) {
        beginLayer(y0, offsetX);
        drawLeft(gfx_);
      }
      if (offsetX < 0) {
        beginLayer(y0, offsetX + SCREEN_W);
        drawRight(gfx_);
      }
      beginLayer(y0, 0);
      overlay(gfx_);
      endStrip(y0);
    }
  }

  // Screenshots: copy the next full frame into `destination` (480*480 pixels).
  void captureNextFrame(uint16_t* destination) { capture_ = destination; }
  bool captureBusy() const { return capture_ != nullptr; }

 private:
  LGFX* display_ = nullptr;
  Gfx gfx_;
  uint16_t* strip_ = nullptr;
  uint16_t* capture_ = nullptr;

  void beginLayer(int y0, int offsetX);
  void endStrip(int y0);
};

// ---- Drawing helpers used by every screen -----------------------------------
//
//  !! IMPORTANT for anyone adding drawing code !!
//  Never call g.drawWideLine(), g.drawSmoothLine(), g.drawWedgeLine() or
//  g.drawSpot() on the canvas. Those library functions replace the clip
//  rectangle with their own and draw outside the current strip, which
//  corrupts memory and crashes the board. Use wideLine() / wedgeLine() below.
//  Everything else (fillSmoothCircle, drawArc, drawString, pushSprite,
//  fillTriangle, drawCircle, ...) respects the strip and is safe.

// Is any part of rows [y, y+h) inside the strip being drawn right now?
bool rowsVisible(Gfx& g, int y, int h);

// Anti-aliased line whose thickness changes from `ra` at one end to `rb` at
// the other (contrails, the radar sweep). Safe replacement for drawWedgeLine.
void wedgeLine(Gfx& g, float ax, float ay, float bx, float by, float ra, float rb, uint16_t color);

// Anti-aliased line of constant thickness. Safe replacement for drawWideLine.
inline void wideLine(Gfx& g, float ax, float ay, float bx, float by, float r, uint16_t color) {
  wedgeLine(g, ax, ay, bx, by, r, r, color);
}

// Text with a smooth font. Returns the width in pixels.
int text(Gfx& g, const char* str, int x, int y, theme::Font font, uint16_t color,
         Align align = Align::Left);
int textWidth(Gfx& g, const char* str, theme::Font font);
int fontHeight(Gfx& g, theme::Font font);

// Mix a rectangle toward `color` by `amount` (0..1) — frosted glass, dimming.
void blendRect(Gfx& g, int x, int y, int w, int h, uint16_t color, float amount);

// A rounded panel with a soft shadow and a thin highlight along the top edge.
void glassPanel(Gfx& g, int x, int y, int w, int h, int radius, uint16_t fill);

// Soft glowing dot (used for planes, the home marker, selections).
void glowDot(Gfx& g, float x, float y, float radius, uint16_t color, uint16_t background,
             float strength);

// Aircraft silhouette pointing along `headingDeg`, about `size` pixels long.
void planeIcon(Gfx& g, float x, float y, float headingDeg, float size, uint16_t color);

// Small up/down/level arrow for climb and descent.
void trendArrow(Gfx& g, int x, int y, int trend, uint16_t color);

// Signal-strength bars for Wi-Fi.
void wifiBars(Gfx& g, int x, int y, int rssi, uint16_t on, uint16_t off);

// Builds the plane silhouette. Call once at start-up.
bool initIcons();

}  // namespace ui
