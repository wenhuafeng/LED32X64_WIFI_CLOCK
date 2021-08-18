#ifndef RTC_TIME_H
#define RTC_TIME_H

#include <time.h>
#include <stdio.h>
#include <string.h>

void RTC_Init(void);
void Time_Display(void);
void Time_Regulate(void);
void Time_SetCalendarTime(struct tm t);
struct tm Time_GetCalendarTime(void);


#endif
