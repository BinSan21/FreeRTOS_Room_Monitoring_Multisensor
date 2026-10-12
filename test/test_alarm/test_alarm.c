#include <unity.h>
#include "alarm.h"

void setUp(void) {}
void tearDown(void) {}

void test_alarm_below_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_STATE_LOW_TEMPERATURE, evaluateTemperature(17.9f));
}
void test_alarm_at_exact_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_STATE_NORMAL, evaluateTemperature(18.0f));
}
void test_alarm_within_normal_range(void) {
    TEST_ASSERT_EQUAL(ALARM_STATE_NORMAL, evaluateTemperature(25.4f));
}
void test_alarm_at_exact_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_STATE_NORMAL, evaluateTemperature(30.0f));
}
void test_alarm_above_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_STATE_HIGH_TEMPERATURE, evaluateTemperature(30.1f));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_alarm_below_lower_threshold);
    RUN_TEST(test_alarm_at_exact_lower_threshold);
    RUN_TEST(test_alarm_within_normal_range);
    RUN_TEST(test_alarm_at_exact_upper_threshold);
    RUN_TEST(test_alarm_above_upper_threshold);
    return UNITY_END();
}
