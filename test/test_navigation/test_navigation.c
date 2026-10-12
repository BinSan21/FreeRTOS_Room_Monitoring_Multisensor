#include <unity.h>
#include "input.h"

void setUp(void) {}
void tearDown(void) {}

void test_nav_clockwise_forward_transitions(void) {
    TEST_ASSERT_EQUAL(DISPLAY_MODE_HUMIDITY, nextDisplayMode(DISPLAY_MODE_TEMPERATURE));
    TEST_ASSERT_EQUAL(DISPLAY_MODE_LIGHT,    nextDisplayMode(DISPLAY_MODE_HUMIDITY));
    TEST_ASSERT_EQUAL(DISPLAY_MODE_MOTION,   nextDisplayMode(DISPLAY_MODE_LIGHT));
}
void test_nav_clockwise_wraparound(void) {
    TEST_ASSERT_EQUAL(DISPLAY_MODE_TEMPERATURE, nextDisplayMode(DISPLAY_MODE_MOTION));
}
void test_nav_counterclockwise_reverse_transitions(void) {
    TEST_ASSERT_EQUAL(DISPLAY_MODE_LIGHT,       previousDisplayMode(DISPLAY_MODE_MOTION));
    TEST_ASSERT_EQUAL(DISPLAY_MODE_HUMIDITY,    previousDisplayMode(DISPLAY_MODE_LIGHT));
    TEST_ASSERT_EQUAL(DISPLAY_MODE_TEMPERATURE, previousDisplayMode(DISPLAY_MODE_HUMIDITY));
}
void test_nav_counterclockwise_wraparound(void) {
    TEST_ASSERT_EQUAL(DISPLAY_MODE_MOTION, previousDisplayMode(DISPLAY_MODE_TEMPERATURE));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_nav_clockwise_forward_transitions);
    RUN_TEST(test_nav_clockwise_wraparound);
    RUN_TEST(test_nav_counterclockwise_reverse_transitions);
    RUN_TEST(test_nav_counterclockwise_wraparound);
    return UNITY_END();
}
