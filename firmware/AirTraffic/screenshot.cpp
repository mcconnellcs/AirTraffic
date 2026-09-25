#include "screenshot.h"

#include <esp_heap_caps.h>

namespace screenshot {

namespace {

enum class State { Idle, Capturing };

State state = State::Idle;
uint16_t* frame = nullptr;
char command[8];
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

}  // namespace

void poll(ui::Canvas& canvas) {
  if (state == State::Capturing && !canvas.captureBusy()) {
    sendFrame();
    heap_caps_free(frame);
    frame = nullptr;
    state = State::Idle;
  }

  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c != '\n' && c != '\r') {
      if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
      continue;
    }
    command[commandLength] = '\0';
    commandLength = 0;
    if (strcmp(command, "shot") != 0 || state != State::Idle) continue;

    frame = static_cast<uint16_t*>(
        heap_caps_malloc(SCREEN_W * SCREEN_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
    if (frame == nullptr) {
      Serial.println("[shot] not enough memory");
      continue;
    }
    canvas.captureNextFrame(frame);
    state = State::Capturing;
  }
}

}  // namespace screenshot
