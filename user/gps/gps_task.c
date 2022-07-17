#include "main.h"
#if defined(GPS_GET_TIME) && GPS_GET_TIME
#include "gps_task.h"
#include <string.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "time_run.h"
#include "display_task.h"
#include "gps_uart_if.h"
#include "minmea.h"
#include "time_stamp.h"
#include "trace.h"

#define LOG_TAG "gps_task"

enum {
    GPS_POWER_OFF,
    GPS_POWER_ON,
};

#define GPS_LED_TOGGLE() HAL_GPIO_TogglePin(GPS_LED_GPIO_Port, GPS_LED_Pin)
#define GPS_LED_OFF()    HAL_GPIO_WritePin(GPS_LED_GPIO_Port, GPS_LED_Pin, GPIO_PIN_RESET)
#define GPS_POWER(x)                                                                                 \
    ((x == GPS_POWER_ON) ? (HAL_GPIO_WritePin(GPS_ENABLE_GPIO_Port, GPS_ENABLE_Pin, GPIO_PIN_SET)) : \
                           (HAL_GPIO_WritePin(GPS_ENABLE_GPIO_Port, GPS_ENABLE_Pin, GPIO_PIN_RESET)))

#define GPS_TASK_EVENT_ALL          (0x00ffffff)
#define GPS_TASK_EVENT_POWER_ON     (1 << 0)
#define GPS_TASK_EVENT_RECEIVE_DATA (1 << 1)

#define GPS_TASK_LED_FLASHING_TIME (100)
#define CHINA_TIME_ZONE            (8 * 60 * 60)

#define GPS_TASK_SEND_MSG_MAX     1
#define GPS_TASK_SEND_MSG_SIZE    (sizeof(struct TimeType))
#define GPS_TASK_RECEIVE_MSG_MAX  1
#define GPS_TASK_RECEIVE_MSG_SIZE 512

#define GPS_TASK_NAME       "gpsTask"
#define GPS_TASK_STACK_SIZE (128 * 11)
#define GPS_TASK_PRIORITY   (osPriority_t) osPriorityNormal2

const osThreadAttr_t g_gpsTaskAttributes = {
    .name       = GPS_TASK_NAME,
    .stack_size = GPS_TASK_STACK_SIZE,
    .priority   = GPS_TASK_PRIORITY,
};

static osThreadId_t g_gpsTaskId             = NULL;
static osEventFlagsId_t g_gpsEvent          = NULL;
static osMessageQueueId_t g_gpsSendMsgId    = NULL;
static osMessageQueueId_t g_gpsReceiveMsgId = NULL;
static osTimerId_t g_gpsTimerId             = NULL;

static void GPS_TimerCallback(void *arg)
{
    GPS_LED_TOGGLE();
}

static void GPS_Task(void *argument)
{
    uint32_t event;
    uint8_t *buffer = NULL;
    struct minmea_sentence_rmc rmc;
    struct timespec ts;
    struct TimeType time;
    osStatus_t ret;

    LOGI(LOG_TAG, "gps task enter!\r\n");
    GPS_TaskSetEvent(GPS_TASK_EVENT_POWER_ON);

    while (1) {
        event = osEventFlagsWait(g_gpsEvent, GPS_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & GPS_TASK_EVENT_RECEIVE_DATA) == GPS_TASK_EVENT_RECEIVE_DATA) {
            buffer = pvPortMalloc(GPS_TASK_RECEIVE_MSG_SIZE);
            if (buffer == NULL) {
                LOGI(LOG_TAG, "malloc error!\r\n");
                vPortFree(buffer);
                continue;
            }
            osMessageQueueGet(g_gpsReceiveMsgId, buffer, NULL, 0);
            osMessageQueueReset(g_gpsReceiveMsgId);
            if (minmea_parse_rmc(&rmc, (char *)buffer) == false) {
                LOGI(LOG_TAG, "parse rmc fail!\r\n");
                vPortFree(buffer);
                continue;
            }
            if (minmea_gettime(&ts, &rmc.date, &rmc.time) == -1) {
                LOGI(LOG_TAG, "minmea_gettime error!\r\n");
                vPortFree(buffer);
                continue;
            }

            // ts.tv_sec += CHINA_TIME_ZONE;
            TimestampConvertTime(ts.tv_sec, &time);
            CalculateWeek(&time);
            SetClock(&time);
            if (osMessageQueuePut(g_gpsSendMsgId, &time, 0, 0) == osOK) {
                DISP_TaskSetEvent(DISP_TASK_EVENT_GET_TIME_DATA);
            }

            osTimerStop(g_gpsTimerId);
            GPS_POWER(GPS_POWER_OFF);
            GPS_LED_OFF();

            memset(&rmc.date, 0, sizeof(rmc.date));
            memset(&rmc.time, 0, sizeof(rmc.time));
            vPortFree(buffer);
        }
        if ((event & GPS_TASK_EVENT_POWER_ON) == GPS_TASK_EVENT_POWER_ON) {
            GPS_ReceiveDmaInit();
            memset(&rmc, 0, sizeof(struct minmea_sentence_rmc));
            GPS_POWER(GPS_POWER_ON);
            ret = osTimerStart(g_gpsTimerId, GPS_TASK_LED_FLASHING_TIME);
            if (ret != osOK) {
                LOGE(LOG_TAG, "timer start error!, ret: %d\r\n", ret);
            }
        }
    }
}

osStatus_t GPS_TaskInit(void)
{
    if (g_gpsTaskId != NULL) {
        return osError;
    }

    g_gpsEvent = osEventFlagsNew(NULL);
    if (g_gpsEvent == NULL) {
        return osError;
    }

    g_gpsSendMsgId = osMessageQueueNew(GPS_TASK_SEND_MSG_MAX, GPS_TASK_SEND_MSG_SIZE, NULL);
    if (g_gpsSendMsgId == NULL) {
        return osError;
    }

    g_gpsReceiveMsgId = osMessageQueueNew(GPS_TASK_RECEIVE_MSG_MAX, GPS_TASK_RECEIVE_MSG_SIZE, NULL);
    if (g_gpsReceiveMsgId == NULL) {
        return osError;
    }

    g_gpsTimerId = osTimerNew(GPS_TimerCallback, osTimerPeriodic, NULL, NULL);
    if (g_gpsTimerId == NULL) {
        return osError;
    }

    g_gpsTaskId = osThreadNew(GPS_Task, NULL, &g_gpsTaskAttributes);
    if (g_gpsTaskId == NULL) {
        return osError;
    }

    return osOK;
}

osStatus_t GPS_TaskGetTimeData(struct TimeType *time)
{
    osStatus_t status;

    status = osMessageQueueGet(g_gpsSendMsgId, time, NULL, 0);

    return status;
}

osStatus_t GPS_TaskSendBuffer(uint8_t *buffer)
{
    osStatus_t status;

    status = osMessageQueuePut(g_gpsReceiveMsgId, buffer, 0, 0);
    if (status == osOK) {
        GPS_TaskSetEvent(GPS_TASK_EVENT_RECEIVE_DATA);
    }

    return status;
}

void GPS_TaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_gpsEvent, event);
}

void GPS_TaskSuspend(void)
{
    osStatus_t ret;

    ret = osThreadSuspend(g_gpsTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "wifi task suspend\r\n");
    } else {
        LOGI(LOG_TAG, "wifi task suspend\r\n");
    }
}

void GPS_TaskResume(void)
{
    osStatus_t ret;

    ret = osThreadResume(g_gpsTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "wifi task resume\r\n");
    } else {
        LOGI(LOG_TAG, "wifi task resume\r\n");
        GPS_TaskSetEvent(GPS_TASK_EVENT_POWER_ON);
    }
}

#else

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "time_run.h"

osStatus_t GPS_TaskInit(void)
{
    return osOK;
}
osStatus_t GPS_TaskGetTimeData(struct TimeType *time)
{
    (void)time;
    return osOK;
}
osStatus_t GPS_TaskSendBuffer(uint8_t *buffer)
{
    (void)buffer;
    return osOK;
}
void GPS_TaskSetEvent(uint32_t event)
{
    (void)event;
}
void GPS_TaskSuspend(void)
{
}
void GPS_TaskResume(void)
{
}

#endif
