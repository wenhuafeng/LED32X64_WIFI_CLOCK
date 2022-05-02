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
#include "trace.h"

#define LOG_TAG          "display_task"
#define SOFTWARE_VERSION "V101"

#define DISP_TASK_SCAN_MSG_MAX  1
#define DISP_TASK_SCAN_MSG_SIZE (sizeof(struct RgbType))

#define DISP_TASK_EVENT_ALL (0x00ffffff)

#define DISP_TASK_NAME       "dispTask"
#define DISP_TASK_STACK_SIZE (128 * 20)
#define DISP_TASK_PRIORITY   (osPriority_t) osPriorityNormal6

const osThreadAttr_t g_dispTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

static osEventFlagsId_t g_dispEvent = NULL;
static osMessageQueueId_t g_dispScanMsgId = NULL;
static osThreadId_t g_dispTaskId = NULL;

static void DISP_Task(void *argument)
{
    uint32_t event;
    struct Hub75dType hub75d;
    struct TimeType time;

    LOGI(LOG_TAG, "display task enter\r\n");
    LOGI(LOG_TAG, "%s, %s, %s\r\n", SOFTWARE_VERSION, __TIME__, __DATE__);
    memset(&hub75d, 0, sizeof(hub75d));
    DISP_TaskSetEvent(DISP_TASK_EVENT_DISP_ON);

    while (1) {
        event = osEventFlagsWait(g_dispEvent, DISP_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_TASK_EVENT_GET_SCAN_RGB) == DISP_TASK_EVENT_GET_SCAN_RGB) {
            if (ClockRun(&time) == true) {
                GetLunarCalendar(&hub75d.lunarCalendar, &time);
            }
            HUB75D_GetCalendar(&hub75d.calendarDecimal, &time);
            if (HUB75D_CtrDec(&hub75d) == true) {
                HAL_TIM_Base_Stop_IT(&htim4);
                HAL_TIM_Base_MspDeInit(&htim4);
                HUB75D_Disp(&hub75d.displayCount, DISP_TIME_OFF);
                WIFI_TaskSuspend();
                TH_TaskSuspend();
                DISP_ScanTaskSuspend();
            }
            HUB75D_GetScanRgb(&hub75d);
            if (osMessageQueuePut(g_dispScanMsgId, &hub75d.rgb, 0, 0) == osOK) {
                DISP_ScanTaskSetEvent(DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA);
            }
        }
        if ((event & DISP_TASK_EVENT_GET_TH_DATA) == DISP_TASK_EVENT_GET_TH_DATA) {
            TH_TaskGetThData(&hub75d.tempHumi);
        }
        if ((event & DISP_TASK_EVENT_GET_TIME_DATA) == DISP_TASK_EVENT_GET_TIME_DATA) {
            WIFI_TaskGetTimeData(&time);
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
                TH_TaskResume();
                DISP_ScanTaskResume();
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
