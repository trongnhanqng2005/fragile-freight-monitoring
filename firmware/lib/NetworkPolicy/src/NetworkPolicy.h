#pragma once

#include <cstdint>

namespace fragilefreight {

class RetryGate {
 public:
  explicit RetryGate(uint32_t intervalMs) : intervalMs_(intervalMs) {}

  bool due(uint32_t nowMs) const;
  void markAttempt(uint32_t nowMs);

 private:
  uint32_t intervalMs_;
  uint32_t lastAttemptMs_ = 0;
  bool hasAttempted_ = false;
};

bool canAttemptWiFi(bool wifiConnected, bool attemptPending, const RetryGate& retry,
                    uint32_t nowMs);
bool canAttemptMqtt(bool wifiConnected, bool mqttDisconnected, const RetryGate& retry,
                    uint32_t nowMs);

}  // namespace fragilefreight
