#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace espMqttClientTypes {
enum class DisconnectReason : uint8_t { TCP_DISCONNECTED };
inline const char* disconnectReasonToString(DisconnectReason) { return "TCP_DISCONNECTED"; }
}

class espMqttClient;
namespace testMqtt {
inline espMqttClient* lastClient = nullptr;
}

class espMqttClient {
 public:
  espMqttClient() { testMqtt::lastClient = this; }
  void setServer(const char* host, uint16_t port) { server = host; serverPort = port; }
  void setClientId(const char* id) { clientId = id; }
  void setCleanSession(bool enabled) { cleanSession = enabled; }
  void onConnect(std::function<void(bool)> callback) { connectCallback = callback; }
  void onDisconnect(std::function<void(espMqttClientTypes::DisconnectReason)> callback) {
    disconnectCallback = callback;
  }
  bool connect() {
    if (!isDisconnected) return false;
    ++connectCalls;
    isDisconnected = false;
    isConnected = false;
    return true;
  }
  bool disconnected() const { return isDisconnected; }
  bool connected() const { return isConnected; }
  uint16_t publish(const char* topic, uint8_t qos, bool retain, const uint8_t* payload,
                   size_t length) {
    ++publishAttempts;
    if (!isConnected || rejectPublishes) return 0;
    lastTopic = topic;
    lastPayload.assign(reinterpret_cast<const char*>(payload), length);
    lastQos = qos;
    lastRetain = retain;
    ++publishCalls;
    if (lastTopic == "fragile-freight/esp32-01/alert") ++alertPublishCalls;
    if (lastTopic == "fragile-freight/esp32-01/telemetry") ++telemetryPublishCalls;
    return 1;
  }
  void markConnected() {
    isDisconnected = false;
    isConnected = true;
    if (connectCallback) connectCallback(false);
  }
  void markDisconnected() {
    isDisconnected = true;
    isConnected = false;
    if (disconnectCallback) {
      disconnectCallback(espMqttClientTypes::DisconnectReason::TCP_DISCONNECTED);
    }
  }
  void reset() {
    server.clear();
    serverPort = 0;
    clientId.clear();
    cleanSession = false;
    connectCallback = nullptr;
    disconnectCallback = nullptr;
    isDisconnected = true;
    isConnected = false;
    connectCalls = 0;
    publishAttempts = 0;
    publishCalls = 0;
    alertPublishCalls = 0;
    telemetryPublishCalls = 0;
    lastTopic.clear();
    lastPayload.clear();
    lastQos = 255;
    lastRetain = true;
    rejectPublishes = false;
  }

  std::string server;
  uint16_t serverPort = 0;
  std::string clientId;
  bool cleanSession = false;
  unsigned int connectCalls = 0;
  unsigned int publishAttempts = 0;
  unsigned int publishCalls = 0;
  unsigned int alertPublishCalls = 0;
  unsigned int telemetryPublishCalls = 0;
  std::string lastTopic;
  std::string lastPayload;
  uint8_t lastQos = 255;
  bool lastRetain = true;
  bool rejectPublishes = false;

 private:
  bool isDisconnected = true;
  bool isConnected = false;
  std::function<void(bool)> connectCallback;
  std::function<void(espMqttClientTypes::DisconnectReason)> disconnectCallback;
};
