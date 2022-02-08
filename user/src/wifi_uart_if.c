#include "main.h"
#if (WIFI_MODULE == WIFI_ESP8266)
#include "esp8266_at.h"
#elif (WIFI_MODULE == WIFI_EMW3060)
#include "emw3060_at.h"
#endif
#include "usart.h"

#define RECEIVE_LENGTH 100
#define USART_DMA_SENDING 1
#define USART_DMA_SENDOVER 0

typedef struct {
    uint8_t receive_flag : 1;
    uint8_t dmaSend_flag : 1;
    uint16_t rx_len;
    uint8_t usartDMA_rxBuf[RECEIVE_LENGTH];
} USART_RECEIVETYPE;

USART_RECEIVETYPE UsartType1;

#ifdef __GNUC__

/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar() */
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

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

void Usart1ReceiveDmaInit(void)
{
    HAL_UART_Receive_DMA(&huart1, UsartType1.usartDMA_rxBuf, RECEIVE_LENGTH);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

void Usart1SendData_DMA(uint8_t *pdata, uint16_t Length)
{
    while (UsartType1.dmaSend_flag == USART_DMA_SENDING)
        ;
    UsartType1.dmaSend_flag = USART_DMA_SENDING;
    HAL_UART_Transmit_DMA(&huart1, pdata, Length);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    __HAL_DMA_DISABLE(huart->hdmatx);
    if (huart->Instance == huart1.Instance)
        UsartType1.dmaSend_flag = USART_DMA_SENDOVER;
}

void Usart1Receive_IDLE(UART_HandleTypeDef *huart)
{
    uint32_t temp;

    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET)) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        HAL_UART_DMAStop(huart);
        if (huart->Instance == huart1.Instance) {
            temp = huart1.hdmarx->Instance->CNDTR;
            UsartType1.rx_len = RECEIVE_LENGTH - temp;
            UsartType1.receive_flag = 1;
            HAL_UART_Receive_DMA(&huart1, UsartType1.usartDMA_rxBuf, RECEIVE_LENGTH);
        }
    }
}

void HandlerUartData(void)
{
    if (UsartType1.receive_flag) {
        UsartType1.receive_flag = 0;
        WIFI_ReceiveProcess(UsartType1.usartDMA_rxBuf);
        //HAL_UART_Receive_DMA(&huart1, UsartType1.usartDMA_rxBuf, RECEIVE_LENGTH);
    }
}
