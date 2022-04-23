#include "display_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "hub75d.h"
#include "time_run.h"

#define DISP_TASK_EVENT_ALL (0xffffffff)

#define DISP_TASK_NAME       "dispTask"
#define DISP_TASK_STACK_SIZE (128 * 4)
#define DISP_TASK_PRIORITY   (osPriority_t)osPriorityNormal7

const osThreadAttr_t g_dispTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

osEventFlagsId_t g_dispEvent;

static void DISP_Task(void *argument)
{
    uint32_t event;
    struct Hub75dType hub75d;
    struct TimeType time;

    HUB75D_Init();
    HUB75D_Disp(&hub75d.displayCount, DISP_TIME);
    HAL_TIM_Base_MspInit(&htim4);
    HAL_TIM_Base_Start_IT(&htim4);

    while (1) {
        event = osEventFlagsWait(g_dispEvent, DISP_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_TASK_EVENT_SCAN_LED) == DISP_TASK_EVENT_SCAN_LED) {
            HUB75D_DispScan(&hub75d.rgbScan);
        }
        if ((event & DISP_TASK_EVENT_GET_SCAN_RGB) == DISP_TASK_EVENT_GET_SCAN_RGB) {
            HUB75D_GetCalendar(&hub75d.calendarDecimal, &time);
            if (HUB75D_CtrDec(&hub75d) == true) {
                HAL_TIM_Base_Stop_IT(&htim4);
                HAL_TIM_Base_MspDeInit(&htim4);
                HUB75D_Disp(&hub75d.displayCount, DISP_TIME_OFF);
            }
            HUB75D_GetScanRgb(&hub75d);
        }
        if ((event & DISP_TASK_EVENT_DISP_ON) == DISP_TASK_EVENT_DISP_ON) {
            HUB75D_Init();
            HUB75D_Disp(&hub75d.displayCount, DISP_TIME);
            HAL_TIM_Base_MspInit(&htim4);
            HAL_TIM_Base_Start_IT(&htim4);
            //TRACE_PRINTF("pir interrupt, renew set display 5 minute\r\n");
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
