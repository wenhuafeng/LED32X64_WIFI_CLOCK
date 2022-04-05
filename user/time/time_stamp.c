#include "main.h"
#if (defined(UNIX_TIME_STAMP) && UNIX_TIME_STAMP)
#include "time_stamp.h"
#include <stdint.h>
#include <time.h>
#include "time_run.h"
#include "trace_printf.h"

#define TIME_STAMP_OFFSET     (8 * 60 * 60) /* 8 hour */
#define TIME_STAMP_START_YEAR 1900

time_t g_timestamp;

struct TimeType TimestampConvertTime(time_t timestamp)
{
    struct TimeType time;
    struct tm *t;

    timestamp += TIME_STAMP_OFFSET;
    t = localtime(&timestamp);

    time.year = t->tm_year + TIME_STAMP_START_YEAR;
    time.month = t->tm_mon + 1;
    time.day = t->tm_mday;
    time.hour = t->tm_hour;
    time.min = t->tm_min;
    time.sec = t->tm_sec;

    return time;
}

void TimeConvertTimestamp(struct TimeType *time)
{
    struct tm t;

    t.tm_year = time->year - TIME_STAMP_START_YEAR;
    t.tm_mon = time->month - 1;
    t.tm_mday = time->day;
    t.tm_hour = time->hour;
    t.tm_min = time->min;
    t.tm_sec = time->sec;
    g_timestamp = mktime(&t) - TIME_STAMP_OFFSET;
}

void TimestampAdd(void)
{
    g_timestamp++;
    TRACE_PRINTF("timestamp: %d\r\n", g_timestamp);
}

#else

void TimeConvertTimestamp(struct TimeType *time) {}
void TimestampAdd(void) {}

#endif
