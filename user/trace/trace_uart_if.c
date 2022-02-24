#include "trace_uart_if.h"
#include <stdbool.h>
#include "dma.h"

extern DMA_HandleTypeDef hdma_usart2_tx;
extern UART_HandleTypeDef huart2;

#define RX_BUFFER_LEN 50
struct ReceiveType {
    uint8_t rxFlag : 1;
    uint8_t txFlag : 1;
    uint16_t rxLength;
    uint8_t rxBuffer[RX_BUFFER_LEN];
};

static struct ReceiveType g_rxType;
static bool g_rxComplete;

static void (*TxCpltCallback)(void *);
static void (*RxCpltCallback)(uint8_t *rx, uint16_t len, uint8_t error);

enum TraceStatus COM_Init(void (*cb)(void *));
enum TraceStatus COM_ReceiveInit(void (*RxCb)(uint8_t *rxChar, uint16_t size, uint8_t error));
enum TraceStatus COM_DeInit(void);
void COM_Resume(void);
void COM_Trace(uint8_t *p_data, uint16_t size);
enum TraceStatus COM_TraceDMA(uint8_t *p_data, uint16_t size);

struct TraceDriverType TraceDriver = {
    COM_Init,
    COM_DeInit,
    COM_ReceiveInit,
    COM_TraceDMA,
};

enum TraceStatus COM_Init(void (*cb)(void *))
{
    TxCpltCallback = cb;
    //MX_DMA_Init();
    //MX_USART2_UART_Init();

    return UTIL_ADV_TRACE_OK;
}

enum TraceStatus COM_DeInit(void)
{
    /* -1- Reset peripherals */
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

    /* -2- MspDeInit */
    HAL_UART_MspDeInit(&huart2);

    /* -3- Disable the NVIC for DMA */
    /* USER CODE BEGIN 1 */
    HAL_NVIC_DisableIRQ(DMA1_Channel6_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel7_IRQn);

    return UTIL_ADV_TRACE_OK;
}

void COM_Trace(uint8_t *inData, uint16_t size)
{
    HAL_UART_Transmit(&huart2, inData, size, 1000);
}

enum TraceStatus COM_TraceDMA(uint8_t *inData, uint16_t size)
{
    HAL_UART_Transmit_DMA(&huart2, inData, size);
    return UTIL_ADV_TRACE_OK;
}

enum TraceStatus COM_ReceiveInit(void (*RxCb)(uint8_t *rx, uint16_t len, uint8_t error))
{
    RxCpltCallback = RxCb;

    /* Make sure that no UART transfer is on-going */
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET)
        ;

    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_rxType.rxBuffer, sizeof(g_rxType.rxBuffer));
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);

    return UTIL_ADV_TRACE_OK;
}

void COM_Resume(void)
{
    /* to re-enable lost DMA settings */
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK) {
        Error_Handler();
    }
}

struct TraceDriverType *TRACE_UART_GetDriver(void)
{
    return &TraceDriver;
}

void TRACE_UART_ReceiveIDLE(UART_HandleTypeDef *huart)
{
    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET)) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        HAL_UART_DMAStop(huart);
        g_rxType.rxLength = sizeof(g_rxType.rxBuffer) - huart->hdmarx->Instance->CNDTR;
        g_rxType.rxFlag = 1;
        if ((RxCpltCallback != NULL) && (huart->ErrorCode == HAL_UART_ERROR_NONE) &&
            (sizeof(g_rxType.rxBuffer) > g_rxType.rxLength)) {
            g_rxComplete = true;
        }
        HAL_UARTEx_ReceiveToIdle_DMA(huart, g_rxType.rxBuffer, sizeof(g_rxType.rxBuffer));
    }
}

void TRACE_UART_DataProcess(void)
{
    if (g_rxComplete == false) {
        return;
    }
    g_rxComplete = false;

    RxCpltCallback(g_rxType.rxBuffer, g_rxType.rxLength, 0);
}

void TRACE_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* buffer transmission complete */
    TxCpltCallback(NULL);
}

void TRACE_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //if ((RxCpltCallback != NULL) && (huart->ErrorCode == HAL_UART_ERROR_NONE)) {
    //    RxCpltCallback(rxBuffer, sizeof(rxBuffer), 0);
    //}
    //HAL_UART_Receive_IT(huart, &charRx, 1);
}
