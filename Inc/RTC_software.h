#ifndef RTC_H
#define RTC_H

#include <stdio.h>
#include "TypeDefine.h"

//whf add
#define _DISP_TIME_1S_ (1 * 2)
#define _DISP_TIME_2S_ (2 * 2)
#define _DISP_TIME_3S_ (3 * 2)
#define _DISP_TIME_4S_ (4 * 2)
#define _DISP_TIME_5S_ (5 * 2)
#define _DISP_TIME_7S_ (7 * 2)
#define _DISP_TIME_10S_ (10 * 2)
#define _DISP_TIME_15S_ (15 * 2)

typedef struct {
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;
    u8 week;
    u8 month;
    u16 year;
} rtc_counter_value_t;

extern rtc_counter_value_t TIME;

extern BOOLEAN F_500MS;
extern BOOLEAN F_COL;
extern BOOLEAN F_SET_COL;
extern BOOLEAN F_1000MS;
extern BOOLEAN F_1MIN;

extern BOOLEAN F_auto_time;
extern BOOLEAN F_24hourRDSUpdateSuccee;

//RTC define
extern rtc_counter_value_t TIME;

//==============================================================================
//void RTC_Init();
void RTC_Time_Init(rtc_counter_value_t *time);
void RTC_Time_Deal(rtc_counter_value_t *time);
u8 Date_Day(u16 Year, u8 Month);
#if 0
u8 Week_Deal(u16 Year,u8 Month,u8 Day);
#else
void Week_Deal(rtc_counter_value_t *time);
#endif

u8 MIN_INC(void);
u8 MIN_DEC(void);
u8 HOUR_INC(void);
u8 HOUR_DEC(void);
u8 DAY_INC(void);
u8 DAY_DEC(void);
u8 MONTH_INC(void);
u8 MONTH_DEC(void);
u8 YEAR_INC(void);
u8 YEAR_DEC(void);

u8 RTC_Set(u16 syear, u8 smon, u8 sday, u8 hour, u8 min, u8 sec);
u8 RTC_Get(void);
u8 RTC_Get_Week(u16 year, u8 month, u8 day);

#endif
