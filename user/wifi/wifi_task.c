#include "wifi_task.h"
#include <string.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "esp8266_at.h"
#include "time_run.h"
#include "display_task.h"
#include "wifi_uart_if.h"
#include "trace.h"

#define LOG_TAG "wifi_task"

#define WIFI_TASK_EVENT_ALL (0x00ffffff)

#define WIFI_TASK_SEND_MSG_MAX     1
#define WIFI_TASK_SEND_MSG_SIZE    (sizeof(struct TimeType))
#define WIFI_TASK_RECEIVE_MSG_MAX  1
#define WIFI_TASK_RECEIVE_MSG_SIZE (200)

#define WIFI_TASK_NAME       "wifiTask"
#define WIFI_TASK_STACK_SIZE (128 * 8)
#define WIFI_TASK_PRIORITY   (osPriority_t) osPriorityNormal

const osThreadAttr_t g_wifiTaskAttributes = {
    .name       = WIFI_TASK_NAME,
    .stack_size = WIFI_TASK_STACK_SIZE,
    .priority   = WIFI_TASK_PRIORITY,
};

static osThreadId_t g_wifiTaskId = NULL;
static osEventFlagsId_t g_wifiEvent = NULL;
static osMessageQueueId_t g_wifiSendMsgId = NULL;
static osMessageQueueId_t g_wifiReceiveMsgId = NULL;
static osTimerId_t g_wifiTimerId = NULL;

static void WIFI_TimerCallback(void *arg)
{
    WIFI_TaskSetEvent(WIFI_TASK_EVENT_SEND_CMD);
}

static void WIFI_Task(void *argument)
{
    uint32_t event;
    enum WifiReceiveInfo info;
    struct Esp8266GetTimeType wifi;
    uint8_t *buffer = NULL;
    osStatus_t ret;

    LOGI(LOG_TAG, "wifi task enter!\r\n");
    WIFI_TaskSetEvent(WIFI_TASK_EVENT_POWER_ON);

    while (1) {
        event = osEventFlagsWait(g_wifiEvent, WIFI_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & WIFI_TASK_EVENT_SEND_CMD) == WIFI_TASK_EVENT_SEND_CMD) {
            WIFI_SendCommand(&wifi);
        }
        if ((event & WIFI_TASK_EVENT_GET_TIME_DATA) == WIFI_TASK_EVENT_GET_TIME_DATA) {
            buffer = pvPortMalloc(WIFI_TASK_RECEIVE_MSG_SIZE);
            osMessageQueueGet(g_wifiReceiveMsgId, buffer, NULL, 0);
            osMessageQueueReset(g_wifiReceiveMsgId);
            info = WIFI_ReceiveProcess(&wifi, buffer);
            if (info == WIFI_GET_TIME_COMPLETE) {
                if (osMessageQueuePut(g_wifiSendMsgId, &wifi.time, 0, 0) == osOK) {
                    DISP_TaskSetEvent(DISP_TASK_EVENT_GET_TIME_DATA);
                }
                SetClock(&wifi.time);
                osTimerStop(g_wifiTimerId);
            }
            vPortFree(buffer);
        }
        if ((event & WIFI_TASK_EVENT_POWER_ON) == WIFI_TASK_EVENT_POWER_ON) {
            memset(&wifi, 0, sizeof(struct Esp8266GetTimeType));
            WIFI_ReceiveDmaInit();
            WIFI_Power(&wifi, WIFI_POWER_ON);
            ret = osTimerStart(g_wifiTimerId, 1000);
            if (ret != osOK) {
                LOGE(LOG_TAG, "timer start error!, ret: %d\r\n", ret);
            }
        }
    }
}

osStatus_t WIFI_TaskInit(void)
{
    if (g_wifiTaskId != NULL) {
        return osError;
    }

    g_wifiEvent = osEventFlagsNew(NULL);
    if (g_wifiEvent == NULL) {
        return osError;
    }

    g_wifiSendMsgId = osMessageQueueNew(WIFI_TASK_SEND_MSG_MAX, WIFI_TASK_SEND_MSG_SIZE, NULL);
    if (g_wifiSendMsgId == NULL) {
        return osError;
    }

    g_wifiReceiveMsgId = osMessageQueueNew(WIFI_TASK_RECEIVE_MSG_MAX, WIFI_TASK_RECEIVE_MSG_SIZE, NULL);
    if (g_wifiReceiveMsgId == NULL) {
        return osError;
    }

    g_wifiTimerId = osTimerNew(WIFI_TimerCallback, osTimerPeriodic, NULL, NULL);
    if (g_wifiTimerId == NULL) {
        return osError;
    }

    g_wifiTaskId = osThreadNew(WIFI_Task, NULL, &g_wifiTaskAttributes);
    if (g_wifiTaskId == NULL) {
        return osError;
    }

    return osOK;
}

osStatus_t WIFI_TaskGetTimeData(struct TimeType *time)
{
    osStatus_t status;

    status = osMessageQueueGet(g_wifiSendMsgId, time, NULL, 0);

    return status;
}

osStatus_t WIFI_TaskSendBuffer(uint8_t *buffer)
{
    osStatus_t status;

    status = osMessageQueuePut(g_wifiReceiveMsgId, buffer, 0, 0);
    if (status == osOK) {
        WIFI_TaskSetEvent(WIFI_TASK_EVENT_GET_TIME_DATA);
    }

    return status;
}

void WIFI_TaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_wifiEvent, event);
}

void WIFI_TaskSuspend(void)
{
    osStatus_t ret;

    ret = osThreadSuspend(g_wifiTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "wifi task suspend\r\n");
    } else {
        LOGI(LOG_TAG, "wifi task suspend\r\n");
    }
}

void WIFI_TaskResume(void)
{
    osStatus_t ret;

    ret = osThreadResume(g_wifiTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "wifi task resume\r\n");
    } else {
        LOGI(LOG_TAG, "wifi task resume\r\n");
        WIFI_TaskSetEvent(WIFI_TASK_EVENT_POWER_ON);
    }
}
