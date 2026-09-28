#include "doctest.h"
#include "screenshot_protocol.h"

#include <vector>

TEST_CASE("screenshot runs preserve colours and byte order") {
  const uint16_t pixels[] = {0x1234, 0x1234, 0xabcd};
  uint8_t encoded[9]{};
  REQUIRE(screenshot::encodeRuns(pixels, 3, encoded) == 6);
  CHECK(encoded[0] == 2);
  CHECK(encoded[1] == 0x34);
  CHECK(encoded[2] == 0x12);
  CHECK(encoded[3] == 1);
  CHECK(encoded[4] == 0xcd);
  CHECK(encoded[5] == 0xab);
}

TEST_CASE("screenshot runs split at 255 pixels and fit the worst-case buffer") {
  std::vector<uint16_t> pixels(480 * 480, 0xbeef);
  std::vector<uint8_t> encoded(pixels.size() * 3);
  const size_t length = screenshot::encodeRuns(pixels.data(), pixels.size(), encoded.data());
  size_t decoded = 0;
  for (size_t i = 0; i < length; i += 3) {
    REQUIRE(encoded[i] > 0);
    CHECK(encoded[i + 1] == 0xef);
    CHECK(encoded[i + 2] == 0xbe);
    decoded += encoded[i];
  }
  CHECK(decoded == pixels.size());
  CHECK(length == ((pixels.size() + 254) / 255) * 3);
  for (size_t i = 0; i < pixels.size(); i++) pixels[i] = i % 2;
  CHECK(screenshot::encodeRuns(pixels.data(), pixels.size(), encoded.data()) == encoded.size());
  CHECK(screenshot::encodeRuns(nullptr, 0, nullptr) == 0);
}
