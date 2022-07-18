#ifndef OLED_TASK_H
#define OLED_TASK_H

#include <stdint.h>
#include "cmsis_os2.h"
#include "lunar_calendar.h"
#include "hub75d.h"
#include "htu21d.h"

#define DISP_OLED_TASK_EVENT_RECEIVED_NEW_DATA (1 << 0)

struct OledType {
    struct LunarCalendarType lunarCalendar;
    //struct CalendarDecimal calendarDecimal;
    struct TimeType time;
    struct Htu21dDataType tempHumi;
};

osStatus_t OLED_TaskInit(void);
void OLED_TaskSetEvent(uint32_t event);
void OLED_TaskSuspend(void);
void OLED_TaskResume(void);

#endif
