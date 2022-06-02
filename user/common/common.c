#include "common.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "wifi_uart_if.h"
#include "gps_uart_if.h"
#include "display_task.h"
#include "trace.h"

#define LOG_TAG "common"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        DISP_TaskSetEvent(DISP_TASK_EVENT_PIR_INT);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_TxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        TRACE_UART_TxCpltCallback(huart);
    } else if (huart->Instance == USART3) {
        GPS_UART_TxCpltCallback(huart);
    } else {
        ;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_RxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        TRACE_UART_RxCpltCallback(huart);
    } else if (huart->Instance == USART3) {
        GPS_UART_RxCpltCallback(huart);
    } else {
        ;
    }
}

void PreSleepProcessing(uint32_t *ulExpectedIdleTime)
{
}

void PostSleepProcessing(uint32_t *ulExpectedIdleTime)
{
}
