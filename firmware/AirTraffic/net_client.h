// =============================================================================
//  net_client.h  —  Download things from the internet (securely, over HTTPS)
// =============================================================================
#pragma once

#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

namespace net {

enum class Result { Ok, NoWifi, ConnectFailed, HttpError, BadJson, TooBig };

const char* describe(Result result);

// ArduinoJson normally uses the ESP32's small internal memory. This makes it
// use the big 8 MB PSRAM chip instead, so large replies never run us out.
ArduinoJson::Allocator* psramAllocator();

// GET a URL and parse the JSON reply. `filter` (optional) keeps only some fields.
Result getJson(const char* url, JsonDocument& doc, const JsonDocument* filter = nullptr,
               int* httpCode = nullptr);

// GET a URL into a PSRAM buffer (used for aircraft photos). Caller frees with free().
Result getBytes(const char* url, uint8_t** data, size_t* length, size_t maxBytes);

}  // namespace net
