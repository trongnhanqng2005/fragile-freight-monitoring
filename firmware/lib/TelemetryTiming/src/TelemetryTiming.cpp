#include "TelemetryTiming.h"

namespace fragilefreight {

bool TelemetryTiming::due(uint32_t nowMs) {
  if (static_cast<uint32_t>(nowMs - lastPublishMs_) < intervalMs_) return false;
  lastPublishMs_ = nowMs;
  return true;
}

}  // namespace fragilefreight
