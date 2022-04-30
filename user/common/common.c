#include "common.h"
#include <stdint.h>
#include "wifi_uart_if.h"
#include "trace.h"
#include "display_task.h"

//static void ClearGpioExtiIT(uint16_t GPIO_Pin)
//{
//    if (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != 0x00u) {
//        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
//    }
//}

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
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_RxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        TRACE_UART_RxCpltCallback(huart);
    }
}
