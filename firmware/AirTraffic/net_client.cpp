#include "net_client.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

// The ESP32 carries a list of trusted certificate authorities (the same idea as
// a web browser). This lets us check that we're really talking to the right
// website — nobody can pretend to be adsb.lol.
extern const uint8_t kCertBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t kCertBundleEnd[] asm("_binary_x509_crt_bundle_end");

namespace net {

namespace {

constexpr uint32_t kTimeoutMs = 10000;
constexpr const char* kUserAgent = "AirTraffic-ESP32/1.0 (+https://github.com/mcconnellcs/AirTraffic)";

struct SpiRamAllocator : ArduinoJson::Allocator {
  void* allocate(size_t size) override { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
  void deallocate(void* ptr) override { heap_caps_free(ptr); }
  void* reallocate(void* ptr, size_t size) override {
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
  }
};

SpiRamAllocator spiRamAllocator;

// Opens a GET request. Returns the HTTP status code, or a negative error.
int beginGet(HTTPClient& http, NetworkClientSecure& client, const char* url) {
  client.setCACertBundle(kCertBundleStart, kCertBundleEnd - kCertBundleStart);
  client.setTimeout(kTimeoutMs / 1000);
  http.setTimeout(kTimeoutMs);
  http.setConnectTimeout(kTimeoutMs);
  http.setUserAgent(kUserAgent);
  http.useHTTP10(true);  // no "chunked" replies, so we can read the stream directly
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return -1;
  http.addHeader("Accept", "application/json");
  return http.GET();
}

}  // namespace

const char* describe(Result result) {
  switch (result) {
    case Result::Ok: return "ok";
    case Result::NoWifi: return "no Wi-Fi";
    case Result::ConnectFailed: return "can't reach server";
    case Result::HttpError: return "server error";
    case Result::BadJson: return "unexpected reply";
    case Result::TooBig: return "reply too big";
  }
  return "unknown";
}

ArduinoJson::Allocator* psramAllocator() { return &spiRamAllocator; }

Result getJson(const char* url, JsonDocument& doc, const JsonDocument* filter, int* httpCode) {
  if (WiFi.status() != WL_CONNECTED) return Result::NoWifi;

  NetworkClientSecure client;
  HTTPClient http;
  const int code = beginGet(http, client, url);
  if (httpCode) *httpCode = code;
  if (code < 0) {
    Serial.printf("[net] %s -> %s\n", url, http.errorToString(code).c_str());
    http.end();
    return Result::ConnectFailed;
  }
  if (code != HTTP_CODE_OK && code != HTTP_CODE_NOT_FOUND) {
    Serial.printf("[net] %s -> HTTP %d\n", url, code);
    http.end();
    return Result::HttpError;
  }

  DeserializationError err =
      filter ? deserializeJson(doc, http.getStream(), DeserializationOption::Filter(*filter))
             : deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("[net] %s -> JSON error: %s\n", url, err.c_str());
    return Result::BadJson;
  }
  return Result::Ok;
}

Result getBytes(const char* url, uint8_t** data, size_t* length, size_t maxBytes) {
  *data = nullptr;
  *length = 0;
  if (WiFi.status() != WL_CONNECTED) return Result::NoWifi;

  NetworkClientSecure client;
  HTTPClient http;
  const int code = beginGet(http, client, url);
  if (code != HTTP_CODE_OK) {
    http.end();
    return code < 0 ? Result::ConnectFailed : Result::HttpError;
  }

  const int declared = http.getSize();
  const size_t capacity = declared > 0 ? static_cast<size_t>(declared) : maxBytes;
  if (capacity > maxBytes) {
    http.end();
    return Result::TooBig;
  }

  uint8_t* buffer = static_cast<uint8_t*>(heap_caps_malloc(capacity, MALLOC_CAP_SPIRAM));
  if (buffer == nullptr) {
    http.end();
    return Result::TooBig;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t received = 0;
  const uint32_t deadline = millis() + kTimeoutMs;
  while (received < capacity && millis() < deadline && (http.connected() || stream->available())) {
    const int n = stream->read(buffer + received, capacity - received);
    if (n > 0) {
      received += n;
    } else {
      delay(5);
    }
  }
  http.end();

  if (received == 0 || (declared > 0 && received != static_cast<size_t>(declared))) {
    heap_caps_free(buffer);
    return Result::ConnectFailed;
  }
  *data = buffer;
  *length = received;
  return Result::Ok;
}

}  // namespace net
