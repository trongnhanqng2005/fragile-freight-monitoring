#include <unity.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <ImpactLogic.h>

using fragilefreight::ImpactLogic;
using fragilefreight::ImpactState;
using fragilefreight::classifyImpact;
using fragilefreight::stateName;

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_classification_preserves_exact_threshold_boundaries() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::SAFE),
                        static_cast<int>(classifyImpact(std::nextafter(0.4f, 0.0f))));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::WARNING),
                        static_cast<int>(classifyImpact(0.4f)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::WARNING),
                        static_cast<int>(classifyImpact(std::nextafter(0.9f, 0.0f))));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::DANGER),
                        static_cast<int>(classifyImpact(0.9f)));
}

void test_magnitude_and_deviation_use_all_three_axes() {
  ImpactLogic logic;
  const auto sample = logic.update(3.0f, 4.0f, 0.0f, 10);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, sample.magnitudeG);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, sample.deviationG);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ImpactState::DANGER), static_cast<int>(sample.state));
  TEST_ASSERT_TRUE(sample.valid);
}

void test_one_event_is_returned_per_armed_danger() {
  ImpactLogic logic;
  TEST_ASSERT_TRUE(logic.update(0, 0, 2.0f, 10).dangerEvent);
  TEST_ASSERT_FALSE(logic.update(0, 0, 2.0f, 20).dangerEvent);
  TEST_ASSERT_FALSE(logic.alertArmed());
}

void test_warning_does_not_rearm_after_cooldown() {
  ImpactLogic logic;
  logic.update(0, 0, 2.0f, 10);
  logic.update(0, 0, 1.5f, 1010);
  TEST_ASSERT_FALSE(logic.alertArmed());
  TEST_ASSERT_FALSE(logic.update(0, 0, 2.0f, 1020).dangerEvent);
}

void test_safe_rearms_at_cooldown_from_previous_danger() {
  ImpactLogic logic;
  logic.update(0, 0, 2.0f, 10);
  logic.update(0, 0, 1.0f, 1009);
  TEST_ASSERT_FALSE(logic.alertArmed());
  logic.update(0, 0, 1.0f, 1010);
  TEST_ASSERT_TRUE(logic.alertArmed());
  TEST_ASSERT_TRUE(logic.update(0, 0, 2.0f, 1020).dangerEvent);
}

void test_safe_after_cooldown_rearms_immediately_after_extended_danger() {
  ImpactLogic logic;
  logic.update(0, 0, 2.0f, 10);
  logic.update(0, 0, 2.0f, 2010);
  logic.update(0, 0, 1.0f, 2020);
  TEST_ASSERT_TRUE(logic.alertArmed());
  TEST_ASSERT_TRUE(logic.update(0, 0, 2.0f, 2030).dangerEvent);
}

void test_cooldown_arithmetic_handles_uint32_rollover() {
  ImpactLogic logic;
  logic.update(0, 0, 2.0f, UINT32_MAX - 500);
  logic.update(0, 0, 1.0f, 498);
  TEST_ASSERT_FALSE(logic.alertArmed());
  logic.update(0, 0, 1.0f, 499);
  TEST_ASSERT_TRUE(logic.alertArmed());
}

void test_non_finite_measurements_are_marked_invalid() {
  ImpactLogic logic;
  const auto sample = logic.update(std::numeric_limits<float>::quiet_NaN(), 0, 1.0f, 10);
  TEST_ASSERT_FALSE(sample.valid);
}

void test_state_names_are_stable() {
  TEST_ASSERT_EQUAL_STRING("SAFE", stateName(ImpactState::SAFE));
  TEST_ASSERT_EQUAL_STRING("WARNING", stateName(ImpactState::WARNING));
  TEST_ASSERT_EQUAL_STRING("DANGER", stateName(ImpactState::DANGER));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_classification_preserves_exact_threshold_boundaries);
  RUN_TEST(test_magnitude_and_deviation_use_all_three_axes);
  RUN_TEST(test_one_event_is_returned_per_armed_danger);
  RUN_TEST(test_warning_does_not_rearm_after_cooldown);
  RUN_TEST(test_safe_rearms_at_cooldown_from_previous_danger);
  RUN_TEST(test_safe_after_cooldown_rearms_immediately_after_extended_danger);
  RUN_TEST(test_cooldown_arithmetic_handles_uint32_rollover);
  RUN_TEST(test_non_finite_measurements_are_marked_invalid);
  RUN_TEST(test_state_names_are_stable);
  return UNITY_END();
}
