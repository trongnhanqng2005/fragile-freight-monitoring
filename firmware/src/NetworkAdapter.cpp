#include "NetworkAdapter.h"

#include <WiFi.h>
#include <cstring>

#include <MqttPayload.h>

namespace {
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr char MQTT_HOST[] = "host.wokwi.internal";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_CLIENT_ID[] = "esp32-01";
constexpr uint32_t NETWORK_RETRY_INTERVAL_MS = 5000;
constexpr uint8_t MQTT_CONNECTED_EVENT = 0x01;
constexpr uint8_t MQTT_DISCONNECTED_EVENT = 0x02;
constexpr size_t MQTT_PAYLOAD_CAPACITY = 192;
}

std::atomic<bool> NetworkAdapter::wifiDisconnectedEvent_{false};

void NetworkAdapter::onWiFiEvent(arduino_event_id_t event) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    wifiDisconnectedEvent_.store(true, std::memory_order_relaxed);
  }
}

void NetworkAdapter::startWiFiAttempt(uint32_t nowMs) {
  wifiRetry_.markAttempt(nowMs);
  wifiAttemptPending_ = true;
  Serial.println("Wi-Fi: starting connection attempt");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
}

void NetworkAdapter::begin() {
  wifiRetry_ = fragilefreight::RetryGate(NETWORK_RETRY_INTERVAL_MS);
  mqttRetry_ = fragilefreight::RetryGate(NETWORK_RETRY_INTERVAL_MS);
  wifiDisconnectedEvent_.store(false, std::memory_order_relaxed);
  mqttEvents_.store(0, std::memory_order_relaxed);
  mqttDisconnectReason_.store(0, std::memory_order_relaxed);
  wifiAttemptPending_ = false;
  wifiConnected_ = false;

  mqttClient_.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient_.setClientId(MQTT_CLIENT_ID);
  mqttClient_.setCleanSession(true);
  mqttClient_.onConnect([this](bool sessionPresent) {
    (void)sessionPresent;
    mqttEvents_.fetch_or(MQTT_CONNECTED_EVENT, std::memory_order_relaxed);
    mqttClient_.subscribe(fragilefreight::COMMAND_TOPIC, 0);
  });
  mqttClient_.onDisconnect([this](espMqttClientTypes::DisconnectReason reason) {
    mqttDisconnectReason_.store(static_cast<uint8_t>(reason), std::memory_order_relaxed);
    mqttEvents_.fetch_or(MQTT_DISCONNECTED_EVENT, std::memory_order_relaxed);
  });
  mqttClient_.onMessage([this](const espMqttClientTypes::MessageProperties& properties,
                               const char* topic, const uint8_t* payload, size_t len,
                               size_t index, size_t total) {
    onMqttMessage(properties, topic, payload, len, index, total);
  });

  WiFi.onEvent(onWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  startWiFiAttempt(millis());
}

void NetworkAdapter::onMqttMessage(
    const espMqttClientTypes::MessageProperties& properties, const char* topic,
    const uint8_t* payload, size_t len, size_t index, size_t total) {
  (void)properties;
  if (!topic || !payload || strcmp(topic, fragilefreight::COMMAND_TOPIC) != 0 ||
      index != 0 || len != total) return;
  if (len == 8 && memcmp(payload, "ALARM_ON", 8) == 0) {
    alarmEnabled_.store(true, std::memory_order_relaxed);
    commandEvents_.store(1, std::memory_order_relaxed);
  } else if (len == 9 && memcmp(payload, "ALARM_OFF", 9) == 0) {
    alarmEnabled_.store(false, std::memory_order_relaxed);
    commandEvents_.store(2, std::memory_order_relaxed);
  } else {
    commandEvents_.store(3, std::memory_order_relaxed);
  }
}

void NetworkAdapter::update(uint32_t nowMs) {
  switch (commandEvents_.exchange(0, std::memory_order_relaxed)) {
    case 1: Serial.println("MQTT command: local buzzer enabled"); break;
    case 2: Serial.println("MQTT command: local buzzer disabled"); break;
    case 3: Serial.println("MQTT command ignored: unsupported payload"); break;
    default: break;
  }

  if (wifiDisconnectedEvent_.exchange(false, std::memory_order_relaxed)) {
    wifiAttemptPending_ = false;
    Serial.println("Wi-Fi: station disconnected");
    wifiConnected_ = false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiAttemptPending_ = false;
    if (!wifiConnected_) {
      wifiConnected_ = true;
      Serial.print("Wi-Fi: connected, IP=");
      Serial.println(WiFi.localIP());
    }
  } else {
    wifiConnected_ = false;
    if (fragilefreight::canAttemptWiFi(false, wifiAttemptPending_, wifiRetry_, nowMs)) {
      startWiFiAttempt(nowMs);
    }
  }

  const uint8_t events = mqttEvents_.exchange(0, std::memory_order_relaxed);
  if (events & MQTT_CONNECTED_EVENT) {
    Serial.println("MQTT: connected (CONNACK received)");
  }
  if (events & MQTT_DISCONNECTED_EVENT) {
    Serial.print("MQTT: disconnected (");
    Serial.print(espMqttClientTypes::disconnectReasonToString(
        static_cast<espMqttClientTypes::DisconnectReason>(
            mqttDisconnectReason_.load(std::memory_order_relaxed))));
    Serial.println(")");
  }

  if (!fragilefreight::canAttemptMqtt(wifiConnected_, mqttClient_.disconnected(),
                                      mqttRetry_, nowMs)) {
    return;
  }

  mqttRetry_.markAttempt(nowMs);
  Serial.println("MQTT: starting connection attempt");
  if (!mqttClient_.connect()) {
    Serial.println("MQTT: connection request rejected");
  }
}

bool NetworkAdapter::connected() const {
  return mqttClient_.connected();
}

bool NetworkAdapter::publishTelemetry(const fragilefreight::ImpactSnapshot& sample) {
  if (!connected()) return false;
  char payload[MQTT_PAYLOAD_CAPACITY];
  size_t length = 0;
  if (!fragilefreight::formatTelemetry(sample, payload, sizeof(payload), length)) {
    Serial.println("MQTT: invalid telemetry snapshot; publish skipped");
    return false;
  }

  const uint16_t packetId = mqttClient_.publish(
      fragilefreight::TELEMETRY_TOPIC, 0, false,
      reinterpret_cast<const uint8_t*>(payload), length);
  if (packetId == 0) Serial.println("MQTT: telemetry publish rejected");
  return packetId != 0;
}

bool NetworkAdapter::publishAlert(const fragilefreight::ImpactSnapshot& sample) {
  if (!connected()) return false;
  char payload[MQTT_PAYLOAD_CAPACITY];
  size_t length = 0;
  if (!fragilefreight::formatAlert(sample, payload, sizeof(payload), length)) {
    Serial.println("MQTT: invalid DANGER snapshot; alert discarded");
    return false;
  }

  const uint16_t packetId = mqttClient_.publish(
      fragilefreight::ALERT_TOPIC, 0, false,
      reinterpret_cast<const uint8_t*>(payload), length);
  Serial.println(packetId == 0 ? "MQTT: alert publish rejected; event discarded"
                              : "MQTT: alert accepted by client (QoS 0)");
  return packetId != 0;
}
