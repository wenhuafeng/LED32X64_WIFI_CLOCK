#ifndef WIFI_UART_IF_H
#define WIFI_UART_IF_H

#include <stdint.h>
#include "usart.h"

void WIFI_ReceiveDmaInit(void);
void WIFI_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
void WIFI_SendDataDMA(uint8_t *pdata, uint16_t Length);
void WIFI_HandlerUartData(void);
void WIFI_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void WIFI_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif
