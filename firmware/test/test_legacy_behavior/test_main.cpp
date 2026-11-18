#include <unity.h>
#include <cstring>
#include <limits>

#include "../../src/NetworkAdapter.cpp"
#define setup firmwareSetup
#define loop firmwareLoop
#include "../../src/main.cpp"
#undef setup
#undef loop

using fragilefreight::classifyImpact;

TestSerial Serial;
TwoWire Wire;
TestWiFi WiFi;

uint32_t millis() { return testHardware::now; }
void pinMode(int, int) {}
void digitalWrite(int pin, int value) { testHardware::leds[pin] = value; }
void tone(int, unsigned int frequency) {
  testHardware::buzzerOn = true;
  testHardware::buzzerFrequency = frequency;
  ++testHardware::toneCalls;
}
void noTone(int) {
  testHardware::buzzerOn = false;
  ++testHardware::noToneCalls;
}
void delay(unsigned long duration) { testHardware::now += duration; }

extern "C" void setUp(void) {
  testHardware::reset();
  testWiFi::reset();
  testMqtt::lastClient->reset();
  impactLogic.reset();
  lastSampleMs = 0;
  lastSerialMs = 0;
  buzzerStartedMs = 0;
  telemetryTiming.reset();
  latestSnapshot = ImpactSnapshot{};
  hasValidSnapshot = false;
  buzzerActive = false;
  firmwareSetup();
}

extern "C" void tearDown(void) {}

void sampleAt(uint32_t time, float ax, float ay, float az) {
  testHardware::now = time;
  testHardware::ax = ax;
  testHardware::ay = ay;
  testHardware::az = az;
  firmwareLoop();
}

espMqttClient& mqtt() { return *testMqtt::lastClient; }

void test_existing_threshold_boundaries() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::SAFE),
                        static_cast<int>(classifyImpact(nextafterf(0.4f, 0.0f))));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::WARNING),
                        static_cast<int>(classifyImpact(0.4f)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::WARNING),
                        static_cast<int>(classifyImpact(nextafterf(0.9f, 0.0f))));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::DANGER),
                        static_cast<int>(classifyImpact(0.9f)));
}

void test_existing_state_names() {
  TEST_ASSERT_EQUAL_STRING("SAFE", stateName(ImpactState::SAFE));
  TEST_ASSERT_EQUAL_STRING("WARNING", stateName(ImpactState::WARNING));
  TEST_ASSERT_EQUAL_STRING("DANGER", stateName(ImpactState::DANGER));
}

void test_existing_magnitude_uses_all_three_axes() {
  sampleAt(250, 3.0f, 4.0f, 0.0f);
  const std::string& output = testHardware::serialOutput;
  const size_t magnitude = output.find("Magnitude=");
  TEST_ASSERT_NOT_EQUAL(std::string::npos, magnitude);
  TEST_ASSERT_EQUAL_STRING("Magnitude=5.00 g | Deviation=4.00 g | State=DANGER\n",
                           output.substr(magnitude).c_str());
}

void test_existing_leds_follow_classification() {
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_INT(HIGH, testHardware::leds[GREEN_LED_PIN]);
  TEST_ASSERT_EQUAL_INT(LOW, testHardware::leds[YELLOW_LED_PIN]);

  sampleAt(20, 0.0f, 0.0f, 1.5f);
  TEST_ASSERT_EQUAL_INT(LOW, testHardware::leds[GREEN_LED_PIN]);
  TEST_ASSERT_EQUAL_INT(HIGH, testHardware::leds[YELLOW_LED_PIN]);
  TEST_ASSERT_EQUAL_INT(LOW, testHardware::leds[RED_LED_PIN]);
}

void test_existing_danger_starts_one_150ms_buzzer() {
  sampleAt(10, 0.0f, 0.0f, 2.0f);
  sampleAt(20, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);
  TEST_ASSERT_EQUAL_UINT(2000, testHardware::buzzerFrequency);
  TEST_ASSERT_TRUE(testHardware::buzzerOn);

  sampleAt(159, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_TRUE(testHardware::buzzerOn);
  sampleAt(160, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_FALSE(testHardware::buzzerOn);
  TEST_ASSERT_EQUAL_UINT(2, testHardware::noToneCalls);  // setup plus buzzer stop
}

void test_existing_warning_does_not_rearm_alert() {
  sampleAt(10, 0.0f, 0.0f, 2.0f);
  sampleAt(1010, 0.0f, 0.0f, 1.5f);
  sampleAt(1020, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);
}

void test_existing_safe_rearms_at_cooldown_from_previous_danger() {
  sampleAt(10, 0.0f, 0.0f, 2.0f);
  sampleAt(1010, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);
  sampleAt(1020, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(2, testHardware::toneCalls);
}

void test_existing_safe_before_cooldown_does_not_rearm_until_next_sample() {
  sampleAt(10, 0.0f, 0.0f, 2.0f);
  sampleAt(1009, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);
  sampleAt(1019, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);
}

void test_existing_wifi_retries_spaced_from_attempt_start() {
  TEST_ASSERT_EQUAL_UINT(1, testWiFi::beginCalls);
  TEST_ASSERT_EQUAL_INT(6, testWiFi::channel);
  TEST_ASSERT_FALSE(testWiFi::autoReconnect);

  sampleAt(4999, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, testWiFi::beginCalls);
  testWiFi::disconnectEvent();
  sampleAt(4999, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, testWiFi::beginCalls);
  sampleAt(5000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(2, testWiFi::beginCalls);
}

void test_existing_mqtt_requires_wifi_and_does_not_overlap_attempts() {
  TEST_ASSERT_EQUAL_STRING("host.wokwi.internal", mqtt().server.c_str());
  TEST_ASSERT_EQUAL_UINT(1883, mqtt().serverPort);
  TEST_ASSERT_EQUAL_STRING("esp32-01", mqtt().clientId.c_str());
  TEST_ASSERT_TRUE(mqtt().cleanSession);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().connectCalls);

  testWiFi::connection = WL_CONNECTED;
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().connectCalls);
  sampleAt(5010, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().connectCalls);

  mqtt().markDisconnected();
  sampleAt(5009, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().connectCalls);
  sampleAt(5010, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(2, mqtt().connectCalls);
}

void test_telemetry_publishes_exact_latest_safe_snapshot_every_second() {
  mqtt().markConnected();
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  sampleAt(999, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().telemetryPublishCalls);

  sampleAt(1000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().telemetryPublishCalls);
  TEST_ASSERT_EQUAL_STRING("fragile-freight/esp32-01/telemetry", mqtt().lastTopic.c_str());
  TEST_ASSERT_EQUAL_STRING(
      "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}",
      mqtt().lastPayload.c_str());

  sampleAt(1999, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().telemetryPublishCalls);
  sampleAt(2000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(2, mqtt().telemetryPublishCalls);
}

void test_danger_event_publishes_one_immediate_exact_alert() {
  mqtt().markConnected();
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  sampleAt(20, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().alertPublishCalls);
  TEST_ASSERT_EQUAL_STRING("fragile-freight/esp32-01/alert", mqtt().lastTopic.c_str());
  TEST_ASSERT_EQUAL_STRING(
      "{\"deviceId\":\"esp32-01\",\"status\":\"DANGER\",\"gForce\":2}",
      mqtt().lastPayload.c_str());
  TEST_ASSERT_EQUAL_UINT(0, mqtt().lastQos);
  TEST_ASSERT_FALSE(mqtt().lastRetain);
  sampleAt(30, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().alertPublishCalls);
}

void test_offline_danger_is_dropped_and_reconnect_does_not_replay() {
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  sampleAt(20, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().alertPublishCalls);
  TEST_ASSERT_EQUAL_UINT(1, testHardware::toneCalls);

  mqtt().markConnected();
  sampleAt(30, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().alertPublishCalls);
  sampleAt(1020, 0.0f, 0.0f, 1.0f);
  sampleAt(1030, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().alertPublishCalls);
}

void test_no_telemetry_is_published_without_a_valid_snapshot() {
  mqtt().markConnected();
  sampleAt(10, std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f);
  sampleAt(1000, std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().telemetryPublishCalls);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().publishAttempts);
}

void test_telemetry_publishes_latest_valid_snapshot_after_invalid_sample() {
  mqtt().markConnected();
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().telemetryPublishCalls);

  sampleAt(1000, std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().telemetryPublishCalls);
  TEST_ASSERT_EQUAL_STRING(
      "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}",
      mqtt().lastPayload.c_str());

  sampleAt(1999, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().telemetryPublishCalls);
  sampleAt(2000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(2, mqtt().telemetryPublishCalls);
}

void test_telemetry_skips_offline_intervals_without_reconnect_burst() {
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  sampleAt(1000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().telemetryPublishCalls);

  mqtt().markConnected();
  sampleAt(1001, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().telemetryPublishCalls);
  sampleAt(2000, 0.0f, 0.0f, 1.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().telemetryPublishCalls);
}

void test_failed_alert_publish_is_discarded_without_retry_or_replay() {
  mqtt().markConnected();
  mqtt().rejectPublishes = true;
  sampleAt(10, 0.0f, 0.0f, 1.0f);
  sampleAt(20, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().publishAttempts);
  TEST_ASSERT_EQUAL_UINT(0, mqtt().alertPublishCalls);

  sampleAt(30, 0.0f, 0.0f, 2.0f);
  mqtt().markDisconnected();
  sampleAt(40, 0.0f, 0.0f, 2.0f);
  mqtt().markConnected();
  sampleAt(50, 0.0f, 0.0f, 2.0f);
  TEST_ASSERT_EQUAL_UINT(1, mqtt().publishAttempts);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_existing_threshold_boundaries);
  RUN_TEST(test_existing_state_names);
  RUN_TEST(test_existing_magnitude_uses_all_three_axes);
  RUN_TEST(test_existing_leds_follow_classification);
  RUN_TEST(test_existing_danger_starts_one_150ms_buzzer);
  RUN_TEST(test_existing_warning_does_not_rearm_alert);
  RUN_TEST(test_existing_safe_rearms_at_cooldown_from_previous_danger);
  RUN_TEST(test_existing_safe_before_cooldown_does_not_rearm_until_next_sample);
  RUN_TEST(test_existing_wifi_retries_spaced_from_attempt_start);
  RUN_TEST(test_existing_mqtt_requires_wifi_and_does_not_overlap_attempts);
  RUN_TEST(test_telemetry_publishes_exact_latest_safe_snapshot_every_second);
  RUN_TEST(test_danger_event_publishes_one_immediate_exact_alert);
  RUN_TEST(test_offline_danger_is_dropped_and_reconnect_does_not_replay);
  RUN_TEST(test_no_telemetry_is_published_without_a_valid_snapshot);
  RUN_TEST(test_telemetry_publishes_latest_valid_snapshot_after_invalid_sample);
  RUN_TEST(test_telemetry_skips_offline_intervals_without_reconnect_burst);
  RUN_TEST(test_failed_alert_publish_is_discarded_without_retry_or_replay);
  return UNITY_END();
}
