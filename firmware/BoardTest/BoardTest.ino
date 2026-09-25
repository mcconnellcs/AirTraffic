// =============================================================================
//  BoardTest  —  Step 1: prove your board, screen and touch all work
// =============================================================================
//  Upload this BEFORE the main AirTraffic sketch. You should see:
//    1. Red, green, blue and white bars   -> the 16 color wires are OK
//    2. A spinning line + an FPS number   -> the graphics memory (PSRAM) is OK
//    3. Dots where you touch the screen   -> the touch chip is OK
//
//  Open Tools > Serial Monitor (115200 baud) to read the test report.
// =============================================================================
#include "board_config.h"

LGFX display;
LGFX_Sprite frame(&display);   // an off-screen "canvas" we draw on first

bool touchOk = false;

void drawColorBars() {
  const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
  const char* names[] = {"RED", "GREEN", "BLUE", "WHITE"};
  const int barH = SCREEN_H / 4;
  for (int i = 0; i < 4; i++) {
    display.fillRect(0, i * barH, SCREEN_W, barH, colors[i]);
    display.setTextColor(i == 3 ? TFT_BLACK : TFT_WHITE);
    display.setTextSize(3);
    display.drawString(names[i], 20, i * barH + barH / 2 - 12);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== AirTraffic BoardTest ===");
  Serial.printf("PSRAM: %u bytes free\n", ESP.getFreePsram());
  Serial.printf("Flash: %u bytes\n", ESP.getFlashChipSize());

  if (!display.init()) {
    Serial.println("FAIL: display.init() returned false");
  }
  display.setBrightness(200);
  drawColorBars();
  Serial.println("Showing color bars for 3 seconds...");
  delay(3000);

  frame.setPsram(true);
  frame.setColorDepth(16);
  if (!frame.createSprite(SCREEN_W, SCREEN_H)) {
    Serial.println("FAIL: could not make a 480x480 canvas. Is PSRAM set to 'OPI PSRAM'?");
  }

  uint16_t x, y;
  display.getTouch(&x, &y);  // wake the touch chip
  touchOk = display.touch() != nullptr;
  Serial.printf("Touch driver: %s\n", touchOk ? "OK" : "missing");
}

void loop() {
  static uint32_t frames = 0, lastReport = millis();
  static float fps = 0, angle = 0;
  static int dotX = -1, dotY = -1;

  uint16_t tx, ty;
  if (display.getTouch(&tx, &ty)) {
    dotX = tx; dotY = ty;
    Serial.printf("Touch at %d,%d\n", tx, ty);
  }

  frame.fillScreen(0x0841);
  const int cx = SCREEN_W / 2, cy = SCREEN_H / 2;
  for (int r = 60; r <= 220; r += 40) frame.drawCircle(cx, cy, r, 0x2A69);
  angle += 0.05f;
  frame.drawWideLine(cx, cy, cx + cosf(angle) * 220, cy + sinf(angle) * 220, 3, TFT_GREEN);
  if (dotX >= 0) frame.fillSmoothCircle(dotX, dotY, 18, TFT_ORANGE);

  frame.setTextColor(TFT_WHITE);
  frame.setTextSize(2);
  frame.setCursor(12, 12);
  frame.printf("FPS %.1f   Touch the screen!", fps);
  frame.pushSprite(0, 0);

  frames++;
  if (millis() - lastReport >= 1000) {
    fps = frames * 1000.0f / (millis() - lastReport);
    Serial.printf("FPS: %.1f\n", fps);
    frames = 0;
    lastReport = millis();
  }
}
