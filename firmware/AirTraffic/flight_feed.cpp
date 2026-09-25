#include "flight_feed.h"

#include <WiFi.h>
#include <esp_heap_caps.h>
#include <time.h>

#include <mutex>

#include "net_client.h"
#include "parsers.h"

namespace feed {

namespace {

// ---- Websites we use (all free, no account or key needed) --------------------
constexpr const char* kPrimaryUrl = "https://api.adsb.lol/v2/point/%.4f/%.4f/%d";
constexpr const char* kFallbackUrl = "https://opendata.adsb.fi/api/v3/lat/%.4f/lon/%.4f/dist/%d";
constexpr const char* kRouteUrl = "https://api.adsbdb.com/v0/callsign/%s";
constexpr const char* kAircraftUrl = "https://api.adsbdb.com/v0/aircraft/%s";
constexpr const char* kLocateUrl =
    "https://ipwho.is/?fields=success,city,latitude,longitude,timezone";

constexpr uint32_t kRetryLocateMs = 15000;
constexpr uint32_t kPrefetchGapMs = 1200;   // be polite: at most ~1 route lookup per second
constexpr size_t kCacheSize = 48;
constexpr size_t kPrefetchCount = 12;       // look up routes for the 12 nearest planes
constexpr size_t kMaxPhotoBytes = 80 * 1024;
constexpr int kMaxQueryRadiusNm = 250;

struct CacheEntry {
  char hex[8];
  Details details;
  uint32_t usedMs;
};

struct Request {
  char hex[8];
  char callsign[10];
};

std::mutex dataLock;   // protects everything in this block
AppSettings settings{};
bool settingsChanged = false;
FeedStatus currentStatus{};
Snapshot snapshot{};
std::vector<CacheEntry> cache;
std::vector<Request> prefetch;
Request focus{};
bool haveFocus = false;

std::mutex photoLock;  // protects the photo
char photoHex[8] = "";
uint8_t* photo = nullptr;
size_t photoLength = 0;

template <size_t N>
void copyText(char (&dest)[N], const char* src) {
  strncpy(dest, src ? src : "", N - 1);
  dest[N - 1] = '\0';
}

void setState(FeedState state, const char* error = "") {
  std::lock_guard<std::mutex> guard(dataLock);
  currentStatus.state = state;
  currentStatus.lastError = error;
}

CacheEntry* findEntry(const char* hex) {  // caller must hold dataLock
  for (CacheEntry& e : cache) {
    if (strcmp(e.hex, hex) == 0) return &e;
  }
  return nullptr;
}

CacheEntry& entryFor(const char* hex) {  // caller must hold dataLock
  if (CacheEntry* e = findEntry(hex)) return *e;
  if (cache.size() >= kCacheSize) {  // forget the one we used longest ago
    size_t oldest = 0;
    for (size_t i = 1; i < cache.size(); i++) {
      if (cache[i].usedMs < cache[oldest].usedMs) oldest = i;
    }
    cache.erase(cache.begin() + oldest);
  }
  CacheEntry e{};
  copyText(e.hex, hex);
  e.usedMs = millis();
  cache.push_back(e);
  return cache.back();
}

// ---- Where are we? ----------------------------------------------------------

bool locate(const AppSettings& s) {
  setState(FeedState::Locating);
  JsonDocument doc(net::psramAllocator());
  Geolocation where{};
  if (net::getJson(kLocateUrl, doc) == net::Result::Ok) where = parsers::parseGeolocation(doc);

  if (!where.valid && s.autoLocation) {
    setState(FeedState::Error, "can't find location");
    return false;
  }
  // Set the clock from the internet (NTP) using our time zone offset.
  configTime(where.valid ? where.utcOffsetSec : 0, 0, "pool.ntp.org", "time.google.com");

  std::lock_guard<std::mutex> guard(dataLock);
  currentStatus.haveLocation = true;
  if (s.autoLocation) {
    currentStatus.home = where.pos;
    copyText(currentStatus.city, where.city);
  } else {
    currentStatus.home = s.home;
    snprintf(currentStatus.city, sizeof(currentStatus.city), "%.2f, %.2f", s.home.lat, s.home.lon);
  }
  Serial.printf("[feed] home %.4f, %.4f (%s)\n", currentStatus.home.lat, currentStatus.home.lon,
                currentStatus.city);
  return true;
}

// ---- Flights ----------------------------------------------------------------

void queuePrefetch(const std::vector<Flight>& flights) {  // caller must hold dataLock
  prefetch.clear();
  for (size_t i = 0; i < flights.size() && prefetch.size() < kPrefetchCount; i++) {
    const CacheEntry* e = findEntry(flights[i].hex);
    if (e != nullptr && e->details.routeDone) continue;
    Request r{};
    copyText(r.hex, flights[i].hex);
    copyText(r.callsign, flights[i].callsign);
    prefetch.push_back(r);
  }
}

void fetchFlights(geo::LatLon home, uint16_t rangeNm) {
  static const JsonDocument filter = parsers::aircraftFilter();
  int radius = static_cast<int>(rangeNm * 1.2f) + 5;
  if (radius > kMaxQueryRadiusNm) radius = kMaxQueryRadiusNm;

  char url[128];
  const char* source = "adsb.lol";
  JsonDocument doc(net::psramAllocator());
  snprintf(url, sizeof(url), kPrimaryUrl, home.lat, home.lon, radius);
  net::Result result = net::getJson(url, doc, &filter);
  if (result != net::Result::Ok) {
    source = "adsb.fi";
    doc.clear();
    snprintf(url, sizeof(url), kFallbackUrl, home.lat, home.lon, radius);
    result = net::getJson(url, doc, &filter);
  }
  if (result != net::Result::Ok) {
    setState(FeedState::Error, net::describe(result));
    return;
  }

  parsers::AircraftResult parsed = parsers::parseAircraft(doc, home, rangeNm * 1.15, kMaxFlights);
  if (!parsed.ok) {
    setState(FeedState::Error, "unexpected reply");
    return;
  }

  std::lock_guard<std::mutex> guard(dataLock);
  snapshot.sequence++;
  snapshot.receivedMs = millis();
  snapshot.flights = std::move(parsed.flights);
  currentStatus.state = FeedState::Live;
  currentStatus.lastError = "";
  currentStatus.source = source;
  currentStatus.lastUpdateMs = snapshot.receivedMs;
  currentStatus.totalInRange = parsed.totalInRange;
  queuePrefetch(snapshot.flights);
}

// ---- Details (route, aircraft, photo) -----------------------------------------

bool looksLikeFlightNumber(const char* callsign) {
  // Airline flights look like "AAL1699": 3 letters then digits.
  return strlen(callsign) >= 4 && isalpha(callsign[0]) && isalpha(callsign[1]) &&
         isalpha(callsign[2]) && isdigit(callsign[3]);
}

void fetchRoute(const Request& r) {
  RouteInfo route{};
  if (looksLikeFlightNumber(r.callsign)) {
    char url[96];
    snprintf(url, sizeof(url), kRouteUrl, r.callsign);
    JsonDocument doc(net::psramAllocator());
    if (net::getJson(url, doc) == net::Result::Ok) route = parsers::parseRoute(doc);
  }
  std::lock_guard<std::mutex> guard(dataLock);
  CacheEntry& e = entryFor(r.hex);
  e.details.route = route;
  e.details.routeDone = true;
  e.usedMs = millis();
}

void fetchAircraft(const Request& r) {
  AircraftInfo info{};
  if (r.hex[0] != '~') {  // '~' means a non-ICAO (ground radar) target: no database entry
    char url[96];
    snprintf(url, sizeof(url), kAircraftUrl, r.hex);
    JsonDocument doc(net::psramAllocator());
    if (net::getJson(url, doc) == net::Result::Ok) info = parsers::parseAircraftInfo(doc);
  }
  std::lock_guard<std::mutex> guard(dataLock);
  CacheEntry& e = entryFor(r.hex);
  e.details.aircraft = info;
  e.details.aircraftDone = true;
}

void fetchPhoto(const Request& r, const char* url) {
  uint8_t* data = nullptr;
  size_t length = 0;
  net::getBytes(url, &data, &length, kMaxPhotoBytes);  // data stays null on failure
  std::lock_guard<std::mutex> guard(photoLock);
  if (photo != nullptr) heap_caps_free(photo);
  photo = data;
  photoLength = length;
  copyText(photoHex, r.hex);
}

// Does one piece of detail work for the focused plane. Returns true if it did.
bool workOnFocus() {
  Request r{};
  Details d{};
  {
    std::lock_guard<std::mutex> guard(dataLock);
    if (!haveFocus) return false;
    r = focus;
    const CacheEntry* e = findEntry(r.hex);
    if (e != nullptr) d = e->details;
  }
  if (!d.routeDone) {
    fetchRoute(r);
    return true;
  }
  if (!d.aircraftDone) {
    fetchAircraft(r);
    return true;
  }

  bool photoDone = false;
  {
    std::lock_guard<std::mutex> guard(photoLock);
    photoDone = strcmp(photoHex, r.hex) == 0;
  }
  if (!photoDone && d.aircraft.valid && d.aircraft.photoUrl[0] != '\0') {
    fetchPhoto(r, d.aircraft.photoUrl);
    return true;
  }
  if (!photoDone) {
    std::lock_guard<std::mutex> guard(photoLock);
    copyText(photoHex, r.hex);  // no photo exists; remember we checked
    if (photo != nullptr) heap_caps_free(photo);
    photo = nullptr;
    photoLength = 0;
  }
  return false;
}

bool workOnPrefetch() {
  Request r{};
  {
    std::lock_guard<std::mutex> guard(dataLock);
    if (prefetch.empty()) return false;
    r = prefetch.front();
    prefetch.erase(prefetch.begin());
    const CacheEntry* e = findEntry(r.hex);
    if (e != nullptr && e->details.routeDone) return true;
  }
  fetchRoute(r);
  return true;
}

// ---- The background task --------------------------------------------------------

void feedTask(void*) {
  uint32_t nextFetchMs = 0, nextLocateMs = 0;
  bool located = false;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      setState(FeedState::WaitingForWifi);
      delay(500);
      continue;
    }

    AppSettings s;
    bool changed;
    {
      std::lock_guard<std::mutex> guard(dataLock);
      s = settings;
      changed = settingsChanged;
      settingsChanged = false;
    }
    if (changed) {
      located = false;
      nextLocateMs = 0;
      nextFetchMs = 0;
    }

    if (!located) {
      if (millis() >= nextLocateMs) {
        located = locate(s);
        nextLocateMs = millis() + kRetryLocateMs;
        if (located) setState(FeedState::Loading);
      }
      delay(100);
      continue;
    }

    if (millis() >= nextFetchMs) {
      geo::LatLon home;
      {
        std::lock_guard<std::mutex> guard(dataLock);
        home = currentStatus.home;
      }
      fetchFlights(home, s.rangeNm);
      nextFetchMs = millis() + kRefreshMs;
      continue;
    }

    if (workOnFocus()) continue;
    if (workOnPrefetch()) {
      delay(kPrefetchGapMs);
      continue;
    }
    delay(50);
  }
}

}  // namespace

void begin(const AppSettings& s) {
  {
    std::lock_guard<std::mutex> guard(dataLock);
    settings = s;
    currentStatus.state = FeedState::WaitingForWifi;
    currentStatus.source = "";
    currentStatus.lastError = "";
    cache.reserve(kCacheSize);
  }
  // 12 KB of stack: HTTPS needs room. Priority 1, pinned to core 0.
  xTaskCreatePinnedToCore(feedTask, "flight_feed", 12288, nullptr, 1, nullptr, 0);
}

void updateSettings(const AppSettings& s) {
  std::lock_guard<std::mutex> guard(dataLock);
  const bool locationChanged = s.autoLocation != settings.autoLocation ||
                               s.home.lat != settings.home.lat || s.home.lon != settings.home.lon;
  const bool rangeChanged = s.rangeNm != settings.rangeNm;
  settings = s;
  if (locationChanged) {
    settingsChanged = true;
    currentStatus.haveLocation = false;
  } else if (rangeChanged) {
    settingsChanged = true;
  }
}

bool latest(uint32_t lastSequence, Snapshot* out) {
  std::lock_guard<std::mutex> guard(dataLock);
  if (snapshot.sequence == lastSequence) return false;
  *out = snapshot;
  return true;
}

FeedStatus status() {
  std::lock_guard<std::mutex> guard(dataLock);
  return currentStatus;
}

void wantDetails(const char* hex, const char* callsign, bool wantFocus) {
  std::lock_guard<std::mutex> guard(dataLock);
  if (wantFocus) {
    copyText(focus.hex, hex);
    copyText(focus.callsign, callsign);
    haveFocus = true;
  }
  if (CacheEntry* e = findEntry(hex)) e->usedMs = millis();
}

bool details(const char* hex, Details* out) {
  std::lock_guard<std::mutex> guard(dataLock);
  const CacheEntry* e = findEntry(hex);
  if (e == nullptr) return false;
  *out = e->details;
  return true;
}

bool lockPhoto(const char* hex, const uint8_t** data, size_t* length) {
  photoLock.lock();
  if (photo == nullptr || strcmp(photoHex, hex) != 0) {
    photoLock.unlock();
    return false;
  }
  *data = photo;
  *length = photoLength;
  return true;
}

void unlockPhoto() { photoLock.unlock(); }

}  // namespace feed
