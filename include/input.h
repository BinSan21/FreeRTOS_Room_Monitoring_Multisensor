#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISPLAY_MODE_TEMPERATURE = 0,
    DISPLAY_MODE_HUMIDITY,
    DISPLAY_MODE_LIGHT,
    DISPLAY_MODE_MOTION
} DisplayMode;

/* Hardware-independent navigation logic (Testable for Part XIV) */
DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);

/* Hardware driver functions for KY-040 Rotary Encoder */
void Input_Init(void);
int Input_PollEncoderStep(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_H */