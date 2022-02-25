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
#include "trace_uart_if.h"
#include "trace_printf.h"

#define SOFTWARE_VERSION "V101"

bool g_pirInt = false;

static void COMMON_EnterStandbyMode(void)
{
    TRACE_PRINTF("enter standby mode\n\r");

    /* wifi off */
    WIFI_PowerOnOff(POWER_OFF);
    /* display off */
    HUB75D_DispOnOff(DISP_TIME_OFF);
    /* LED off */
    HAL_GPIO_WritePin(WORK_LED_GPIO_Port, WORK_LED_Pin, GPIO_PIN_SET);

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    /* Initialize interrupts */
    //MX_NVIC_Init();
    HAL_RTCEx_SetSecond_IT(&hrtc);
    GetClock();
    HAL_Delay(200);
    HTU21D_Init();
    /* wifi init */
    WIFI_PowerOnOff(POWER_ON);
    HAL_Delay(100);
    WIFI_Init();
    WIFI_ReceiveDmaInit();
    //DISP power on
    HUB75D_DispOnOff(DISP_TIME_5MIN);
    HAL_Delay(100);
    MX_TIM4_Init();
    HAL_TIM_Base_Start_IT(&htim4);

    TRACE_PRINTF("exit standby mode\n\r");
}

static void IsPirIntFlagSet(void)
{
    if (g_pirInt == false) {
        return;
    }
    g_pirInt = false;

    HUB75D_SetDispOffCtr(DISP_TIME_5MIN);
    TRACE_PRINTF("pir interrupt, renew set display 5 minute\r\n");
}

void COMMON_Init(void)
{
    if (TRACE_Init() != UTIL_ADV_TRACE_OK) {
        __asm("nop");
    } else {
        TRACE_PRINTF("trace init ok\r\n");
    }

    WIFI_ReceiveDmaInit();
    WIFI_PowerOnOff(POWER_ON);
    HAL_RTCEx_SetSecond_IT(&hrtc);
    HAL_Delay(200);
    HTU21D_Init();
    HAL_Delay(100);
    GetClock();
    CalculationLunarCalendar(GetTimeData());
    HUB75D_DispOnOff(DISP_TIME_5MIN);
    HAL_TIM_Base_Start_IT(&htim4);
    WIFI_Init();

    TRACE_PRINTF("%s, %s, %s\n", SOFTWARE_VERSION, __TIME__, __DATE__);
}

void COMMON_Process(void)
{
    IsPirIntFlagSet();
    WIFI_HandlerUartData();
    if (Get1sFlag() == true) {
        Set1sFlag(false);
        WIFI_CtrDec();
        HTU21D_Sampling();
        if (ClockRun() == true) {
            CalculationLunarCalendar(GetTimeData());
        }
        HUB75D_CalculateCalendar(GetTimeData());
        if (HUB75D_CtrDec() == true) {
            COMMON_EnterStandbyMode();
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        g_pirInt = true;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_TxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        TRACE_UART_TxCpltCallback(huart);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        WIFI_UART_RxCpltCallback(huart);
    } else if (huart->Instance == USART2) {
        TRACE_UART_RxCpltCallback(huart);
    }
}