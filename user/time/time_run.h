#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

struct TimeType {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t week;
    uint8_t month;
    uint16_t year;
};

void CalculateWeek(struct TimeType *time);
bool ClockRun(struct TimeType *time);
void GetClock(struct TimeType *time);
void SetClock(struct TimeType *time);

#endif
