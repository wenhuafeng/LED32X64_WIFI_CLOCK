#ifndef COMMON_H
#define COMMON_H

#include "main.h"

#define WORK_LED_BLINK()                                      \
    do {                                                      \
        HAL_GPIO_TogglePin(WORK_LED_GPIO_Port, WORK_LED_Pin); \
    } while (0)
#define WORK_LED_ON()                                                        \
    do {                                                                     \
        HAL_GPIO_WritePin(WORK_LED_GPIO_Port, WORK_LED_Pin, GPIO_PIN_RESET); \
    } while (0)
#define WORK_LED_OFF()                                                     \
    do {                                                                   \
        HAL_GPIO_WritePin(WORK_LED_GPIO_Port, WORK_LED_Pin, GPIO_PIN_SET); \
    } while (0)

#endif
