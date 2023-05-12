#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include "usart.h"

#define PRINT_USART      0
#define PRINT_SEGGER_RTT 1
#define PRINTF_TYPE      PRINT_SEGGER_RTT

extern void TRACE_ReceiveDmaInit(void);
extern void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart);
extern void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart);
extern void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern void PrintUsart2(char *format, ...);

#if (PRINTF_TYPE == PRINT_USART)
#define PRINTF PrintUsart2

#define LOGF(tag, format, ...)                                     \
    do {                                                           \
        PRINTF("Fatal: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGE(tag, format, ...)                                     \
    do {                                                           \
        PRINTF("Error: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGW(tag, format, ...)                                     \
    do {                                                           \
        PRINTF("Warn:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGI(tag, format, ...)                                     \
    do {                                                           \
        PRINTF("Info:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGD(tag, format, ...)                                     \
    do {                                                           \
        PRINTF("Debug: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)

#elif (PRINTF_TYPE == PRINT_SEGGER_RTT)
#include "SEGGER_RTT.h"
#define PRINTF SEGGER_RTT_printf

#define LOGF(tag, format, ...)                                   \
    do {                                                         \
        PRINTF(0, "Fatal: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGE(tag, format, ...)                                   \
    do {                                                         \
        PRINTF(0, "Error: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGW(tag, format, ...)                                   \
    do {                                                         \
        PRINTF(0, "Warn:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGI(tag, format, ...)                                   \
    do {                                                         \
        PRINTF(0, "Info:  [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)
#define LOGD(tag, format, ...)                                   \
    do {                                                         \
        PRINTF(0, "Debug: [" tag "] " format "", ##__VA_ARGS__); \
    } while (0)

#endif
#endif
