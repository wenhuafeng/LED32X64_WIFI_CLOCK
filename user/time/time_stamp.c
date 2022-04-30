#include "main.h"
#if (defined(UNIX_TIME_STAMP) && UNIX_TIME_STAMP)
#include "time_stamp.h"
#include <stdint.h>
#include <time.h>
#include "time_run.h"
#include "trace.h"

#define TIME_STAMP_OFFSET     (8 * 60 * 60) /* 8 hour */
#define TIME_STAMP_START_YEAR 1900

time_t g_timestamp;

struct TimeType TimestampConvertTime(time_t timestamp)
{
    struct TimeType time;
    struct tm *t;

    timestamp += TIME_STAMP_OFFSET;
    t = localtime(&timestamp);

    time.year   = t->tm_year + TIME_STAMP_START_YEAR;
    time.month  = t->tm_mon + 1;
    time.day    = t->tm_mday;
    time.hour   = t->tm_hour;
    time.minute = t->tm_min;
    time.second = t->tm_sec;

    return time;
}

void TimeConvertTimestamp(struct TimeType *time)
{
    struct tm t;

    t.tm_year   = time->year - TIME_STAMP_START_YEAR;
    t.tm_mon    = time->month - 1;
    t.tm_mday   = time->day;
    t.tm_hour   = time->hour;
    t.tm_min    = time->minute;
    t.tm_sec    = time->second;
    g_timestamp = mktime(&t) - TIME_STAMP_OFFSET;
}

void TimestampAdd(void)
{
    //g_timestamp++;
    //LOGI(LOG_TAG, "timestamp: %d\r\n", g_timestamp);
}

#else

void TimeConvertTimestamp(struct TimeType *time)
{
}
void TimestampAdd(void)
{
}

#endif
