#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INACTIVITY_TIMEOUT_MS 15000U

typedef enum {
    SYSTEM_STATE_ACTIVE = 0,
    SYSTEM_STATE_INACTIVE
} SystemState;

/* Pure hardware-independent state transition logic (Testable for Part XIV) */
SystemState evaluateSystemState(SystemState currentState, bool motionDetected, uint32_t elapsedInactiveMs);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STATE_H */