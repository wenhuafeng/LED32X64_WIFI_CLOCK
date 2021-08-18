#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>
#include "type_define.h"

struct TimeType {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t week;
    uint8_t month;
    uint16_t year;
};

bool Get1sFlag(void);
void Set1sFlag(bool flag);
struct TimeType *GetTimeData(void);
void CalculateWeek(u16 year, u8 month, u8 day, u8 *week);
bool ClockRun(void);
void GetClock(void);
void SetClock(struct TimeType *time);

#endif
