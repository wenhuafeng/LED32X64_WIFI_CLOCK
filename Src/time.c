#include "time.h"
#include <stdbool.h>
#include <stdio.h>
#include "rtc.h"
#include "hub75d.h"
#include "lunar_calendar.h"

struct TimeType g_time;
bool g_1sFlag;

static u8 GetMaxDay(u16 year, u8 month)
{
    u8 day;
    const u8 constDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2) {
        if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) {
            day = 29;
        } else {
            day = 28;
        }
    } else {
        day = constDays[month];
    }

    return day;
}

bool Get1sFlag(void)
{
    return g_1sFlag;
}

void Set1sFlag(bool flag)
{
    g_1sFlag = true;
}

struct TimeType *GetTimeData(void)
{
    return &g_time;
}

void CalculateWeek(u16 year, u8 month, u8 day, u8 *week)
{
    s16 yearTemp = 0;
    s16 yearHigh;
    s16 yearLow;
    s8 monthTemp = 0;
    s8 wk;

    if (month < 3) {
        monthTemp = month + 12;
        yearTemp = year - 1;
    } else {
        monthTemp = month;
        yearTemp = year;
    }

    yearHigh = yearTemp / 100;
    yearLow = yearTemp % 100;

    wk = yearLow + (yearLow / 4) + (yearHigh / 4);
    wk = wk - (2 * yearHigh) + (26 * (monthTemp + 1) / 10) + day - 1;
    wk = (wk + 140) % 7;

    *week = wk;
}

bool ClockRun(void)
{
    g_time.sec++;
    if (g_time.sec < 60) {
        return false;
    }

    g_time.sec = 0;
    g_time.min++;
    if (g_time.min < 60) {
        return false;
    }

    g_time.min = 0;
    g_time.hour++;
    if (g_time.hour < 24) {
        return false;
    }

    g_time.hour = 0;
    g_time.day++;
    if (g_time.day <= GetMaxDay(g_time.year, g_time.month)) {
        goto calc_week;
    }

    g_time.day = 1;
    g_time.month++;
    if (g_time.month < 13) {
        goto calc_week;
    }

    g_time.month = 1;
    g_time.year++;

calc_week:
    CalculateWeek(g_time.year, g_time.month, g_time.day, &g_time.week);
    return true;
}

void GetClock(void)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    g_time.sec = sTime.Seconds;
    g_time.min = sTime.Minutes;
    g_time.hour = sTime.Hours;

    g_time.day = sDate.Date;
    g_time.month = sDate.Month;
    g_time.week = sDate.WeekDay;
    g_time.year = (sDate.Year + 2000);

    HUB75D_CalculateCalendar(&g_time);
    CalculationLunarCalendar(&g_time);

    printf("\r\nTime: %d-%d-%d %02d:%02d:%02d \r\n",
           g_time.year, g_time.month, g_time.day,
           g_time.hour, g_time.min, g_time.sec);
}

void SetClock(struct TimeType *time)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    sTime.Seconds = time->sec;
    sTime.Minutes = time->min;
    sTime.Hours = time->hour;

    sDate.Date = time->day;
    sDate.Month = time->month;
    sDate.WeekDay = time->week;
    sDate.Year = (time->year % 100);

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    printf("\r\nTime: %d-%d-%d   %02d:%02d:%02d \r\n",
           time->year, time->month, time->day,
           time->hour, time->min, time->sec);
}
