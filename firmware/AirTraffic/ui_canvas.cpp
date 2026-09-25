#include "ui_canvas.h"

#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <math.h>
#include <string.h>

#include <algorithm>

#include "anim.h"
#include "ui_pixels.h"

namespace ui {

// ---- Canvas -----------------------------------------------------------------

namespace {

// Copies one finished strip to the screen, turning it to match SCREEN_ROTATION.
// Unrotated strips are a straight copy. Rotated ones are written one screen
// row at a time (48 pixels each), which keeps the slow PSRAM writes tidy.
void pushStrip(LGFX& display, const uint16_t* strip, int y0) {
  Panel& panel = display.panel();
  if (SCREEN_ROTATION == 0) {  // straight copy, one screen row at a time
    for (int r = 0; r < Canvas::kStripRows; r++) {
      memcpy(panel.row(y0 + r), strip + r * SCREEN_W, SCREEN_W * sizeof(uint16_t));
    }
    return;
  }
  if (SCREEN_ROTATION == 2) {  // upside down: rows reversed, and each row back to front
    for (int r = 0; r < Canvas::kStripRows; r++) {
      const uint16_t* src = strip + r * SCREEN_W;
      uint16_t* dst = panel.row(SCREEN_H - 1 - (y0 + r));
      for (int x = 0; x < SCREEN_W; x++) dst[x] = src[SCREEN_W - 1 - x];
    }
    return;
  }
  // 90 degrees either way: the strip becomes a vertical band of 48 columns.
  // Each screen row gets 48 pixels gathered from one picture column.
  const int px0 = SCREEN_ROTATION == 3 ? y0 : SCREEN_W - Canvas::kStripRows - y0;
  for (int py = 0; py < SCREEN_H; py++) {
    const int x = SCREEN_ROTATION == 3 ? SCREEN_W - 1 - py : py;  // picture column for this row
    const uint16_t* column = strip + x;
    uint16_t* dst = panel.row(py) + px0;
    for (int c = 0; c < Canvas::kStripRows; c++) dst[c] = column[c * SCREEN_W];
  }
}

}  // namespace

bool Canvas::begin(LGFX* display) {
  display_ = display;
  const size_t bytes = SCREEN_W * kStripRows * sizeof(uint16_t);
  for (int i = 0; i < 2; i++) {
    strips_[i] = static_cast<uint16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    free_[i] = xSemaphoreCreateBinary();
    if (strips_[i] == nullptr || free_[i] == nullptr) return false;
    xSemaphoreGive(free_[i]);
  }
  jobs_ = xQueueCreate(2, sizeof(Job));
  if (jobs_ == nullptr) return false;
  gfx_.setColorDepth(16);
  xSemaphoreTake(free_[current_], portMAX_DELAY);  // we own buffer 0 to start with
  // The copier lives on core 0 (Arduino's loop() runs on core 1), at the same
  // priority as the download task so the two share the core fairly. (At a
  // higher priority it starved the downloads and HTTPS connections timed out.)
  return xTaskCreatePinnedToCore(pushTask, "strip_push", 3072, this, 1, nullptr, 0) == pdPASS;
}

void Canvas::pushTask(void* self) {
  Canvas& canvas = *static_cast<Canvas*>(self);
  // Core 0 is deliberately kept busy (copying strips + downloads), so its idle
  // task rarely runs and the default watchdog would reboot us for that. Tell
  // the watchdog to stop watching the idle tasks and watch THIS task instead:
  // if a copy ever gets stuck for 8 seconds, the board restarts.
  esp_task_wdt_config_t config = {};
  config.timeout_ms = 8000;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  const bool watched = esp_task_wdt_reconfigure(&config) == ESP_OK && esp_task_wdt_add(nullptr) == ESP_OK;
  Serial.printf("[canvas] strip copier %s by the watchdog\n", watched ? "guarded" : "not guarded");
  Job job;
  for (;;) {
    if (watched) esp_task_wdt_reset();
    if (xQueueReceive(canvas.jobs_, &job, pdMS_TO_TICKS(1000)) != pdTRUE) continue;
    uint16_t* strip = canvas.strips_[job.buffer];
    pushStrip(*canvas.display_, strip, job.y0);

    // Screenshots copy a whole frame, starting only at the first strip.
    if (job.y0 == 0) canvas.capturing_ = canvas.capture_ != nullptr;
    if (canvas.capturing_) {
      memcpy(canvas.capture_ + job.y0 * SCREEN_W, strip, SCREEN_W * kStripRows * sizeof(uint16_t));
      if (job.y0 + kStripRows >= SCREEN_H) {
        canvas.capturing_ = false;
        canvas.capture_ = nullptr;  // frame finished
      }
    }
    xSemaphoreGive(canvas.free_[job.buffer]);
    if (job.y0 + kStripRows >= SCREEN_H) vTaskDelay(1);  // a breather for Wi-Fi once per frame
  }
}

void Canvas::beginLayer(int y0, int offsetX) {
  // Point the sprite at a "virtual" 480x480 picture whose rows y0..y0+47
  // land exactly on our small strip, shifted sideways by offsetX.
  // The clip rectangle makes sure nothing is ever written outside the strip.
  uint16_t* virtualBase = strips_[current_] - y0 * SCREEN_W + offsetX;
  gfx_.setBuffer(virtualBase, SCREEN_W, SCREEN_H, 16);
  const int left = offsetX < 0 ? -offsetX : 0;
  const int right = offsetX > 0 ? SCREEN_W - offsetX : SCREEN_W;
  gfx_.setClipRect(left, y0, right - left, kStripRows);
}

// A quick fingerprint of a strip: if it matches last frame's, the screen
// already shows it and the (slow) copy to screen memory can be skipped.
static uint32_t fingerprint(const uint16_t* strip) {
  const uint32_t* words = reinterpret_cast<const uint32_t*>(strip);
  uint32_t hash = 0;
  for (size_t i = 0; i < SCREEN_W * Canvas::kStripRows / 2; i++) hash = hash * 31 + words[i];
  return hash;
}

void Canvas::endStrip(int y0) {
  const uint32_t hash = fingerprint(strips_[current_]);
  const int index = y0 / kStripRows;
  if (hash == stripHash_[index] && capture_ == nullptr) return;  // unchanged: nothing to copy
  stripHash_[index] = hash;
  const Job job{current_, y0};
  xQueueSend(jobs_, &job, portMAX_DELAY);          // hand it to core 0
  current_ ^= 1;                                   // and draw the next strip into the other buffer
  xSemaphoreTake(free_[current_], portMAX_DELAY);  // (once core 0 has finished with it)
}

// ---- Layers -------------------------------------------------------------------

bool createLayerSprite(LGFX_Sprite& sprite) {
  sprite.setPsram(true);
  sprite.setColorDepth(16);
  return sprite.createSprite(SCREEN_W, SCREEN_H) != nullptr;
}

namespace {

constexpr int kMaxRun = 255;

// How far apart two colors look (roughly). Used to pick the nearest palette entry.
uint32_t colorDistance(uint16_t a, uint16_t b) {
  const int dr = ((a >> 11) & 0x1F) - ((b >> 11) & 0x1F);
  const int dg = ((a >> 5) & 0x3F) - ((b >> 5) & 0x3F);
  const int db = (a & 0x1F) - (b & 0x1F);
  return dr * dr * 4 + dg * dg + db * db * 4;
}

}  // namespace

bool Layer::encode(const LGFX_Sprite& sprite) {
  const uint16_t* pixels = static_cast<const uint16_t*>(sprite.getBuffer());
  const size_t total = static_cast<size_t>(SCREEN_W) * SCREEN_H;

  // 1. Count how often every color appears (65,536 counters, briefly, in PSRAM).
  uint32_t* histogram = static_cast<uint32_t*>(heap_caps_calloc(65536, sizeof(uint32_t), MALLOC_CAP_SPIRAM));
  if (histogram == nullptr) return false;
  for (size_t i = 0; i < total; i++) histogram[pixels[i]]++;

  // 2. The 256 most common colors get a palette slot of their own.
  uint8_t* index = static_cast<uint8_t*>(heap_caps_malloc(65536, MALLOC_CAP_SPIRAM));
  if (index == nullptr) {
    heap_caps_free(histogram);
    return false;
  }
  int used = 0;
  for (; used < 256; used++) {
    uint32_t best = 0;
    int bestColor = -1;
    for (int c = 0; c < 65536; c++) {
      if (histogram[c] > best) {
        best = histogram[c];
        bestColor = c;
      }
    }
    if (bestColor < 0) break;
    palette_[used] = static_cast<uint16_t>(bestColor);
    histogram[bestColor] = 0;
  }
  // 3. Every color maps to its nearest palette entry (exact for the common ones).
  for (int c = 0; c < 65536; c++) {
    int nearest = 0;
    uint32_t nearestDistance = ~0u;
    for (int p = 0; p < used; p++) {
      const uint32_t d = colorDistance(static_cast<uint16_t>(c), palette_[p]);
      if (d < nearestDistance) {
        nearestDistance = d;
        nearest = p;
        if (d == 0) break;
      }
    }
    index[c] = static_cast<uint8_t>(nearest);
  }
  heap_caps_free(histogram);

  // 4. Count the runs, so we know how much memory to ask for, then write them.
  size_t count = 0;
  for (int pass = 0; pass < 2; pass++) {
    size_t n = 0;
    for (int y = 0; y < SCREEN_H; y++) {
      if (pass == 1) rowStart_[y] = n;
      const uint16_t* row = pixels + y * SCREEN_W;
      int start = 0;
      for (int x = 1; x <= SCREEN_W; x++) {
        if (x == SCREEN_W || index[row[x]] != index[row[start]] || x - start == kMaxRun) {
          if (pass == 1) runs_[n] = static_cast<uint16_t>((index[row[start]] << 8) | (x - start));
          n++;
          start = x;
        }
      }
    }
    if (pass == 0) {
      count = n;
      rowStart_ = static_cast<uint32_t*>(heap_caps_malloc((SCREEN_H + 1) * sizeof(uint32_t), MALLOC_CAP_SPIRAM));
      runs_ = static_cast<uint16_t*>(heap_caps_malloc(count * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
      if (rowStart_ == nullptr || runs_ == nullptr) {
        heap_caps_free(rowStart_);
        heap_caps_free(runs_);
        rowStart_ = nullptr;
        runs_ = nullptr;
        heap_caps_free(index);
        return false;
      }
    } else {
      rowStart_[SCREEN_H] = n;
    }
  }
  heap_caps_free(index);
  Serial.printf("[layer] %u colors, %u runs (%u KB)\n", used, static_cast<unsigned>(count),
                static_cast<unsigned>(count * 2 / 1024));
  return true;
}

void Layer::paint(Gfx& g) const {
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  const int right = cx + cw;
  for (int y = cy; y < cy + ch; y++) {
    uint16_t* row = pixelAt(g, 0, y);
    int x = 0;
    for (uint32_t i = rowStart_[y]; i < rowStart_[y + 1] && x < right; i++) {
      const uint16_t run = runs_[i];
      const int len = run & 0xFF;
      const int from = std::max<int>(x, cx), to = std::min<int>(x + len, right);
      if (from < to) std::fill(row + from, row + to, palette_[run >> 8]);
      x += len;
    }
  }
}

// ---- Helpers ----------------------------------------------------------------

void rotateTouch(int* x, int* y) {
  const int px = *x, py = *y;
  switch (SCREEN_ROTATION) {
    case 1: *x = py; *y = SCREEN_H - 1 - px; break;
    case 2: *x = SCREEN_W - 1 - px; *y = SCREEN_H - 1 - py; break;
    case 3: *x = SCREEN_W - 1 - py; *y = px; break;
    default: break;
  }
}

bool rowsVisible(Gfx& g, int y, int h) {
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  return y < cy + ch && y + h > cy;
}

void wedgeLine(Gfx& g, float ax, float ay, float bx, float by, float ra, float rb, uint16_t color) {
  const float rmax = std::max(ra, rb) + 1.0f;
  const int bx0 = static_cast<int>(floorf(std::min(ax, bx) - rmax));
  const int by0 = static_cast<int>(floorf(std::min(ay, by) - rmax));
  const int bx1 = static_cast<int>(ceilf(std::max(ax, bx) + rmax)) + 1;
  const int by1 = static_cast<int>(ceilf(std::max(ay, by) + rmax)) + 1;
  const ClipBox box = clipBox(g, bx0, by0, bx1 - bx0, by1 - by0);
  if (box.empty()) return;

  const float dx = bx - ax, dy = by - ay;
  const float len2 = dx * dx + dy * dy;
  const float invLen2 = len2 > 1e-6f ? 1.0f / len2 : 0.0f;
  const float invLen = len2 > 1e-6f ? 1.0f / sqrtf(len2) : 0.0f;
  const Ink ink(color);

  for (int yp = box.y0; yp < box.y1; yp++) {
    const float yc = yp + 0.5f;
    // Only look at the part of this row the line can actually reach.
    float xmin = std::min(ax, bx), xmax = std::max(ax, bx);
    if (fabsf(dy) > 1e-3f) {
      const float t1 = anim::clamp01((yc - rmax - ay) / dy), t2 = anim::clamp01((yc + rmax - ay) / dy);
      xmin = ax + std::min(t1, t2) * dx;
      xmax = ax + std::max(t1, t2) * dx;
    }
    const int x0 = std::max(box.x0, static_cast<int>(floorf(xmin - rmax)));
    const int x1 = std::min(box.x1, static_cast<int>(ceilf(xmax + rmax)) + 1);
    uint16_t* row = pixelAt(g, 0, yp);
    for (int xp = x0; xp < x1; xp++) {
      const float px = xp + 0.5f - ax, py = yc - ay;
      const float t = (px * dx + py * dy) * invLen2;  // where along the line we are
      float dist, r;
      if (t <= 0.0f) {                                  // past the start: round cap
        dist = sqrtf(px * px + py * py);
        r = ra;
      } else if (t >= 1.0f) {                           // past the end: round cap
        const float qx = px - dx, qy = py - dy;
        dist = sqrtf(qx * qx + qy * qy);
        r = rb;
      } else {                                          // alongside: no square root needed
        dist = fabsf(px * dy - py * dx) * invLen;
        r = ra + (rb - ra) * t;
      }
      const float coverage = r + 0.5f - dist;  // how much of this pixel the line covers
      if (coverage > 0.0f) blendPixel(row + xp, ink, static_cast<uint32_t>(std::min(coverage, 1.0f) * 256.0f));
    }
  }
}

void triangle(Gfx& g, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color,
              float amount) {
  // Sort the corners top to bottom, then fill each row between the two edges.
  // A pixel counts as "inside" when its centre is, so neighbouring triangles
  // never overlap or leave a gap.
  if (y1 < y0) { std::swap(x0, x1); std::swap(y0, y1); }
  if (y2 < y0) { std::swap(x0, x2); std::swap(y0, y2); }
  if (y2 < y1) { std::swap(x1, x2); std::swap(y1, y2); }
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  const int top = std::max<int>(cy, static_cast<int>(ceilf(y0 - 0.5f)));
  const int bottom = std::min<int>(cy + ch - 1, static_cast<int>(ceilf(y2 - 0.5f)) - 1);
  if (top > bottom || y2 <= y0) return;

  const bool solid = amount >= 0.999f;
  const uint16_t swapped = swapBytes(color);
  const Ink ink(color);
  const uint32_t a = static_cast<uint32_t>(anim::clamp01(amount) * 256.0f + 0.5f);
  // Slopes of the three edges (divisions are slow, so do them once).
  const float slopeLong = (x2 - x0) / (y2 - y0);
  const float slopeTop = y1 > y0 ? (x1 - x0) / (y1 - y0) : 0.0f;
  const float slopeBottom = y2 > y1 ? (x2 - x1) / (y2 - y1) : 0.0f;
  for (int y = top; y <= bottom; y++) {
    const float yc = y + 0.5f;
    // The long edge (0->2) is always one side; the other is 0->1 then 1->2.
    const float xa = x0 + slopeLong * (yc - y0);
    const float xb = yc < y1 ? (y1 > y0 ? x0 + slopeTop * (yc - y0) : x1)
                             : (y2 > y1 ? x1 + slopeBottom * (yc - y1) : x2);
    const int left = std::max<int>(cx, static_cast<int>(ceilf(std::min(xa, xb) - 0.5f)));
    const int right = std::min<int>(cx + cw - 1, static_cast<int>(ceilf(std::max(xa, xb) - 0.5f)) - 1);
    if (left > right) continue;
    uint16_t* p = pixelAt(g, left, y);
    if (solid) {
      std::fill(p, p + (right - left + 1), swapped);
    } else {
      for (int x = left; x <= right; x++) blendPixel(p++, ink, a);
    }
  }
}

void blendRect(Gfx& g, int x, int y, int w, int h, uint16_t color, float amount) {
  const ClipBox box = clipBox(g, x, y, w, h);
  if (box.empty()) return;
  const uint32_t a = static_cast<uint32_t>(anim::clamp01(amount) * 256.0f + 0.5f);
  const Ink ink(color);
  for (int row = box.y0; row < box.y1; row++) {
    uint16_t* p = pixelAt(g, box.x0, row);
    for (int col = box.x0; col < box.x1; col++, p++) blendPixel(p, ink, a);
  }
}

void darkenRect(Gfx& g, int x, int y, int w, int h) {
  const ClipBox box = clipBox(g, x, y, w, h);
  if (box.empty()) return;
  for (int row = box.y0; row < box.y1; row++) {
    uint16_t* p = pixelAt(g, box.x0, row);
    for (int col = box.x0; col < box.x1; col++, p++) {
      // Shift every channel right by one = half brightness (mask keeps the
      // channels from bleeding into each other). Works on the swapped bytes.
      *p = swapBytes(static_cast<uint16_t>((swapBytes(*p) >> 1) & 0x7BEF));
    }
  }
}

void roundedRect(Gfx& g, int x, int y, int w, int h, int radius, uint16_t color) {
  const ClipBox box = clipBox(g, x, y, w, h);
  if (box.empty()) return;
  radius = std::min(radius, std::min(w, h) / 2);
  const uint16_t swapped = swapBytes(color);
  const Ink ink(color);
  for (int row = box.y0; row < box.y1; row++) {
    // How far the rounded corner pulls this row in from the left/right edges.
    float inset = 0.0f;
    const int fromTop = row - y, fromBottom = y + h - 1 - row;
    const int d = std::min(fromTop, fromBottom);
    if (d < radius) {
      const float dy = radius - 0.5f - d;
      inset = radius - sqrtf(std::max(0.0f, radius * radius - dy * dy));
    }
    const int left = x + static_cast<int>(inset), right = x + w - 1 - static_cast<int>(inset);
    const float edge = 1.0f - (inset - static_cast<int>(inset));  // coverage of the edge pixel
    const int fillFrom = std::max(box.x0, left + 1), fillTo = std::min(box.x1 - 1, right - 1);
    if (fillFrom <= fillTo) std::fill(pixelAt(g, fillFrom, row), pixelAt(g, fillTo, row) + 1, swapped);
    if (left >= box.x0 && left < box.x1) blendPixel(pixelAt(g, left, row), ink, static_cast<uint32_t>(edge * 256));
    if (right != left && right >= box.x0 && right < box.x1) blendPixel(pixelAt(g, right, row), ink, static_cast<uint32_t>(edge * 256));
  }
}

void glassPanel(Gfx& g, int x, int y, int w, int h, int radius, uint16_t fill) {
  if (!rowsVisible(g, y - 12, h + 24)) return;
  // Strips entirely inside the flat middle of the panel are just a fill.
  int32_t cx, cy, cw, ch;
  g.getClipRect(&cx, &cy, &cw, &ch);
  if (cy >= y + radius + 4 && cy + ch <= y + h - radius - 4) {
    g.fillRect(x, cy, w, ch, fill);
    return;
  }
  // Soft shadow: a few slightly bigger, darker outlines behind the panel.
  for (int i = 3; i >= 1; i--) {
    roundedRect(g, x - i * 2, y - i + 4, w + i * 4, h + i * 2, radius + i * 2,
                anim::blend565(theme::kBackground, 0x0000, 0.25f * (4 - i)));
  }
  roundedRect(g, x, y, w, h, radius, fill);
  // Highlight: a thin brighter line along the top, like light catching glass.
  g.drawGradientHLine(x + radius, y, w - 2 * radius, anim::blend565(fill, theme::kText, 0.10f),
                      anim::blend565(fill, theme::kText, 0.22f));
}

namespace {

// How bright a glow is at each (squared) distance from its centre, worked
// out once per radius. Squared distances avoid a slow square root per pixel.
struct GlowProfile {
  int radius = 0;
  int reach = 0;            // pixels from the centre where the glow ends
  int entries = 0;          // reach*reach + 1
  uint8_t* core = nullptr;  // the solid dot with a soft edge
  uint8_t* halo = nullptr;  // the fading light around it
};

constexpr int kMaxProfiles = 8;
GlowProfile profiles[kMaxProfiles];

const GlowProfile* glowProfile(int radius) {
  for (const GlowProfile& p : profiles) {
    if (p.radius == radius) return &p;
  }
  for (GlowProfile& p : profiles) {
    if (p.radius != 0) continue;
    const float outer = radius * 2.8f;
    p.radius = radius;
    p.reach = static_cast<int>(ceilf(outer)) + 1;
    p.entries = p.reach * p.reach + 1;
    p.core = static_cast<uint8_t*>(heap_caps_malloc(p.entries, MALLOC_CAP_INTERNAL));
    p.halo = static_cast<uint8_t*>(heap_caps_malloc(p.entries, MALLOC_CAP_INTERNAL));
    if (p.core == nullptr || p.halo == nullptr) return nullptr;
    for (int d2 = 0; d2 < p.entries; d2++) {
      const float d = sqrtf(static_cast<float>(d2));
      const float t = anim::clamp01(1.0f - (d - radius) / (outer - radius));
      p.core[d2] = static_cast<uint8_t>(255.0f * anim::clamp01(radius + 0.5f - d));
      p.halo[d2] = d <= radius + 0.5f ? 0 : static_cast<uint8_t>(255.0f * 0.55f * t * t);
    }
    return &p;
  }
  return nullptr;
}

}  // namespace

void glowDot(Gfx& g, float x, float y, float radius, uint16_t color, float strength) {
  const GlowProfile* p = glowProfile(std::max(1, static_cast<int>(radius + 0.5f)));
  if (p == nullptr) return;
  const int xi = static_cast<int>(lroundf(x)), yi = static_cast<int>(lroundf(y));
  const ClipBox box = clipBox(g, xi - p->reach, yi - p->reach, p->reach * 2 + 1, p->reach * 2 + 1);
  if (box.empty()) return;

  const uint32_t s = static_cast<uint32_t>(anim::clamp01(strength) * 256.0f);
  const uint32_t coreGain = 77 + (179 * s >> 8);  // 0.3 + 0.7 * strength, in 256ths
  const Ink ink(color);
  for (int row = box.y0; row < box.y1; row++) {
    const int dy2 = (row - yi) * (row - yi);
    uint16_t* pix = pixelAt(g, box.x0, row);
    for (int col = box.x0; col < box.x1; col++, pix++) {
      const int d2 = (col - xi) * (col - xi) + dy2;
      if (d2 >= p->entries) continue;
      const uint32_t a = (p->core[d2] * coreGain + p->halo[d2] * s) >> 8;
      if (a) blendPixel(pix, ink, alpha256(std::min<uint32_t>(a, 255)));
    }
  }
}

void trendArrow(Gfx& g, int x, int y, int trend, uint16_t color) {
  if (trend > 0) {
    g.fillTriangle(x, y - 6, x - 5, y + 3, x + 5, y + 3, color);
  } else if (trend < 0) {
    g.fillTriangle(x, y + 6, x - 5, y - 3, x + 5, y - 3, color);
  } else {
    g.fillRoundRect(x - 5, y - 1, 10, 3, 1, color);
  }
}

void wifiBars(Gfx& g, int x, int y, int rssi, uint16_t on, uint16_t off) {
  const int level = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi >= -85 ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    const int h = 4 + i * 3;
    g.fillRoundRect(x + i * 5, y + 13 - h, 3, h, 1, i < level ? on : off);
  }
}

}  // namespace ui
