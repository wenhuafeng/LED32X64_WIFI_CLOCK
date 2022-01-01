#ifndef TIME_H
#define TIME_H

#include <stdbool.h>
#include "type_define.h"

struct TimeType {
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;
    u8 week;
    u8 month;
    u16 year;
};

bool Get1sFlag(void);
void Set1sFlag(bool flag);
struct TimeType *GetTimeData(void);
void CalculateWeek(u16 year, u8 month, u8 day, u8 *week);
bool ClockRun(void);
void GetClock(void);
void SetClock(struct TimeType *time);

#endif
