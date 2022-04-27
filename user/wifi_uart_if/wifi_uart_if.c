#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "usart.h"
#include "wifi_task.h"

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

void WIFI_ReceiveDmaInit(void)
{
    HAL_UART_Receive_DMA(&huart1, g_usartType.buffer, RECEIVE_LENGTH);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

void WIFI_UART_ReceiveIDLE(UART_HandleTypeDef *huart)
{
    uint32_t temp;

    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) == RESET) {
        return;
    }
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    HAL_UART_DMAStop(huart);

    if (huart->Instance == huart1.Instance) {
        temp                    = huart1.hdmarx->Instance->CNDTR;
        g_usartType.rxLength    = RECEIVE_LENGTH - temp;
        g_usartType.receiveFlag = 1;
        HAL_UART_Receive_DMA(&huart1, g_usartType.buffer, sizeof(g_usartType.buffer));
    }
}

void WIFI_HandlerUartData(void)
{
    if (g_usartType.receiveFlag) {
        g_usartType.receiveFlag = 0;
        //WIFI_ReceiveProcess(g_usartType.buffer);
        //memset(g_usartType.buffer, 0, sizeof(g_usartType.buffer));
        (void)WIFI_TaskSendBuffer(g_usartType.buffer);
    }
}

void WIFI_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    __HAL_DMA_DISABLE(huart->hdmatx);
    if (huart->Instance == huart1.Instance) {
        g_usartType.sendFlag = USART_DMA_SENDOVER;
    }
}

void WIFI_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
}

static int inHandlerMode (void)
{
    return __get_IPSR() != 0;
}

void print_usart1(char *format, ...)
{
    char buf[SEND_LENGTH];

    if (inHandlerMode() != 0) {
        taskDISABLE_INTERRUPTS();
    } else {
        while (HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX) {
            osThreadYield();
        }
    }

    va_list ap;
    va_start(ap, format);
    if (vsprintf(buf, format, ap) > 0) {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buf, strlen(buf));
    }
    va_end(ap);

    if (inHandlerMode() != 0) {
        taskENABLE_INTERRUPTS();
    }
}
