#include "main.h"
#if defined(SUPPORT_OLED_DISPLAY) && SUPPORT_OLED_DISPLAY
#include "oled_task.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "hub75d.h"
#include "trace.h"
#include "display_task.h"
#include "fonts.h"
#include "ssd1306.h"

#define LOG_TAG "oled_task"

#define DISP_SCAN_TASK_EVENT_ALL (0x00ffffff)

#define DISP_TASK_NAME       "oled_task"
#define DISP_TASK_STACK_SIZE (128 * 8)
#define DISP_TASK_PRIORITY   (osPriority_t) osPriorityNormal5

const osThreadAttr_t g_oledTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

static osEventFlagsId_t g_oledEvent = NULL;
static osThreadId_t g_oledTaskId    = NULL;

static void OLED_Task(void *argument)
{
    bool init = false;
    uint32_t event;
    char data[16];
    struct OledType oled;

    LOGI(LOG_TAG, "oled task enter!\r\n");
    init = SSD1306_Init();
    if (init == false) {
        LOGE(LOG_TAG, "SSD1306 init error!\r\n");
    }

    while (1) {
        event = osEventFlagsWait(g_oledEvent, DISP_SCAN_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA) == DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA) {
            if (DISP_TaskGetOledData(&oled) != osOK) {
                LOGI(LOG_TAG, "Get oled data error!\r\n");
            }
            if (init == true) {
                SSD1306_GotoXY(2, 0);

                sprintf(data, "%02d:%02d", oled.time.hour, oled.time.minute);
                SSD1306_Puts(data, &Font_16x26, SSD1306_COLOR_WHITE);

                sprintf(data, ":%02d", oled.time.second);
                SSD1306_Puts(data, &Font_11x18, SSD1306_COLOR_WHITE);

                SSD1306_GotoXY(4, 24);
                sprintf(data, "%02d-%02d-%02d", oled.time.day, oled.time.month, oled.time.year);
                SSD1306_Puts(data, &Font_11x18, SSD1306_COLOR_WHITE);

                SSD1306_GotoXY(4, 42);
                sprintf(data, "%d.%01dC %d.%01d%%", (char)(oled.tempHumi.temperature / 10),
                        (char)(oled.tempHumi.temperature % 10), (char)(oled.tempHumi.humidity / 10),
                        (char)(oled.tempHumi.humidity % 10));
                SSD1306_Puts(data, &Font_11x18, SSD1306_COLOR_WHITE);

                SSD1306_UpdateScreen();
            }
        }
    }
}

osStatus_t OLED_TaskInit(void)
{
    if (g_oledTaskId != NULL) {
        return osError;
    }

    g_oledEvent = osEventFlagsNew(NULL);
    if (g_oledEvent == NULL) {
        return osError;
    }

    g_oledTaskId = osThreadNew(OLED_Task, NULL, &g_oledTaskAttributes);
    if (g_oledTaskId == NULL) {
        return osError;
    }

    return osOK;
}

void OLED_TaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_oledEvent, event);
}

void OLED_TaskSuspend(void)
{
    osStatus_t ret;

    ret = osThreadSuspend(g_oledTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "oled task suspend\r\n");
    } else {
        LOGI(LOG_TAG, "oled task suspend\r\n");
    }
    SSD1306_Off();
}

void OLED_TaskResume(void)
{
    osStatus_t ret;

    ret = osThreadResume(g_oledTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "oled task resume\r\n");
    } else {
        LOGI(LOG_TAG, "oled task resume\r\n");
    }
    SSD1306_On();
}

#else

#include <stdint.h>
#include "cmsis_os2.h"

osStatus_t OLED_TaskInit(void)
{
    return osOK;
}

void OLED_TaskSetEvent(uint32_t event)
{
    (void)event;
}
void OLED_TaskSuspend(void) {}
void OLED_TaskResume(void) {}

#endif
