#include "trace.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "usart.h"

#define RECEIVE_LENGTH     200
#define SEND_LENGTH        200
#define USART_DMA_SENDING  1
#define USART_DMA_SENDOVER 0

struct UsartReceiveType {
    uint8_t receiveFlag : 1;
    uint8_t sendFlag : 1;
    uint16_t rxLength;
    uint8_t buffer[RECEIVE_LENGTH];
};
static struct UsartReceiveType g_usartType;

void TRACE_ReceiveDmaInit(void)
{
    HAL_UART_Receive_DMA(&huart2, g_usartType.buffer, RECEIVE_LENGTH);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
}

void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart)
{
    uint32_t temp;

    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) == RESET) {
        return;
    }

    __HAL_UART_CLEAR_IDLEFLAG(huart);
    HAL_UART_DMAStop(huart);
    if (huart->Instance == huart2.Instance) {
        temp                    = huart2.hdmarx->Instance->CNDTR;
        g_usartType.rxLength    = RECEIVE_LENGTH - temp;
        g_usartType.receiveFlag = 1;
        HAL_UART_Receive_DMA(&huart2, g_usartType.buffer, RECEIVE_LENGTH);
    }
}

void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    __HAL_DMA_DISABLE(huart->hdmatx);
    if (huart->Instance == huart2.Instance) {
        g_usartType.sendFlag = USART_DMA_SENDOVER;
    }
}

void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
}

static int inHandlerMode (void)
{
    return __get_IPSR() != 0;
}

void PrintUsart2(char *format, ...)
{
    char buf[SEND_LENGTH];
    int length;

    if (inHandlerMode() != 0) {
        taskDISABLE_INTERRUPTS();
    } else {
        while (HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX) {
            osThreadYield();
        }
    }

    va_list ap;
    va_start(ap, format);
    length = vsprintf(buf, format, ap);
    if (length > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, length, 1000);
    }
    va_end(ap);

    if (inHandlerMode() != 0) {
        taskENABLE_INTERRUPTS();
    }
}
