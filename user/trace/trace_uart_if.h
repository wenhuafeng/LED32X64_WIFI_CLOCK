#ifndef TRACE_IF_H
#define TRACE_IF_H

#include "trace.h"
#include "usart.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TraceDriverType *TRACE_UART_GetDriver(void);
void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
void TRACE_UART_DataProcess(void);
void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
