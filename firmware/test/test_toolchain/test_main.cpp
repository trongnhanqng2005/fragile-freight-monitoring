#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_unity_runner_executes_assertions() {
  TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_unity_runner_executes_assertions);
  return UNITY_END();
}
