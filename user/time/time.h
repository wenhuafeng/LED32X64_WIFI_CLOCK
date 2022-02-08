#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

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
void SetTimeData(struct TimeType *time);
void CalculateWeek(uint16_t year, uint8_t month, uint8_t day, uint8_t *week);
bool ClockRun(void);
void GetClock(void);
void SetClock(struct TimeType *time);

#endif
