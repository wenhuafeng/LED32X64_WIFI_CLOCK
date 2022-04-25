#include "common.h"
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "gpio.h"
#include "dma.h"
#include "tim.h"
#include "rtc.h"
#include "hub75d.h"
#include "htu21d.h"
#include "lunar_calendar.h"
#if (WIFI_MODULE == WIFI_ESP8266)
#include "esp8266_at.h"
#elif (WIFI_MODULE == WIFI_EMW3060)
#include "emw3060_at.h"
#endif
#include "wifi_uart_if.h"
//#include "trace_uart_if.h"
#include "trace_printf.h"
#include "time_stamp.h"
#include "display_task.h"

#define SOFTWARE_VERSION "V101"

static void ClearGpioExtiIT(uint16_t GPIO_Pin)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
    }
}

/*
static void COMMON_EnterStandbyMode(void)
{
    //TRACE_PRINTF("enter stop mode\r\n");

    //HAL_TIM_Base_Stop_IT(&htim4);
    //HAL_TIM_Base_MspDeInit(&htim4);

    //WIFI_Power(POWER_OFF);
    //HUB75D_Disp(DISP_TIME_OFF);
    WORK_LED_OFF();

    //HAL_Delay(100);
    ClearGpioExtiIT(PIR_INT_Pin);

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    //MX_RTC_Init();
    MX_TIM4_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    COMMON_Init();
    //WIFI_ReInit();

    TRACE_PRINTF("exit stop mode\r\n");
}*/

void COMMON_Init(void)
{
    //if (TRACE_Init() != UTIL_ADV_TRACE_OK) {
    //    __asm("nop");
    //} else {
    //    TRACE_PRINTF("trace init ok\r\n");
    //}
    TRACE_PRINTF("%s, %s, %s\r\n", SOFTWARE_VERSION, __TIME__, __DATE__);

    //WIFI_ReceiveDmaInit();
    //WIFI_Power(POWER_ON);

    HAL_RTC_MspInit(&hrtc);
    HAL_RTCEx_SetSecond_IT(&hrtc);

    //HAL_Delay(100);
    //HTU21D_Init();
    //HTU21D_GetData();

    //HAL_Delay(100);
    //GetClock();

    //HAL_Delay(100);
    //HUB75D_Init();
    //HUB75D_Disp(DISP_TIME);
    //HAL_TIM_Base_MspInit(&htim4);
    //HAL_TIM_Base_Start_IT(&htim4);
}

void COMMON_Process(void)
{
    //IsPirIntFlagSet();
    //WIFI_HandlerUartData();

    //if (Get1sFlag() == false) {
    //    return;
    //}
    //SetOneSecondFlag(false);

    TimestampAdd();
    //WIFI_SendCommand();
    //HTU21D_Sampling();
    //if (ClockRun() == true) {
    //    CalculationLunarCalendar(GetTimeData());
    //}
    //HUB75D_GetCalendar(GetTimeData());
    //if (HUB75D_CtrDec() == true) {
    //    COMMON_EnterStandbyMode();
    //}
    //HUB75D_GetScanRgb();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        DISP_TaskSetEvent(DISP_TASK_EVENT_DISP_ON);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_TxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        //TRACE_UART_TxCpltCallback(huart);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_RxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        //TRACE_UART_RxCpltCallback(huart);
    }
}
