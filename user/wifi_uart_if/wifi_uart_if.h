#ifndef WIFI_UART_IF_H
#define WIFI_UART_IF_H

#include <stdint.h>
#include "usart.h"

void USART1_ReceiveDmaInit(void);
void USART1_Receive_IDLE(UART_HandleTypeDef *huart);
void USART1_SendData_DMA(uint8_t *pdata, uint16_t Length);
void USART1_HandlerUartData(void);

#endif
