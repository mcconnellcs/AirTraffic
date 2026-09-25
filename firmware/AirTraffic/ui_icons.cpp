// =============================================================================
//  ui_icons.cpp  —  The aircraft symbol, rotated to any heading, quickly
// =============================================================================
//  Rotating a picture smoothly is slow, and we draw dozens of planes per
//  frame. So the first time we need a plane at a given size and heading we
//  rotate it once and keep the result (a "stencil": how much ink goes on
//  each pixel). Every frame after that just stamps the stencil, like text.
//  Headings are rounded to 5 degrees: 72 stencils cover every direction.
// =============================================================================
#include <esp_heap_caps.h>
#include <math.h>

#include <vector>

#include "ui_canvas.h"
#include "ui_pixels.h"

namespace ui {

namespace {

constexpr int kSourceSize = 64;      // the plane is drawn big once, then shrunk smoothly
constexpr int kHeadingSteps = 72;    // 360 / 5 degrees
constexpr int kScratchSize = 96;
constexpr size_t kMaxStencils = 600;

struct Stencil {
  uint8_t size;
  uint8_t step;
  int16_t dim;       // stencil is dim x dim pixels, centred on the plane
  uint8_t* alpha;    // dim*dim bytes in PSRAM
};

LGFX_Sprite planeSprite;   // the master drawing, 1 bit per pixel
LGFX_Sprite scratch;       // where we rotate into
std::vector<Stencil> stencils;

void drawMasterPlane() {
  planeSprite.fillScreen(0);
  const int c = kSourceSize / 2;
  planeSprite.fillRoundRect(c - 4, 2, 8, 58, 4, 1);           // body
  planeSprite.fillTriangle(c, 18, 1, 38, 63, 38, 1);           // wings
  planeSprite.fillRect(1, 36, 62, 4, 1);
  planeSprite.fillTriangle(c, 46, c - 15, 60, c + 15, 60, 1);  // tail
  planeSprite.fillRect(c - 15, 58, 30, 3, 1);
  planeSprite.setPivot(c, c);
}

const Stencil* makeStencil(int size, int step) {
  const int dim = std::min(kScratchSize, static_cast<int>(ceilf(size * 1.5f)) + 4);
  uint8_t* alpha = static_cast<uint8_t*>(heap_caps_malloc(dim * dim, MALLOC_CAP_SPIRAM));
  if (alpha == nullptr) return nullptr;

  // Rotate the master plane (white on black) into the scratch picture...
  scratch.fillScreen(0);
  planeSprite.setPaletteColor(1, static_cast<uint16_t>(0xFFFF));
  const float zoom = static_cast<float>(size) / kSourceSize;
  planeSprite.pushRotateZoomWithAA(&scratch, dim / 2.0f, dim / 2.0f, step * 5.0f, zoom, zoom, 0);

  // ...then keep only "how white" each pixel is. That's our stencil.
  const uint16_t* pixels = static_cast<const uint16_t*>(scratch.getBuffer());
  for (int y = 0; y < dim; y++) {
    for (int x = 0; x < dim; x++) {
      const uint16_t v = swapBytes(pixels[y * kScratchSize + x]);
      alpha[y * dim + x] = static_cast<uint8_t>(((v >> 5) & 0x3F) * 255 / 63);
    }
  }
  if (stencils.size() >= kMaxStencils) {  // very unlikely, but never grow forever
    heap_caps_free(stencils.front().alpha);
    stencils.erase(stencils.begin());
  }
  stencils.push_back({static_cast<uint8_t>(size), static_cast<uint8_t>(step),
                      static_cast<int16_t>(dim), alpha});
  return &stencils.back();
}

const Stencil* stencilFor(int size, int step) {
  for (const Stencil& s : stencils) {
    if (s.size == size && s.step == step) return &s;
  }
  return makeStencil(size, step);
}

}  // namespace

bool initIcons() {
  planeSprite.setPsram(false);
  planeSprite.setColorDepth(1);
  if (!planeSprite.createSprite(kSourceSize, kSourceSize)) return false;
  drawMasterPlane();
  scratch.setPsram(true);
  scratch.setColorDepth(16);
  if (!scratch.createSprite(kScratchSize, kScratchSize)) return false;
  stencils.reserve(128);
  return true;
}

void planeIcon(Gfx& g, float x, float y, float headingDeg, float size, uint16_t color) {
  const int sz = std::max(6, std::min(90, static_cast<int>(size + 0.5f)));
  int step = static_cast<int>(lroundf(headingDeg / 5.0f)) % kHeadingSteps;
  if (step < 0) step += kHeadingSteps;
  const Stencil* st = stencilFor(sz, step);
  if (st == nullptr) return;

  const int left = static_cast<int>(x) - st->dim / 2, top = static_cast<int>(y) - st->dim / 2;
  const ClipBox box = clipBox(g, left, top, st->dim, st->dim);
  if (box.empty()) return;
  const Ink ink(color);
  for (int row = box.y0; row < box.y1; row++) {
    const uint8_t* src = st->alpha + (row - top) * st->dim + (box.x0 - left);
    uint16_t* dst = pixelAt(g, box.x0, row);
    for (int col = box.x0; col < box.x1; col++, src++, dst++) {
      if (*src) blendPixel(dst, ink, alpha256(*src));
    }
  }
}

}  // namespace ui
