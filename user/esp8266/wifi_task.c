#include "wifi_task.h"
#include <string.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "wifi_uart_if.h"
#include "esp8266_at.h"
#include "time_run.h"
#include "display_task.h"

#define WIFI_TASK_EVENT_ALL (0xffffffff)

#define WIFI_RX_BUFFER_SIZE     200
#define WIFI_TASK_SEND_MSG_MAX  1
#define WIFI_TASK_SEND_MSG_SIZE (sizeof(struct Esp8266GetTimeType))

#define WIFI_TASK_NAME       "wifiTask"
#define WIFI_TASK_STACK_SIZE (128 * 4)
#define WIFI_TASK_PRIORITY   (osPriority_t) osPriorityNormal

const osThreadAttr_t g_wifiTaskAttributes = {
    .name       = WIFI_TASK_NAME,
    .stack_size = WIFI_TASK_STACK_SIZE,
    .priority   = WIFI_TASK_PRIORITY,
};

osEventFlagsId_t g_wifiEvent;
osMessageQueueId_t g_wifiSendMsgId;
osMessageQueueId_t g_wifiReceiveMsgId;
osTimerId_t g_wifiTimerId;

static void WIFI_TimerCallback(void *arg)
{
    DISP_TaskSetEvent(WIFI_TASK_EVENT_SEND_CMD);
}

static void WIFI_Task(void *argument)
{
    uint32_t event;
    struct Esp8266GetTimeType wifi;
    uint8_t *buffer = NULL;

    WIFI_TaskSetEvent(WIFI_TASK_EVENT_POWER_ON);

    while (1) {
        event = osEventFlagsWait(g_wifiEvent, WIFI_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & WIFI_TASK_EVENT_SEND_CMD) == WIFI_TASK_EVENT_SEND_CMD) {
            WIFI_SendCommand(&wifi);
        }
        if ((event & WIFI_TASK_EVENT_GET_TIME_DATA) == WIFI_TASK_EVENT_GET_TIME_DATA) {
            buffer = pvPortMalloc(200);
            osMessageQueueGet(g_wifiReceiveMsgId, buffer, NULL, 0);
            WIFI_ReceiveProcess(&wifi, buffer);
            if (wifi.rxInfoCtr == WIFI_GET_TIME_COMPLETE) {
                if (osMessageQueuePut(g_wifiSendMsgId, &wifi.time, 0, 0) == osOK) {
                    DISP_TaskSetEvent(WIFI_TASK_EVENT_SEND_TIME_DATA);
                }
                osTimerStop(g_wifiTimerId);
            }
            vPortFree(buffer);
        }
        if ((event & WIFI_TASK_EVENT_POWER_ON) == WIFI_TASK_EVENT_POWER_ON) {
            memset(&wifi, 0, sizeof(struct Esp8266GetTimeType));
            WIFI_ReceiveDmaInit();
            WIFI_Power(&wifi, POWER_ON);
            if (osTimerStart(g_wifiTimerId, 1000) != osOK) {
                ;
            }
        }
    }
}

osStatus_t WIFI_TaskInit(void)
{
    static osThreadId_t wifiTaskId = NULL;

    if (wifiTaskId != NULL) {
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

    g_wifiTimerId = osTimerNew(WIFI_TimerCallback, osTimerPeriodic, NULL, NULL);
    if (g_wifiTimerId == NULL) {
        return osError;
    }

    wifiTaskId = osThreadNew(WIFI_Task, NULL, &g_wifiTaskAttributes);
    if (wifiTaskId == NULL) {
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
