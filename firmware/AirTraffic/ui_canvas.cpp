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
  // The copier lives on core 0 (Arduino's loop() runs on core 1).
  return xTaskCreatePinnedToCore(pushTask, "strip_push", 3072, this, 2, nullptr, 0) == pdPASS;
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
    canvas.display_->pushImage(0, job.y0, SCREEN_W, kStripRows, reinterpret_cast<lgfx::swap565_t*>(strip));

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

void Canvas::endStrip(int y0) {
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
    g.fillSmoothRoundRect(x - i * 2, y - i + 4, w + i * 4, h + i * 2, radius + i * 2,
                          anim::blend565(theme::kBackground, 0x0000, 0.25f * (4 - i)));
  }
  g.fillSmoothRoundRect(x, y, w, h, radius, fill);
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
