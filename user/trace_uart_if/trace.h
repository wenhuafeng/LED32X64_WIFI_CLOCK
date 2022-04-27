#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include "usart.h"

extern void TRACE_ReceiveDmaInit(void);
extern void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
extern void TRACE_HandlerUartData(void);
extern void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart);
extern void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern void print_usart2(char *format, ...);

#define TRACE_PRINTF(...)
    //do {
    //    TRACE_COND_FSend(VLEVEL_OFF, T_REG_OFF, TS_OFF, __VA_ARGS__);
    //} while (0)

#endif
