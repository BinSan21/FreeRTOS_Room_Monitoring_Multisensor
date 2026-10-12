#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS Hook & Assertion Stubs required by FreeRTOSConfig.h */
void vApplicationTickHook(void) {
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    while (1) {
    }
}

void Diag_AssertFailed(const char *file, int line) {
    (void)file;
    (void)line;
    while (1) {
    }
}

int main(void) {
    HAL_Init();
    while (1) {
    }
}