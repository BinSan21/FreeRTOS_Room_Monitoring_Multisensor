#include <unity.h>
#include "system_state.h"

void setUp(void) {}
void tearDown(void) {}

void test_state_active_no_timeout_remains_active(void) {
    TEST_ASSERT_EQUAL(SYSTEM_STATE_ACTIVE, evaluateSystemState(SYSTEM_STATE_ACTIVE, false, 14999U));
}
void test_state_active_at_timeout_becomes_inactive(void) {
    TEST_ASSERT_EQUAL(SYSTEM_STATE_INACTIVE, evaluateSystemState(SYSTEM_STATE_ACTIVE, false, 15000U));
}
void test_state_active_with_motion_even_at_timeout_stays_active(void) {
    TEST_ASSERT_EQUAL(SYSTEM_STATE_ACTIVE, evaluateSystemState(SYSTEM_STATE_ACTIVE, true, 20000U));
}
void test_state_inactive_no_motion_remains_inactive(void) {
    TEST_ASSERT_EQUAL(SYSTEM_STATE_INACTIVE, evaluateSystemState(SYSTEM_STATE_INACTIVE, false, 30000U));
}
void test_state_inactive_with_motion_restores_active(void) {
    TEST_ASSERT_EQUAL(SYSTEM_STATE_ACTIVE, evaluateSystemState(SYSTEM_STATE_INACTIVE, true, 30000U));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_state_active_no_timeout_remains_active);
    RUN_TEST(test_state_active_at_timeout_becomes_inactive);
    RUN_TEST(test_state_active_with_motion_even_at_timeout_stays_active);
    RUN_TEST(test_state_inactive_no_motion_remains_inactive);
    RUN_TEST(test_state_inactive_with_motion_restores_active);
    return UNITY_END();
}
