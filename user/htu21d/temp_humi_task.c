#include "temp_humi_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "htu21d.h"
#include "display_task.h"

#define TH_TASK_MSG_MAX  1
#define TH_TASK_MSG_SIZE (sizeof(struct Htu21dDataType))

#define TH_TASK_NAME       "thTask"
#define TH_TASK_STACK_SIZE (128 * 4)
#define TH_TASK_PRIORITY   (osPriority_t)osPriorityNormal

const osThreadAttr_t g_thTaskAttributes = {
    .name       = TH_TASK_NAME,
    .stack_size = TH_TASK_STACK_SIZE,
    .priority   = TH_TASK_PRIORITY,
};

osMessageQueueId_t g_thMsgId;

static void TH_Task(void *argument)
{
    struct Htu21dDataType th;

    HTU21D_Init();

    while (1) {
        HTU21D_GetData(&th);
        if (osMessageQueuePut(g_thMsgId, &th, 0, 0) == osOK) {
            DISP_TaskSetEvent(DISP_TASK_EVENT_GET_TH_DATA);
        }

        osDelay(10000);
    }
}

osStatus_t TH_TaskInit(void)
{
    static osThreadId_t thTaskId = NULL;

    if (thTaskId != NULL) {
        return osError;
    }

    g_thMsgId = osMessageQueueNew(TH_TASK_MSG_MAX, TH_TASK_MSG_SIZE, NULL);
    if (g_thMsgId == NULL) {
        return osError;
    }

    thTaskId = osThreadNew(TH_Task, NULL, &g_thTaskAttributes);
    if (thTaskId == NULL) {
        return osError;
    }

    return osOK;
}

osStatus_t TH_TaskGetThData(struct Htu21dDataType *th)
{
    osStatus_t status;

    status = osMessageQueueGet(g_thMsgId, th, NULL, 0);

    return status;
}
