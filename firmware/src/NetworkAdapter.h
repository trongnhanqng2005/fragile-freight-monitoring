#pragma once

#include <atomic>
#include <cstdint>

#include <espMqttClient.h>
#include <WiFi.h>

#include <ImpactLogic.h>
#include <NetworkPolicy.h>

class NetworkAdapter {
 public:
  void begin();
  void update(uint32_t nowMs);
  bool connected() const;
  bool publishTelemetry(const fragilefreight::ImpactSnapshot& sample);
  bool publishAlert(const fragilefreight::ImpactSnapshot& sample);

 private:
  static void onWiFiEvent(arduino_event_id_t event);
  void startWiFiAttempt(uint32_t nowMs);

  espMqttClient mqttClient_;
  fragilefreight::RetryGate wifiRetry_{5000};
  fragilefreight::RetryGate mqttRetry_{5000};
  std::atomic<uint8_t> mqttEvents_{0};
  std::atomic<uint8_t> mqttDisconnectReason_{0};
  bool wifiAttemptPending_ = false;
  bool wifiConnected_ = false;

  static std::atomic<bool> wifiDisconnectedEvent_;
};
