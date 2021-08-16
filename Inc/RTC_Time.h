#ifndef RTC_TIME_H
#define RTC_TIME_H

#include "stm32f10x.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

void RTC_Init(void);
void Time_Display(void);
void Time_Regulate(void);
void Time_SetCalendarTime(struct tm t);
struct tm Time_GetCalendarTime(void);
void RTC_Init_1(void);

extern FunctionalState TimeDisplay;

#endif
