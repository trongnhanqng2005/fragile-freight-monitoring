#pragma once

#include <cstdint>

namespace fragilefreight {

class TelemetryTiming {
 public:
  explicit TelemetryTiming(uint32_t intervalMs = 1000) : intervalMs_(intervalMs) {}

  bool due(uint32_t nowMs);
  void reset(uint32_t nowMs = 0) { lastPublishMs_ = nowMs; }

 private:
  uint32_t intervalMs_;
  uint32_t lastPublishMs_ = 0;
};

}  // namespace fragilefreight
