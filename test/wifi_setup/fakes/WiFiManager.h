#pragma once
#include <functional>
#include <map>
#include <string>
#include "WiFi.h"

class WiFiManagerParameter {
  std::string id_, value_;
 public:
  WiFiManagerParameter(const char* id, const char*, const char* value, int)
      : id_(id), value_(value) {}
  void setValue(const char* value, int length) { value_ = std::string(value).substr(0, length); }
  const char* getValue() const { return value_.c_str(); }
  const std::string& id() const { return id_; }
};
class WiFiManager {
 public:
  inline static bool saved = false;
  inline static bool portal = false;
  inline static std::map<std::string, WiFiManagerParameter*> params;
  inline static std::function<void()> saveParams;
  void setTitle(const char*) {}
  void setClass(const char*) {}
  void addParameter(WiFiManagerParameter* p) { params[p->id()] = p; }
  void setSaveParamsCallback(std::function<void()> cb) { saveParams = cb; }
  void setSaveConfigCallback(std::function<void()>) {}
  void setConfigPortalBlocking(bool) {}
  void setConnectTimeout(int) {}
  void setShowInfoUpdate(bool) {}
  bool getWiFiIsSaved() { return saved; }
  void autoConnect(const char*) { portal = true; }
  void startConfigPortal(const char*) { portal = true; }
  bool getConfigPortalActive() { return portal; }
  void process() {}
};
