#ifndef LUNAR_CALENDAR_H
#define LUNAR_CALENDAR_H

#include <stdint.h>
#include "time_run.h"

struct LunarCalendarType {
    uint8_t year;
    uint8_t month;
    uint8_t day;
};

void CalculationLunarCalendar(struct TimeType *time);
struct LunarCalendarType *GetLunarCalendar(void);

#endif
