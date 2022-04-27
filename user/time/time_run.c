#include "time_run.h"
#include <stdbool.h>
#include <stdio.h>
#include "common.h"
#include "rtc.h"
#include "hub75d.h"
#include "lunar_calendar.h"
#include "trace.h"
#include "time_stamp.h"

static uint8_t GetMaxDay(uint16_t year, uint8_t month)
{
    uint8_t day;
    uint8_t daysTable[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2) {
        if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) {
            day = 29;
        } else {
            day = 28;
        }
    } else {
        day = daysTable[month];
    }

    return day;
}

static void CalculateWeek(struct TimeType *time)
{
    int16_t yearTemp = 0;
    int16_t yearHigh;
    int16_t yearLow;
    int8_t monthTemp = 0;
    int8_t wk;

    if (time->month < 3) {
        monthTemp = time->month + 12;
        yearTemp  = time->year - 1;
    } else {
        monthTemp = time->month;
        yearTemp  = time->year;
    }

    yearHigh = yearTemp / 100;
    yearLow  = yearTemp % 100;

    wk = yearLow + (yearLow / 4) + (yearHigh / 4);
    wk = wk - (2 * yearHigh) + (26 * (monthTemp + 1) / 10) + time->day - 1;
    wk = (wk + 140) % 7;

    time->week = wk;
}

bool ClockRun(struct TimeType *time)
{
    time->second++;
    if (time->second < 60) {
        return false;
    }

    time->second = 0;
    time->minute++;
    if (time->minute < 60) {
        return false;
    }

    time->minute = 0;
    time->hour++;
    if (time->hour < 24) {
        return false;
    }

    time->hour = 0;
    time->day++;
    if (time->day <= GetMaxDay(time->year, time->month)) {
        goto calc_week;
    }

    time->day = 1;
    time->month++;
    if (time->month < 13) {
        goto calc_week;
    }

    time->month = 1;
    time->year++;

calc_week:
    CalculateWeek(time);

    return true;
}

void GetClock(struct TimeType *time)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    time->second = sTime.Seconds;
    time->minute = sTime.Minutes;
    time->hour   = sTime.Hours;
    time->day    = sDate.Date;
    time->month  = sDate.Month;
    time->week   = sDate.WeekDay;
    time->year   = (sDate.Year + 2000);

    //HUB75D_GetCalendar(&g_time);
    //CalculationLunarCalendar(&g_time);
    //TimeConvertTimestamp(&g_time);

    TRACE_PRINTF("get time: %d-%d-%d %02d:%02d:%02d \r\n", time->year, time->month, time->day, time->hour, time->min,
                 time->sec);
}

void SetClock(struct TimeType *time)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    sTime.Seconds = time->second;
    sTime.Minutes = time->minute;
    sTime.Hours   = time->hour;

    sDate.Date    = time->day;
    sDate.Month   = time->month;
    sDate.WeekDay = time->week;
    sDate.Year    = (time->year % 100);

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    TRACE_PRINTF("set time: %d-%d-%d %02d:%02d:%02d \r\n", time->year, time->month, time->day, time->hour, time->min,
                 time->sec);
}
