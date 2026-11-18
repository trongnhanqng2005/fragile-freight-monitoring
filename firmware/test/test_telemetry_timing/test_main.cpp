#include <unity.h>

#include <cstdint>
#include <limits>

#include <TelemetryTiming.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_first_telemetry_is_due_at_one_second() {
  fragilefreight::TelemetryTiming timing;
  TEST_ASSERT_FALSE(timing.due(999));
  TEST_ASSERT_TRUE(timing.due(1000));
}

void test_period_is_independent_and_does_not_catch_up() {
  fragilefreight::TelemetryTiming timing;
  TEST_ASSERT_TRUE(timing.due(1000));
  TEST_ASSERT_FALSE(timing.due(1999));
  TEST_ASSERT_TRUE(timing.due(2000));

  TEST_ASSERT_TRUE(timing.due(20000));
  TEST_ASSERT_FALSE(timing.due(20000));
  TEST_ASSERT_FALSE(timing.due(20001));
}

void test_interval_uses_unsigned_rollover_arithmetic() {
  fragilefreight::TelemetryTiming timing;
  TEST_ASSERT_TRUE(timing.due(std::numeric_limits<uint32_t>::max() - 500));
  TEST_ASSERT_FALSE(timing.due(498));
  TEST_ASSERT_TRUE(timing.due(499));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_first_telemetry_is_due_at_one_second);
  RUN_TEST(test_period_is_independent_and_does_not_catch_up);
  RUN_TEST(test_interval_uses_unsigned_rollover_arithmetic);
  return UNITY_END();
}
