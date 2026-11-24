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
  bool alarmEnabled() const { return alarmEnabled_.load(std::memory_order_relaxed); }

 private:
  static void onWiFiEvent(arduino_event_id_t event);
  void startWiFiAttempt(uint32_t nowMs);
  void onMqttMessage(const espMqttClientTypes::MessageProperties& properties,
                     const char* topic, const uint8_t* payload, size_t len,
                     size_t index, size_t total);

  espMqttClient mqttClient_;
  fragilefreight::RetryGate wifiRetry_{5000};
  fragilefreight::RetryGate mqttRetry_{5000};
  std::atomic<uint8_t> mqttEvents_{0};
  std::atomic<uint8_t> mqttDisconnectReason_{0};
  std::atomic<uint8_t> commandEvents_{0};
  std::atomic<bool> alarmEnabled_{true};
  bool wifiAttemptPending_ = false;
  bool wifiConnected_ = false;

  static std::atomic<bool> wifiDisconnectedEvent_;
};
