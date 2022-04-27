#ifndef WIFI_UART_IF_H
#define WIFI_UART_IF_H

#include <stdint.h>
#include "usart.h"

extern void WIFI_ReceiveDmaInit(void);
extern void WIFI_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
extern void WIFI_HandlerUartData(void);
extern void WIFI_UART_TxCpltCallback(UART_HandleTypeDef *huart);
extern void WIFI_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern void print_usart1(char *format, ...);

#endif
