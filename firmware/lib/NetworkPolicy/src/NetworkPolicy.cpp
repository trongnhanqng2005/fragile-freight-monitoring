#include "NetworkPolicy.h"

namespace fragilefreight {

bool RetryGate::due(uint32_t nowMs) const {
  return !hasAttempted_ || static_cast<uint32_t>(nowMs - lastAttemptMs_) >= intervalMs_;
}

void RetryGate::markAttempt(uint32_t nowMs) {
  lastAttemptMs_ = nowMs;
  hasAttempted_ = true;
}

bool canAttemptWiFi(bool wifiConnected, bool attemptPending, const RetryGate& retry,
                    uint32_t nowMs) {
  return !wifiConnected && !attemptPending && retry.due(nowMs);
}

bool canAttemptMqtt(bool wifiConnected, bool mqttDisconnected, const RetryGate& retry,
                    uint32_t nowMs) {
  return wifiConnected && mqttDisconnected && retry.due(nowMs);
}

}  // namespace fragilefreight
