#include <stdio.h>
#include "main.h"
#if (WIFI_MODULE == WIFI_ESP8266)
#include "esp8266_at.h"
#elif (WIFI_MODULE == WIFI_EMW3060)
#include "emw3060_at.h"
#endif
#include "usart.h"

#define RECEIVE_LENGTH     200
#define USART_DMA_SENDING  1
#define USART_DMA_SENDOVER 0

struct UsartReceiveType {
    uint8_t receiveFlag : 1;
    uint8_t sendFlag : 1;
    uint16_t rxLength;
    uint8_t buffer[RECEIVE_LENGTH];
};
struct UsartReceiveType g_usartType;

#ifdef __GNUC__

int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xFFFF);
    return len;
}

#else

#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

#endif /* __GNUC__ */

void WIFI_ReceiveDmaInit(void)
{
    HAL_UART_Receive_DMA(&huart1, g_usartType.buffer, RECEIVE_LENGTH);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

void WIFI_SendDataDMA(uint8_t *pdata, uint16_t Length)
{
    while (g_usartType.sendFlag == USART_DMA_SENDING)
        ;
    g_usartType.sendFlag = USART_DMA_SENDING;
    HAL_UART_Transmit_DMA(&huart1, pdata, Length);
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
        HAL_UART_Receive_DMA(&huart1, g_usartType.buffer, RECEIVE_LENGTH);
    }
}

void WIFI_HandlerUartData(void)
{
    if (g_usartType.receiveFlag) {
        g_usartType.receiveFlag = 0;
        WIFI_ReceiveProcess(g_usartType.buffer);
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
