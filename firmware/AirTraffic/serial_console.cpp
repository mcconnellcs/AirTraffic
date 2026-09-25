#include "serial_console.h"

#include <esp_heap_caps.h>
#include <string.h>

#include <stdlib.h>

#include <WiFi.h>

#include "app.h"
#include "flight_feed.h"

namespace console {

namespace {

uint16_t* frame = nullptr;  // screenshot in progress, or nullptr
char command[32];
size_t commandLength = 0;

// Run-length encoding: "N copies of this color" instead of N separate pixels.
// The screen has big areas of one color, so this shrinks the data ~5-10x.
void sendFrame() {
  const size_t total = static_cast<size_t>(SCREEN_W) * SCREEN_H;
  Serial.printf("\nSHOT_BEGIN %d %d\n", SCREEN_W, SCREEN_H);
  uint8_t packet[3];
  size_t i = 0;
  while (i < total) {
    const uint16_t color = frame[i];
    uint8_t run = 1;
    while (i + run < total && frame[i + run] == color && run < 255) run++;
    packet[0] = run;
    packet[1] = color & 0xFF;
    packet[2] = color >> 8;
    Serial.write(packet, sizeof(packet));
    i += run;
  }
  Serial.print("\nSHOT_END\n");
}

void startScreenshot(ui::Canvas& canvas) {
  if (frame != nullptr) return;  // one at a time
  frame = static_cast<uint16_t*>(
      heap_caps_malloc(SCREEN_W * SCREEN_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
  if (frame == nullptr) {
    Serial.println("[console] not enough memory for a screenshot");
    return;
  }
  canvas.captureNextFrame(frame);
}

bool runGestureCommand(const char* cmd) {
  if (strncmp(cmd, "tap ", 4) == 0) {
    char* end = nullptr;
    const int x = static_cast<int>(strtol(cmd + 4, &end, 10));
    const int y = static_cast<int>(strtol(end, nullptr, 10));
    app::injectGesture({GestureType::Tap, x, y});
    return true;
  }
  if (strncmp(cmd, "swipe ", 6) == 0) {
    const char* dir = cmd + 6;
    GestureType type = strcmp(dir, "left") == 0    ? GestureType::SwipeLeft
                       : strcmp(dir, "right") == 0 ? GestureType::SwipeRight
                       : strcmp(dir, "up") == 0    ? GestureType::SwipeUp
                       : strcmp(dir, "down") == 0  ? GestureType::SwipeDown
                                                   : GestureType::None;
    if (type == GestureType::None) return false;
    app::injectGesture({type, SCREEN_W / 2, SCREEN_H / 2});
    return true;
  }
  if (strcmp(cmd, "hold") == 0) {
    app::injectGesture({GestureType::LongPress, SCREEN_W / 2, SCREEN_H / 2});
    return true;
  }
  return false;
}

void runCommand(const char* cmd, ui::Canvas& canvas) {
  if (runGestureCommand(cmd)) {
    Serial.printf("[console] ok: %s\n", cmd);
  } else if (strcmp(cmd, "shot") == 0) {
    startScreenshot(canvas);
  } else if (strcmp(cmd, "demo") == 0) {
    const bool on = !feed::isDemo();
    feed::setDemo(on);
    Serial.printf("[console] demo mode %s\n", on ? "ON - pretend planes" : "OFF - live data");
  } else if (strcmp(cmd, "status") == 0) {
    const FeedStatus st = feed::status();
    Serial.printf("[status] Wi-Fi %s to %s (AP %s, channel %d), IP %s, signal %d dBm\n",
                  WiFi.status() == WL_CONNECTED ? "connected" : "not connected", WiFi.SSID().c_str(),
                  WiFi.BSSIDstr().c_str(), WiFi.channel(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.printf("[status] feed: %s%s%s, source %s, %u in range, last update %lu s ago\n",
                  feed::stateName(st.state), st.lastError[0] ? " - " : "", st.lastError,
                  st.source[0] ? st.source : "-", static_cast<unsigned>(st.totalInRange),
                  st.lastUpdateMs ? (millis() - st.lastUpdateMs) / 1000 : 0);
    Serial.printf("[status] home %.4f, %.4f (%s), heap %u KB, psram %u KB\n", st.home.lat, st.home.lon,
                  st.city, ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
  } else if (strcmp(cmd, "scan") == 0) {
    Serial.println("[scan] looking for networks (takes a few seconds)...");
    const int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      Serial.printf("[scan] %-24s %4d dBm  ch %2d  %s\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                    WiFi.channel(i), WiFi.BSSIDstr(i).c_str());
    }
    WiFi.scanDelete();
  } else if (strcmp(cmd, "help") == 0) {
    Serial.println("[console] commands: status, scan, demo, shot, tap X Y, swipe left|right|up|down, hold, help");
  } else if (cmd[0] != '\0') {
    Serial.printf("[console] unknown command '%s' (try: help)\n", cmd);
  }
}

}  // namespace

void poll(ui::Canvas& canvas) {
  if (frame != nullptr && !canvas.captureBusy()) {  // the frame has been captured
    sendFrame();
    heap_caps_free(frame);
    frame = nullptr;
  }

  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c != '\n' && c != '\r') {
      if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
      continue;
    }
    command[commandLength] = '\0';
    commandLength = 0;
    runCommand(command, canvas);
  }
}

}  // namespace console
