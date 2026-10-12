#include "system_state.h"

SystemState evaluateSystemState(SystemState currentState, bool motionDetected, uint32_t elapsedInactiveMs) {
    if (motionDetected) {
        return SYSTEM_STATE_ACTIVE;
    }
    if (currentState == SYSTEM_STATE_ACTIVE && elapsedInactiveMs >= INACTIVITY_TIMEOUT_MS) {
        return SYSTEM_STATE_INACTIVE;
    }
    return currentState;
}