#pragma once

#include "Arduino.h"

#include <cstdint>

enum arduino_event_id_t { ARDUINO_EVENT_WIFI_STA_DISCONNECTED };
enum wl_status_t { WL_DISCONNECTED, WL_CONNECTED };
enum wifi_mode_t { WIFI_STA };

namespace testWiFi {
inline wl_status_t connection = WL_DISCONNECTED;
inline void (*eventCallback)(arduino_event_id_t) = nullptr;
inline unsigned int beginCalls = 0;
inline int channel = 0;
inline bool autoReconnect = true;

inline void reset() {
  connection = WL_DISCONNECTED;
  eventCallback = nullptr;
  beginCalls = 0;
  channel = 0;
  autoReconnect = true;
}

inline void disconnectEvent() {
  if (eventCallback) eventCallback(ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}
}

class TestWiFi {
 public:
  void onEvent(void (*callback)(arduino_event_id_t)) { testWiFi::eventCallback = callback; }
  void mode(wifi_mode_t) {}
  bool setAutoReconnect(bool enabled) {
    testWiFi::autoReconnect = enabled;
    return true;
  }
  void begin(const char*, const char*, int channel) {
    ++testWiFi::beginCalls;
    testWiFi::channel = channel;
  }
  wl_status_t status() const { return testWiFi::connection; }
  const char* localIP() const { return "192.0.2.1"; }
};

extern TestWiFi WiFi;
