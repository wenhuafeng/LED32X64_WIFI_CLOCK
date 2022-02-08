#ifndef WIFI_UART_IF_H
#define WIFI_UART_IF_H

#include <stdint.h>
#include "usart.h"

void Usart1ReceiveDmaInit(void);
void Usart1Receive_IDLE(UART_HandleTypeDef *huart);
void Usart1SendData_DMA(uint8_t *pdata, uint16_t Length);

void HandlerUartData(void);

#endif
