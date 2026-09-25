// =============================================================================
//  ui_canvas.h  —  How we draw a whole screen smoothly, 30+ times a second
// =============================================================================
//  The problem: a 480x480 picture is 460 KB. That only fits in the big-but-slow
//  PSRAM chip, and copying it around takes too long for smooth animation.
//
//  Trick 1 - STRIPS: draw the screen in 10 horizontal strips of 48 rows. Each
//  strip is only 46 KB, so it fits in the ESP32's small-but-FAST internal
//  memory. Your drawing code doesn't need to know: it always uses normal
//  screen coordinates (0..479) and anything outside the strip is skipped.
//
//  Trick 2 - TWO CORES: the ESP32-S3 has two processors. While core 1 draws
//  the next strip, core 0 copies the finished one to the screen. We have two
//  strip buffers so they never step on each other.
//
//  Trick 3 - LAYERS: things that never change (the radar rings, the top bar)
//  are drawn once into a full-size picture in PSRAM and copied in each frame.
//  Copying is much faster than drawing.
// =============================================================================
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

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
  struct Job {
    int buffer;
    int y0;
  };

  LGFX* display_ = nullptr;
  Gfx gfx_;
  uint16_t* strips_[2] = {nullptr, nullptr};
  int current_ = 0;
  QueueHandle_t jobs_ = nullptr;
  SemaphoreHandle_t free_[2] = {nullptr, nullptr};
  uint16_t* volatile capture_ = nullptr;
  bool capturing_ = false;  // core 0 only: copying the frame that started at strip 0

  static void pushTask(void* self);
  void beginLayer(int y0, int offsetX);
  void endStrip(int y0);
};

// ---- Layers ------------------------------------------------------------------
//
//  Reading a whole 460 KB picture out of PSRAM every frame is slow, so a
//  layer is stored as "runs": each row is a list of (color, how many pixels).
//  Flat areas become a handful of numbers, and painting is just fast fills.
//  Colors are kept in a palette of up to 256 so each run is only 2 bytes;
//  the 256 most common colors are exact, the rare ones snap to the nearest.

class Layer {
 public:
  // Draw the picture into `sprite` (a normal full-screen sprite), then call
  // this. The sprite can be deleted afterwards.
  bool encode(const LGFX_Sprite& sprite);

  // Copies the layer into the strip being drawn.
  void paint(Gfx& g) const;

 private:
  uint32_t* rowStart_ = nullptr;  // SCREEN_H + 1 entries: where each row's runs begin
  uint16_t* runs_ = nullptr;      // each run: palette index in the top byte, length below
  uint16_t palette_[256] = {};
};

// Makes a temporary full-screen sprite in PSRAM to draw a layer into.
bool createLayerSprite(LGFX_Sprite& sprite);

// ---- Drawing helpers used by every screen -----------------------------------
//
//  !! IMPORTANT for anyone adding drawing code !!
//  Never call g.drawWideLine(), g.drawSmoothLine(), g.drawWedgeLine() or
//  g.drawSpot() on the canvas. Those library functions replace the clip
//  rectangle with their own and draw outside the current strip, which
//  corrupts memory and crashes the board. Use wideLine() / wedgeLine() below.
//  Everything else (fillSmoothCircle, drawArc, fillTriangle, drawCircle,
//  pushSprite, ...) respects the strip and is safe.

// Turns a touch on the physical glass into picture coordinates (SCREEN_ROTATION).
void rotateTouch(int* x, int* y);

// Is any part of rows [y, y+h) inside the strip being drawn right now?
bool rowsVisible(Gfx& g, int y, int h);

// Text with a smooth font. Returns the width in pixels.
int text(Gfx& g, const char* str, int x, int y, theme::Font font, uint16_t color,
         Align align = Align::Left);
int textWidth(Gfx& g, const char* str, theme::Font font);
int fontHeight(Gfx& g, theme::Font font);

// Anti-aliased line whose thickness changes from `ra` at one end to `rb` at
// the other (contrails, the radar sweep). Safe replacement for drawWedgeLine.
void wedgeLine(Gfx& g, float ax, float ay, float bx, float by, float ra, float rb, uint16_t color);

// Anti-aliased line of constant thickness. Safe replacement for drawWideLine.
inline void wideLine(Gfx& g, float ax, float ay, float bx, float by, float r, uint16_t color) {
  wedgeLine(g, ax, ay, bx, by, r, r, color);
}

// A triangle filled directly (much faster than g.fillTriangle for big ones).
// `amount` < 1 blends it over what's there instead of painting solid.
// Triangles that share an edge tile perfectly: no gaps, no double-painting.
void triangle(Gfx& g, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color,
              float amount = 1.0f);

// Mix a rectangle toward `color` by `amount` (0..1) — frosted glass, dimming.
void blendRect(Gfx& g, int x, int y, int w, int h, uint16_t color, float amount);

// Halve the brightness of a rectangle. Much cheaper than blendRect.
void darkenRect(Gfx& g, int x, int y, int w, int h);

// A filled rectangle with rounded, anti-aliased corners, drawn directly.
// Use this instead of g.fillSmoothRoundRect() for anything drawn every frame.
void roundedRect(Gfx& g, int x, int y, int w, int h, int radius, uint16_t color);

// A rounded panel with a soft shadow and a thin highlight along the top edge.
void glassPanel(Gfx& g, int x, int y, int w, int h, int radius, uint16_t fill);

// A dot with a soft glow around it (planes, the home marker, selections).
void glowDot(Gfx& g, float x, float y, float radius, uint16_t color, float strength);

// Aircraft silhouette pointing along `headingDeg`, about `size` pixels long.
void planeIcon(Gfx& g, float x, float y, float headingDeg, float size, uint16_t color);

// Small up/down/level arrow for climb and descent.
void trendArrow(Gfx& g, int x, int y, int trend, uint16_t color);

// Signal-strength bars for Wi-Fi.
void wifiBars(Gfx& g, int x, int y, int rssi, uint16_t on, uint16_t off);

// Builds the plane silhouette. Call once at start-up.
bool initIcons();

}  // namespace ui
