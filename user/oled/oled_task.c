#include "main.h"
#if defined(SUPPORT_OLED_DISPLAY) && SUPPORT_OLED_DISPLAY
#include "oled_task.h"
#include <stdint.h>
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
//static struct OledType g_oled = {0};

static void OLED_Task(void *argument)
{
    bool ssd1306Init = false;
    uint32_t event;
    struct OledType oled;

    LOGI(LOG_TAG, "oled task enter!\r\n");
    ssd1306Init = SSD1306_Init();
    if (ssd1306Init == false) {
        LOGE(LOG_TAG, "SSD1306 init error!\r\n");
    }

    while (1) {
        event = osEventFlagsWait(g_oledEvent, DISP_SCAN_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA) == DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA) {
            if (DISP_TaskGetOledData(&oled) != osOK) {
                LOGI(LOG_TAG, "Get oled data error!\r\n");
            }
            if (ssd1306Init == true) {
                SSD1306_GotoXY (0,0);
                SSD1306_Puts("Hello!", &Font_7x10, SSD1306_COLOR_WHITE);
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
}

#else

osStatus_t OLED_TaskInit(void)
{
    return osError;
}

void OLED_TaskSetEvent(uint32_t event)
{
    (void)event;
}
void OLED_TaskSuspend(void) {}
void OLED_TaskResume(void) {}

#endif
