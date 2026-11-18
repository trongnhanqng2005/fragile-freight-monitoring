#include <unity.h>

#include <cstdint>
#include <limits>

#include <NetworkPolicy.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_retry_gate_allows_first_attempt_and_spaces_retries() {
  fragilefreight::RetryGate retry(5000);
  TEST_ASSERT_TRUE(retry.due(0));
  retry.markAttempt(1000);
  TEST_ASSERT_FALSE(retry.due(5999));
  TEST_ASSERT_TRUE(retry.due(6000));
}

void test_wifi_policy_rejects_connected_or_pending_attempts() {
  fragilefreight::RetryGate retry(5000);
  TEST_ASSERT_TRUE(fragilefreight::canAttemptWiFi(false, false, retry, 0));
  TEST_ASSERT_FALSE(fragilefreight::canAttemptWiFi(true, false, retry, 0));
  TEST_ASSERT_FALSE(fragilefreight::canAttemptWiFi(false, true, retry, 5000));
}

void test_mqtt_policy_requires_wifi_fully_disconnected_and_due() {
  fragilefreight::RetryGate retry(5000);
  TEST_ASSERT_FALSE(fragilefreight::canAttemptMqtt(false, true, retry, 0));
  TEST_ASSERT_FALSE(fragilefreight::canAttemptMqtt(true, false, retry, 0));
  TEST_ASSERT_TRUE(fragilefreight::canAttemptMqtt(true, true, retry, 0));
  retry.markAttempt(0);
  TEST_ASSERT_FALSE(fragilefreight::canAttemptMqtt(true, true, retry, 4999));
  TEST_ASSERT_TRUE(fragilefreight::canAttemptMqtt(true, true, retry, 5000));
}

void test_retry_gate_handles_millis_rollover() {
  fragilefreight::RetryGate retry(1000);
  retry.markAttempt(std::numeric_limits<uint32_t>::max() - 500);
  TEST_ASSERT_FALSE(retry.due(498));
  TEST_ASSERT_TRUE(retry.due(499));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_retry_gate_allows_first_attempt_and_spaces_retries);
  RUN_TEST(test_wifi_policy_rejects_connected_or_pending_attempts);
  RUN_TEST(test_mqtt_policy_requires_wifi_fully_disconnected_and_due);
  RUN_TEST(test_retry_gate_handles_millis_rollover);
  return UNITY_END();
}
