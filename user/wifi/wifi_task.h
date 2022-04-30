#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <stdint.h>
#include "cmsis_os2.h"
#include "time_run.h"

#define WIFI_TASK_EVENT_POWER_ON       (1 << 0)
#define WIFI_TASK_EVENT_SEND_CMD       (1 << 1)
#define WIFI_TASK_EVENT_GET_TIME_DATA  (1 << 2)
//#define WIFI_TASK_EVENT_SEND_TIME_DATA (1 << 3)

osStatus_t WIFI_TaskInit(void);
osStatus_t WIFI_TaskGetTimeData(struct TimeType *time);
osStatus_t WIFI_TaskSendBuffer(uint8_t *buffer);
void WIFI_TaskSetEvent(uint32_t event);

#endif
