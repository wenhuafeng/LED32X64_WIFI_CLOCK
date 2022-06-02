#include "temp_humi_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "htu21d.h"
#include "display_task.h"
#include "trace.h"

#define LOG_TAG "temp_humi_task"

#define TH_TASK_EVENT_ALL    (0x00ffffff)
#define TH_TASK_EVENT_START  (1 << 0)
#define TH_TASK_EVENT_GET_TH (1 << 1)

#define TH_TASK_GET_TH_TIME 10000

#define TH_TASK_MSG_MAX  1
#define TH_TASK_MSG_SIZE (sizeof(struct Htu21dDataType))

#define TH_TASK_NAME       "thTask"
#define TH_TASK_STACK_SIZE (128 * 8)
#define TH_TASK_PRIORITY   (osPriority_t) osPriorityNormal

const osThreadAttr_t g_thTaskAttributes = {
    .name       = TH_TASK_NAME,
    .stack_size = TH_TASK_STACK_SIZE,
    .priority   = TH_TASK_PRIORITY,
};

static osEventFlagsId_t g_thEvent   = NULL;
static osMessageQueueId_t g_thMsgId = NULL;
static osTimerId_t g_thTimerId      = NULL;
static osThreadId_t g_thTaskId      = NULL;

static void TH_TimerCallback(void *arg)
{
    TH_TaskSetEvent(TH_TASK_EVENT_GET_TH);
}

static void TH_Task(void *argument)
{
    uint32_t event;
    struct Htu21dDataType th;
    osStatus_t ret;

    LOGI(LOG_TAG, "temp humi task enter\r\n");
    TH_TaskSetEvent(TH_TASK_EVENT_START);

    while (1) {
        event = osEventFlagsWait(g_thEvent, TH_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & TH_TASK_EVENT_GET_TH) == TH_TASK_EVENT_GET_TH) {
            HTU21D_GetData(&th);
            if (osMessageQueuePut(g_thMsgId, &th, 0, 0) == osOK) {
                DISP_TaskSetEvent(DISP_TASK_EVENT_GET_TH_DATA);
            }
        }
        if ((event & TH_TASK_EVENT_START) == TH_TASK_EVENT_START) {
            HTU21D_Init();
            ret = osTimerStart(g_thTimerId, TH_TASK_GET_TH_TIME);
            if (ret != osOK) {
                LOGE(LOG_TAG, "temp and humi timer start error!, ret: %d\r\n", ret);
            }
            TH_TaskSetEvent(TH_TASK_EVENT_GET_TH);
        }
    }
}

osStatus_t TH_TaskInit(void)
{
    if (g_thTaskId != NULL) {
        return osError;
    }

    g_thEvent = osEventFlagsNew(NULL);
    if (g_thEvent == NULL) {
        return osError;
    }

    g_thMsgId = osMessageQueueNew(TH_TASK_MSG_MAX, TH_TASK_MSG_SIZE, NULL);
    if (g_thMsgId == NULL) {
        return osError;
    }

    g_thTimerId = osTimerNew(TH_TimerCallback, osTimerPeriodic, NULL, NULL);
    if (g_thTimerId == NULL) {
        return osError;
    }

    g_thTaskId = osThreadNew(TH_Task, NULL, &g_thTaskAttributes);
    if (g_thTaskId == NULL) {
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

void TH_TaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_thEvent, event);
}

void TH_TaskSuspend(void)
{
    osStatus_t ret;

    ret = osTimerStop(g_thTimerId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "temp and humi timer stop\r\n");
    } else {
        LOGI(LOG_TAG, "temp and humi timer stop\r\n");
    }

    ret = osThreadSuspend(g_thTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "temp and humi task suspend\r\n");
    } else {
        LOGI(LOG_TAG, "temp and humi task suspend\r\n");
    }
}

void TH_TaskResume(void)
{
    osStatus_t ret;

    ret = osThreadResume(g_thTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "temp and humi task resume\r\n");
    } else {
        LOGI(LOG_TAG, "temp and humi task resume\r\n");
    }

    TH_TaskSetEvent(TH_TASK_EVENT_START);
}
