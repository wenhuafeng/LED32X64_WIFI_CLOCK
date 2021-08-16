
//==============================================================================
//==============================================================================
//Description : RTC operation
//Version     : V100
//Date-Time   : 2018-10-27 ~
//Author      : wenhuafeng
//==============================================================================
//==============================================================================

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "TypeDefine.h"
#include "RTC_software.h"
#include "HUB75D.h"

typedef enum { _24HR_, _12HR_ } Time_Format;
typedef enum { _DM_, _MD_ } DM_MD_Format;

//250ms counter
BOOLEAN F_500MS;
BOOLEAN F_COL;
BOOLEAN F_SET_COL;
BOOLEAN F_1000MS;
BOOLEAN F_1MIN;

BOOLEAN F_auto_time;
BOOLEAN F_24hourRDSUpdateSuccee;

//RTC define
rtc_counter_value_t TIME;

//----------------------------------------------------------------------
//Init��year,month,day,hour,min,sec��
void RTC_Time_Init(rtc_counter_value_t *time)
{
    time->year = 2019;
    time->month = 8;
    time->day = 8;
    time->hour = 8;
    time->min = 8;
    time->sec = 0;
    //time->week  = Week_Deal(TIME.year, TIME.month, TIME.day);
    Week_Deal(time);
}

//Time running��hour,min,sec��
void RTC_Time_Deal(rtc_counter_value_t *time)
{
    time->sec++; //sec
    if (time->sec == 60) {
        F_1MIN = 1;
        time->sec = 0;
        time->min++; //min
        if (time->min == 60) {
            time->min = 0; //hour
            time->hour++;
            if (time->hour > 23) {
                time->hour = 0;
                time->day++; //day

                HUB75D_CalcYinli(&TIME);

                if (time->day > Date_Day(time->year, time->month)) {
                    time->day = 1;
                    time->month++; //month
                    if (time->month > 12) {
                        time->month = 1;
                        time->year++; //year
                    }
                }
                //time->week=Week_Deal(time->year,time->month,time->day);//week
                Week_Deal(time);
            }

            //      if ((time->hour == 2) && F_rdstime) {
            //        F_auto_time = 1;
            //      }
            //      if (time->hour == 0 && time->min == 0) {
            //        F_24hourRDSUpdateSuccee = 0;
            //      }
        }
    }
}

u8 Date_Day(u16 Year, u8 Month)
{
    u8 day_temp;
    u8 const_days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (Month == 2) {
        if (((Year % 4 == 0) && (Year % 100 != 0)) || (Year % 400 == 0)) {
            day_temp = 29;
        } else {
            day_temp = 28;
        }
    } else {
        day_temp = const_days[Month];
    }

    return day_temp;
}

#if 0
u8 Week_Deal(u16 Year,u8 Month,u8 Day)
{
  s16 temp_year=0;
  s8 temp_cen=0;
  s8 temp_month=0;
  s8 week_data;

  if (Month < 3) {
    temp_month = Month+12;
    temp_year = Year-1;
  } else {
    temp_month = Month;
    temp_year = Year;
  }

  temp_cen = temp_year / 100;//C;
  temp_year = temp_year % 100;//Y

  week_data = temp_year + temp_year/4 + temp_cen/4;
  week_data = week_data-2*temp_cen + 26*(temp_month+1)/10 + Day-1;
  week_data = (week_data+140)%7;

  return week_data;
}

#else

const u8 WEEK_TAB_ADDR[13] = { 0, 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
//calculate week
void Week_Deal(rtc_counter_value_t *time)
{
    u16 tmp0;
    u8 year;

    year = time->year % 100;

    if (time->month < 3) {
        year -= 1;
    }
    tmp0 = (year + 12) + time->day + ((year + 12) / 4) +
           WEEK_TAB_ADDR[time->month] - 1;
    time->week = (u8)(tmp0 % 7);
}
#endif

#if (KEY_SET)

/********************************************************************/ /**
 * @brief:      MIN++
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 MIN_INC(void)
{
    TIME.sec = 0x00;
    if (++TIME.min >= 60) {
        TIME.min = 0;
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      MIN--
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 MIN_DEC(void)
{
    TIME.sec = 0x00;
    if (--TIME.min == 0xff) {
        TIME.min = 59;
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      HOUR++
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 HOUR_INC(void)
{
    if (++TIME.hour >= 24) {
        TIME.hour = 0;
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      HOUR--
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 HOUR_DEC(void)
{
    if (--TIME.hour == 0xff) {
        TIME.hour = 23;
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      DAY++
 *              and Get max day
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 DAY_INC(void)
{
    if (++TIME.day > Date_Day(TIME.year, TIME.month)) {
        TIME.day = 1;
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      DAY--
 *              and Get max day
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 DAY_DEC(void)
{
    if (--TIME.day == 0) {
        TIME.day = Date_Day(TIME.year, TIME.month);
        return 1;
    } else {
        return 0;
    }
}

/********************************************************************/ /**
 * @brief:      MONTH INC
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 MONTH_INC(void)
{
    BOOLEAN F_tmp;
    INT8U MaxDay;

    if (++TIME.month >= 13) {
        TIME.month = 1;
        F_tmp = 1;
    } else {
        F_tmp = 0;
    }

    //update Max day
    MaxDay = Date_Day(TIME.year, TIME.month);
    if (TIME.day >= MaxDay) {
        TIME.day = MaxDay;
    }

    return F_tmp;
}

/********************************************************************/ /**
 * @brief:      MONTH DEC
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 MONTH_DEC(void)
{
    BOOLEAN F_tmp;
    INT8U MaxDay;

    if (--TIME.month == 0) {
        TIME.month = 12;
        F_tmp = 1;
    } else {
        F_tmp = 0;
    }

    //update Max day
    MaxDay = Date_Day(TIME.year, TIME.month);
    if (TIME.day >= MaxDay) {
        TIME.day = MaxDay;
    }

    return F_tmp;
}

/********************************************************************/ /**
 * @brief:      MONTH INC
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 YEAR_INC(void)
{
    BOOLEAN F_tmp;
    INT8U MaxDay;

    if (++TIME.year > 2099) {
        TIME.year = 2000;
        F_tmp = 1;
    } else {
        F_tmp = 0;
    }

    //update Max day
    MaxDay = Date_Day(TIME.year, TIME.month);
    if (TIME.day >= MaxDay) {
        TIME.day = MaxDay;
    }

    return F_tmp;
}

/********************************************************************/ /**
 * @brief:      MONTH DEC
 *
 * @param[in]:  NONE
 *
 * @return:     NONE
 *********************************************************************/
u8 YEAR_DEC(void)
{
    BOOLEAN F_tmp;
    INT8U MaxDay;

    if (--TIME.year == 1999) {
        TIME.year = 2099;
        F_tmp = 1;
    } else {
        F_tmp = 0;
    }

    //update Max day
    MaxDay = Date_Day(TIME.year, TIME.month);
    if (TIME.day >= MaxDay) {
        TIME.day = MaxDay;
    }

    return F_tmp;
}

#endif

#if 0
u8 Is_Leap_Year(u16 year)
{
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0)
                return 1;
            else
                return 0;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

const u8 table_week[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
const u8 mon_table[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

u8 RTC_Get(void)
{
    static u16 daycnt = 0;
    u32 timecount = 0;
    u32 temp = 0;
    u16 temp1 = 0;

    timecount = RTC->CNTH;
    timecount <<= 16;
    timecount += RTC->CNTL;

    temp = timecount / 86400;
    if (daycnt != temp) {
        daycnt = temp;
        temp1 = 1970;
        while (temp >= 365) {
            if (Is_Leap_Year(temp1)) {
                if (temp >= 366)
                    temp -= 366;
                else {
                    temp1++;
                    break;
                }
            } else
                temp -= 365;
            temp1++;
        }
        TIME.year = temp1;
        temp1 = 0;
        while (temp >= 28) {
            if (Is_Leap_Year(TIME.year) && temp1 == 1) {
                if (temp >= 29)
                    temp -= 29;
                else
                    break;
            } else {
                if (temp >= mon_table[temp1])
                    temp -= mon_table[temp1];
                else
                    break;
            }
            temp1++;
        }
        TIME.month = temp1 + 1;
        TIME.day = temp + 1;
    }
    temp = timecount % 86400;
    TIME.hour = temp / 3600;
    TIME.min = (temp % 3600) / 60;
    TIME.sec = (temp % 3600) % 60;
    TIME.week = RTC_Get_Week(TIME.year, TIME.month, TIME.day);

    return 0;
}

u8 RTC_Get_Week(u16 year, u8 month, u8 day)
{
    u16 temp2;
    u8 yearH, yearL;

    yearH = year / 100;
    yearL = year % 100;
    if (yearH > 19) {
        yearL += 100;
    }
    temp2 = yearL + yearL / 4;
    temp2 = temp2 % 7;
    temp2 = temp2 + day + table_week[month - 1];
    if ((yearL % 4 == 0) && (month < 3)) {
        temp2--;
    }

    return (temp2 % 7);
}
#endif
