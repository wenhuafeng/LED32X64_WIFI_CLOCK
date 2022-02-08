#ifndef LUNAR_CALENDAR_H
#define LUNAR_CALENDAR_H

#include "type_define.h"
#include "time.h"

struct LunarCalendarType {
    u8 year;
    u8 month;
    u8 day;
};

void CalculationLunarCalendar(struct TimeType *time);
struct LunarCalendarType *GetLunarCalendar(void);

#endif
