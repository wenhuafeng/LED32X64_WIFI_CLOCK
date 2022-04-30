#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include "usart.h"

extern void TRACE_ReceiveDmaInit(void);
extern void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
extern void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart);
extern void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern void PrintUsart2(char *format, ...);

#define LOGF(tag, format, ...)                                     \
    do {                                                           \
        PrintUsart2("Fatal: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGE(tag, format, ...)                                     \
    do {                                                           \
        PrintUsart2("Error: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGW(tag, format, ...)                                     \
    do {                                                           \
        PrintUsart2("Warn:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGI(tag, format, ...)                                     \
    do {                                                           \
        PrintUsart2("Info:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGD(tag, format, ...)                                     \
    do {                                                           \
        PrintUsart2("Debug: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)

#endif
