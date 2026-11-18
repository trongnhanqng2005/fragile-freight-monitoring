#include <unity.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#include <ImpactLogic.h>
#include <MqttPayload.h>

using fragilefreight::ImpactLogic;
using fragilefreight::ImpactSnapshot;
using fragilefreight::formatAlert;
using fragilefreight::formatTelemetry;

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

ImpactSnapshot sampleFor(float azG) {
  ImpactLogic logic;
  return logic.update(0.0f, 0.0f, azG, 10);
}

void test_topics_and_device_identity_match_contract() {
  TEST_ASSERT_EQUAL_STRING("esp32-01", fragilefreight::DEVICE_ID);
  TEST_ASSERT_EQUAL_STRING("fragile-freight/esp32-01/telemetry",
                           fragilefreight::TELEMETRY_TOPIC);
  TEST_ASSERT_EQUAL_STRING("fragile-freight/esp32-01/alert", fragilefreight::ALERT_TOPIC);
}

void test_safe_telemetry_has_exact_contract_fields_and_g_units() {
  const ImpactSnapshot sample = sampleFor(1.0f);
  char output[192];
  size_t length = 0;
  TEST_ASSERT_TRUE(formatTelemetry(sample, output, sizeof(output), length));
  TEST_ASSERT_EQUAL_STRING(
      "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}",
      output);
  TEST_ASSERT_EQUAL_UINT(std::strlen(output), length);
  TEST_ASSERT_NULL(std::strstr(output, "timestamp"));
}

void test_warning_and_danger_are_uppercase_states() {
  char output[192];
  size_t length = 0;
  ImpactSnapshot warning = sampleFor(1.5f);
  TEST_ASSERT_TRUE(formatTelemetry(warning, output, sizeof(output), length));
  TEST_ASSERT_NOT_NULL(std::strstr(output, "\"az\":1.5"));
  TEST_ASSERT_NOT_NULL(std::strstr(output, "\"status\":\"WARNING\""));

  ImpactSnapshot danger = sampleFor(2.0f);
  TEST_ASSERT_TRUE(formatTelemetry(danger, output, sizeof(output), length));
  TEST_ASSERT_NOT_NULL(std::strstr(output, "\"gForce\":2"));
  TEST_ASSERT_NOT_NULL(std::strstr(output, "\"status\":\"DANGER\""));
}

void test_alert_has_only_three_fields_and_danger_status() {
  const ImpactSnapshot danger = sampleFor(2.0f);
  char output[128];
  size_t length = 0;
  TEST_ASSERT_TRUE(formatAlert(danger, output, sizeof(output), length));
  TEST_ASSERT_EQUAL_STRING(
      "{\"deviceId\":\"esp32-01\",\"status\":\"DANGER\",\"gForce\":2}", output);
  TEST_ASSERT_EQUAL_UINT(std::strlen(output), length);
}

void test_alert_rejects_non_danger_snapshot() {
  const ImpactSnapshot safe = sampleFor(1.0f);
  char output[128];
  size_t length = 0;
  TEST_ASSERT_FALSE(formatAlert(safe, output, sizeof(output), length));
}

void test_payloads_reject_non_finite_values() {
  ImpactSnapshot invalid = sampleFor(1.0f);
  invalid.axG = std::numeric_limits<float>::quiet_NaN();
  char output[192];
  size_t length = 42;
  TEST_ASSERT_FALSE(formatTelemetry(invalid, output, sizeof(output), length));
  TEST_ASSERT_EQUAL_UINT(0, length);

  invalid = sampleFor(2.0f);
  invalid.magnitudeG = std::numeric_limits<float>::infinity();
  TEST_ASSERT_FALSE(formatAlert(invalid, output, sizeof(output), length));
}

void test_payload_formatting_is_bounded_and_clears_truncated_output() {
  const ImpactSnapshot sample = sampleFor(1.0f);
  char full[192];
  size_t length = 0;
  TEST_ASSERT_TRUE(formatTelemetry(sample, full, sizeof(full), length));

  char exact[192];
  TEST_ASSERT_TRUE(formatTelemetry(sample, exact, length + 1, length));
  TEST_ASSERT_EQUAL_STRING(full, exact);

  char shortBuffer[192];
  shortBuffer[0] = 'x';
  TEST_ASSERT_FALSE(formatTelemetry(sample, shortBuffer, length, length));
  TEST_ASSERT_EQUAL_UINT(0, shortBuffer[0]);
}

void test_payload_formatting_rejects_null_or_zero_capacity() {
  const ImpactSnapshot sample = sampleFor(1.0f);
  size_t length = 1;
  TEST_ASSERT_FALSE(formatTelemetry(sample, nullptr, 0, length));
  TEST_ASSERT_EQUAL_UINT(0, length);
  char output[1] = {'x'};
  TEST_ASSERT_FALSE(formatTelemetry(sample, output, 0, length));
  TEST_ASSERT_EQUAL_CHAR('x', output[0]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_topics_and_device_identity_match_contract);
  RUN_TEST(test_safe_telemetry_has_exact_contract_fields_and_g_units);
  RUN_TEST(test_warning_and_danger_are_uppercase_states);
  RUN_TEST(test_alert_has_only_three_fields_and_danger_status);
  RUN_TEST(test_alert_rejects_non_danger_snapshot);
  RUN_TEST(test_payloads_reject_non_finite_values);
  RUN_TEST(test_payload_formatting_is_bounded_and_clears_truncated_output);
  RUN_TEST(test_payload_formatting_rejects_null_or_zero_capacity);
  return UNITY_END();
}
