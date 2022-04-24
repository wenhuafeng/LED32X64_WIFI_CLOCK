#include "display_task.h"
#include <stdbool.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "hub75d.h"
#include "time_run.h"
#include "display_scan_task.h"
#include "temp_humi_task.h"
#include "trace_printf.h"

#define DISP_TASK_SCAN_MSG_MAX  1
#define DISP_TASK_SCAN_MSG_SIZE (sizeof(struct RgbType))

#define DISP_TASK_EVENT_ALL (0xffffffff)

#define DISP_TASK_NAME       "dispTask"
#define DISP_TASK_STACK_SIZE (128 * 4)
#define DISP_TASK_PRIORITY   (osPriority_t)osPriorityNormal6

const osThreadAttr_t g_dispTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

osEventFlagsId_t g_dispEvent;
osMessageQueueId_t g_dispScanMsgId;

static void DISP_Task(void *argument)
{
    uint32_t event;
    struct Hub75dType hub75d;
    struct TimeType time;

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
            }
            HUB75D_GetScanRgb(&hub75d);
            if (osMessageQueuePut(g_dispScanMsgId, &hub75d.rgbScan, 0, 0) == osOK) {
                DISP_ScanTaskSetEvent(DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA);
            }
        }
        if ((event & DISP_TASK_EVENT_GET_TH_DATA) == DISP_TASK_EVENT_GET_TH_DATA) {
            TH_TaskGetThData(&hub75d.tempHumi);
        }
        if ((event & DISP_TASK_EVENT_DISP_ON) == DISP_TASK_EVENT_DISP_ON) {
            HUB75D_Init();
            HUB75D_Disp(&hub75d.displayCount, DISP_TIME);
            HAL_TIM_Base_MspInit(&htim4);
            HAL_TIM_Base_Start_IT(&htim4);
            GetClock(&time);
            TRACE_PRINTF("pir interrupt, renew set display 5 minute\r\n");
        }
    }
}

osStatus_t DISP_TaskInit(void)
{
    static osThreadId_t dispTaskId = NULL;

    if (dispTaskId != NULL) {
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

    dispTaskId = osThreadNew(DISP_Task, NULL, &g_dispTaskAttributes);
    if (dispTaskId == NULL) {
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