#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <time.h>
#include "time_run.h"

void TimestampConvertTime(time_t timestamp, struct TimeType *time);
void TimestampAdd(void);

#endif
