#ifndef GPS_UART_IF_H
#define GPS_UART_IF_H

#include <stdint.h>
#include "usart.h"

extern void GPS_ReceiveDmaInit(void);
extern void GPS_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
extern void GPS_UART_TxCpltCallback(UART_HandleTypeDef *huart);
extern void GPS_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern void PrintUsart3(char *format, ...);

#endif
