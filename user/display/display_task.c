#include "display_task.h"
#include <string.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "hub75d.h"
#include "time_run.h"
#include "display_scan_task.h"
#include "temp_humi_task.h"
#include "wifi_task.h"
#include "gps_task.h"
#include "oled_task.h"
#include "trace.h"

#define LOG_TAG          "display_task"
#define SOFTWARE_VERSION "V101"

#define DISP_TASK_SCAN_MSG_MAX  1
#define DISP_TASK_SCAN_MSG_SIZE (sizeof(struct RgbType))

#define DISP_TASK_OLED_MSG_MAX  1
#define DISP_TASK_OLED_MSG_SIZE (sizeof(struct OledType))

#define DISP_TASK_EVENT_ALL (0x00ffffff)

#define DISP_TASK_NAME       "dispTask"
#define DISP_TASK_STACK_SIZE (128 * 22)
#define DISP_TASK_PRIORITY   (osPriority_t) osPriorityNormal7

const osThreadAttr_t g_dispTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

static osEventFlagsId_t g_dispEvent       = NULL;
static osMessageQueueId_t g_dispScanMsgId = NULL;
static osMessageQueueId_t g_oledMsgId     = NULL;
static osThreadId_t g_dispTaskId          = NULL;

static void DISP_Task(void *argument)
{
    uint32_t event;
    struct Hub75dType hub75d;
    struct TimeType time;
    struct OledType oled;

    LOGI(LOG_TAG, "display task enter\r\n");
    LOGI(LOG_TAG, "%s, %s, %s\r\n", SOFTWARE_VERSION, __TIME__, __DATE__);
    memset(&hub75d, 0, sizeof(hub75d));
    DISP_TaskSetEvent(DISP_TASK_EVENT_DISP_ON);

    while (1) {
        event = osEventFlagsWait(g_dispEvent, DISP_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_TASK_EVENT_GET_SCAN_RGB) == DISP_TASK_EVENT_GET_SCAN_RGB) {
            if (ClockRun(&time) == true) {
                GetLunarCalendar(&hub75d.lunarCalendar, &time);
                memcpy(&oled.lunarCalendar, &hub75d.lunarCalendar, sizeof(struct LunarCalendarType));
            }
            HUB75D_GetCalendar(&hub75d.calendarDecimal, &time);
            memcpy(&oled.calendarDecimal, &hub75d.calendarDecimal, sizeof(struct CalendarDecimal));
            if (HUB75D_CtrDec(&hub75d) == true) {
                HAL_TIM_Base_Stop_IT(&htim4);
                HAL_TIM_Base_MspDeInit(&htim4);
                HUB75D_Disp(&hub75d.displayCount, DISP_TIME_OFF);
                WIFI_TaskSuspend();
                GPS_TaskSuspend();
                TH_TaskSuspend();
                DISP_ScanTaskSuspend();
                OLED_TaskSuspend();
            }
            HUB75D_GetScanRgb(&hub75d);
            if (osMessageQueuePut(g_dispScanMsgId, &hub75d.rgb, 0, 0) == osOK) {
                DISP_ScanTaskSetEvent(DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA);
            }
            if (osMessageQueuePut(g_oledMsgId, &oled, 0, 0) == osOK) {
                OLED_TaskSetEvent(DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA);
            }
        }
        if ((event & DISP_TASK_EVENT_GET_TH_DATA) == DISP_TASK_EVENT_GET_TH_DATA) {
            TH_TaskGetThData(&hub75d.tempHumi);
            memcpy(&oled.tempHumi, &hub75d.tempHumi, sizeof(struct Htu21dDataType));
        }
        if ((event & DISP_TASK_EVENT_GET_TIME_DATA) == DISP_TASK_EVENT_GET_TIME_DATA) {
            WIFI_TaskGetTimeData(&time);
            GPS_TaskGetTimeData(&time);
            GetLunarCalendar(&hub75d.lunarCalendar, &time);
        }
        if ((event & DISP_TASK_EVENT_DISP_ON) == DISP_TASK_EVENT_DISP_ON) {
            if (hub75d.displayCount == DISP_TIME_OFF) {
                HUB75D_Init();
                memset(&hub75d, 0, sizeof(hub75d));
                HUB75D_Disp(&hub75d.displayCount, DISP_TIME);
                HAL_TIM_Base_MspInit(&htim4);
                HAL_TIM_Base_Start_IT(&htim4);
                GetClock(&time);
                GetLunarCalendar(&hub75d.lunarCalendar, &time);
            }
        }
        if ((event & DISP_TASK_EVENT_PIR_INT) == DISP_TASK_EVENT_PIR_INT) {
            LOGI(LOG_TAG, "pir interrupt, renew set display 5 minute\r\n");
            if (hub75d.displayCount == DISP_TIME_OFF) {
                WIFI_TaskResume();
                GPS_TaskResume();
                TH_TaskResume();
                DISP_ScanTaskResume();
                OLED_TaskResume();
                DISP_TaskSetEvent(DISP_TASK_EVENT_DISP_ON);
                HUB75D_GetCalendar(&hub75d.calendarDecimal, &time);
                HUB75D_GetScanRgb(&hub75d);
                if (osMessageQueuePut(g_dispScanMsgId, &hub75d.rgb, 0, 0) == osOK) {
                    DISP_ScanTaskSetEvent(DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA);
                }
            } else {
                HUB75D_Disp(&hub75d.displayCount, DISP_TIME);
            }
        }
    }
}

osStatus_t DISP_TaskInit(void)
{
    if (g_dispTaskId != NULL) {
        return osError;
    }

    g_dispEvent = osEventFlagsNew(NULL);
    if (g_dispEvent == NULL) {
        return osError;
    }

    g_dispScanMsgId = osMessageQueueNew(DISP_TASK_SCAN_MSG_MAX, DISP_TASK_SCAN_MSG_SIZE, NULL);
    if (g_dispScanMsgId == NULL) {
        return osError;
    }

    g_oledMsgId = osMessageQueueNew(DISP_TASK_OLED_MSG_MAX, DISP_TASK_OLED_MSG_SIZE, NULL);
    if (g_oledMsgId == NULL) {
        return osError;
    }

    g_dispTaskId = osThreadNew(DISP_Task, NULL, &g_dispTaskAttributes);
    if (g_dispTaskId == NULL) {
        return osError;
    }

    return osOK;
}

void DISP_TaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_dispEvent, event);
}

osStatus_t DISP_TaskGetDispScanData(struct RgbType *scanRgb)
{
    osStatus_t status;

    status = osMessageQueueGet(g_dispScanMsgId, scanRgb, NULL, 0);

    return status;
}

osStatus_t DISP_TaskGetOledData(struct OledType *oled)
{
    osStatus_t status;

    status = osMessageQueueGet(g_oledMsgId, oled, NULL, 0);

    return status;
}

