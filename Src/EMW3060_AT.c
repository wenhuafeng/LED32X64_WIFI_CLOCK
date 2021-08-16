#include "EMW3060_AT.h"
#include <stdio.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "TypeDefine.h"
#include "main.h"
#include "RTC_software.h"
#include "usart.h"

#if (WIFI_MODULE == WIFI_EMW3060)

#define _MON_ 0x004d6f6e
#define _TUE_ 0x00547565
#define _WED_ 0x00576564
#define _THU_ 0x00546875
#define _FRI_ 0x00467269
#define _SAT_ 0x00536174
#define _SUN_ 0x0053756e

#define _JAN_ 0x3031 //0x004a616e
#define _FEB_ 0x3032 //0x00466562
#define _MAR_ 0x3033 //0x004d6172
#define _APR_ 0x3034 //0x00417072
#define _MAY_ 0x3035 //0x004d6179
#define _JUN_ 0x3036 //0x004a756e
#define _JUL_ 0x3037 //0x004a756c
#define _AUG_ 0x3038 //0x00417567
#define _SEP_ 0x3039 //0x00536570
#define _OCT_ 0x3130 //0x004f6374
#define _NOV_ 0x3131 //0x004e6f76
#define _DEC_ 0x3132 //0x00446563

#define _GET_TIME_1S_ (1)
#define _GET_TIME_2S_ (2)
#define _GET_TIME_3S_ (3)
#define _GET_TIME_4S_ (4)
#define _GET_TIME_5S_ (5)
#define _GET_TIME_6S_ (6)
#define _GET_TIME_7S_ (7)
#define _GET_TIME_8S_ (8)
#define _GET_TIME_9S_ (9)
#define _GET_TIME_10S_ (10)
#define _GET_TIME_11S_ (11)
#define _GET_TIME_12S_ (12)
#define _GET_TIME_13S_ (13)
#define _GET_TIME_14S_ (14)
#define _GET_TIME_15S_ (15)
#define _GET_TIME_16S_ (16)
#define _GET_TIME_17S_ (17)
#define _GET_TIME_18S_ (18)
#define _GET_TIME_19S_ (19)
#define _GET_TIME_20S_ (20)
#define _GET_TIME_21S_ (21)
#define _GET_TIME_22S_ (22)
#define _GET_TIME_23S_ (23)
#define _GET_TIME_24S_ (24)
#define _GET_TIME_25S_ (25)
#define _GET_TIME_26S_ (26)
#define _GET_TIME_27S_ (27)
#define _GET_TIME_28S_ (28)
#define _GET_TIME_29S_ (29)
#define _GET_TIME_30S_ (30)

enum ConnectFlag {
    DISCONNECT,
    CONNECT
};

#define _WIFI_OFF_TIME_ (2 * 60) //2min
#define EMW3060_AT_POWER_PIN_HIGH()                                            \
    do {                                                                       \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, GPIO_PIN_SET); \
    } while (0)
#define EMW3060_AT_POWER_PIN_LOW()                              \
    do {                                                        \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, \
                          GPIO_PIN_RESET);                      \
    } while (0)

static u8 g_powerOffCtr;
static u8 g_getTimeCtr;
static u8 g_renewInitCtr;
static u8 g_renewInitCtr_1;

static enum ConnectFlag g_connect;
static BOOLEAN g_timeDataOkFlag;

static u8 AscToHex(u8 asc)
{
    u8 hex = asc;

    if ((asc >= 0x30) && (asc <= 0x39)) {
        hex -= 0x30;
    } else if ((asc >= 0x41) && (asc <= 0x46)) {
        hex -= 0x37;
    } else if ((asc >= 0x61) && (asc <= 0x66)) {
        hex -= 0x57;
    } else {
        hex = 0xff;
    }

    return hex;
}

static u8 WeekDeal(u16 Year, u8 Month, u8 Day)
{
    s16 temp_year = 0;
    s8 temp_cen = 0;
    s8 temp_month = 0;
    s8 week_data;

    if (Month < 3) {
        temp_month = Month + 12;
        temp_year = Year - 1;
    } else {
        temp_month = Month;
        temp_year = Year;
    }

    temp_cen = temp_year / 100;
    temp_year = temp_year % 100;

    week_data = temp_year + temp_year / 4 + temp_cen / 4;
    week_data = week_data - 2 * temp_cen + 26 * (temp_month + 1) / 10 + Day - 1;
    week_data = (week_data + 140) % 7;

    return week_data;
}

static u8 EMW3060_AT_ProcessClock(char *cRxBuf)
{
    u16 year;
    u8 month, day, hour, minute, second;
    BOOLEAN F_tmp = 1;

    year = AscToHex(cRxBuf[10]) * 1000 + AscToHex(cRxBuf[11]) * 100 +
           AscToHex(cRxBuf[12]) * 10 + AscToHex(cRxBuf[13]);
    month = AscToHex(cRxBuf[15]) * 10 + AscToHex(cRxBuf[16]);
    day = AscToHex(cRxBuf[18]) * 10 + AscToHex(cRxBuf[19]);
    hour = AscToHex(cRxBuf[21]) * 10 + AscToHex(cRxBuf[22]);
    minute = AscToHex(cRxBuf[24]) * 10 + AscToHex(cRxBuf[25]);
    second = AscToHex(cRxBuf[27]) * 10 + AscToHex(cRxBuf[28]);

    if (year == 1970) {
        F_tmp = 0;
    } else {
        if ((year < 2000) || (year > 2099) || (month == 0) || (month > 12) ||
            (day == 0) || (day > 31) || (hour > 23) || (minute > 59) || (second > 59)) {
            F_tmp = 0;
        }
    }

    if (F_tmp != 0x00) {
        TIME.year = year;
        TIME.month = month;
        TIME.day = day;
        TIME.week = WeekDeal(TIME.year, TIME.month, TIME.day);
        TIME.hour = hour;
        TIME.min = minute;
        TIME.sec = second;

        HAL_UART_DeInit(&huart1);
        g_powerOffCtr = 0x00;
        EMW3060_AT_POWER_PIN_LOW();

        printf("\r\nTime: %d-%d-%d   %02d:%02d:%02d \r\n", TIME.year,
               TIME.month, TIME.day, TIME.hour, TIME.min, TIME.sec);
    } else {
        g_getTimeCtr = _GET_TIME_10S_;
    }

    return F_tmp;
}

static void EMW3060_AT_GetTime(void)
{
    printf("AT+SNTPTIME\r\n");
}

static void EMW3060_AT_SNTP_CFG(void)
{
    printf("AT+SNTPCFG=+8,cn.ntp.org.cn,pool.ntp.org\r\n");
}

void WIFI_Init(void)
{
    printf("AT+FWVER?\r\n");
    HAL_Delay(1000);

    printf("AT+WFVER?\r\n");
    HAL_Delay(100);

    printf("AT+WMAC?\r\n");
    HAL_Delay(100);

    printf("AT+WJAP=ABC,WenHuaFeng547566993\r\n");
}

void WIFI_PowerOnOff(enum PowerFlag flag)
{
    if (flag == POWER_ON) {
        EMW3060_AT_POWER_PIN_HIGH();
        g_powerOffCtr = _WIFI_OFF_TIME_;
    } else {
        EMW3060_AT_POWER_PIN_LOW();
        g_connect = DISCONNECT;
        g_getTimeCtr = 0x00;
        g_powerOffCtr = 0x00;
        g_renewInitCtr = 0x00;
        g_renewInitCtr_1 = 0x00;
    }
}

void WIFI_CtrDec(void)
{
    if (g_powerOffCtr) {
        g_powerOffCtr--;
        if (g_powerOffCtr == 0x00) {
            //off wifi. 5min rx time fail.
            WIFI_PowerOnOff(POWER_OFF);
        }
    }

    if (g_getTimeCtr) {
        g_getTimeCtr--;
        if (g_getTimeCtr == _GET_TIME_14S_) {
            EMW3060_AT_SNTP_CFG();
        }
        else if (g_getTimeCtr == _GET_TIME_8S_) {
            EMW3060_AT_GetTime();
        } else if (g_getTimeCtr == 0x00) {
            EMW3060_AT_GetTime();
        }
    }

    if (g_renewInitCtr) {
        g_renewInitCtr--;
        if (g_renewInitCtr == 0x00) {
            g_renewInitCtr_1 = 2;
            WIFI_PowerOnOff(POWER_ON);
        }
    }

    if (g_renewInitCtr_1) {
        g_renewInitCtr_1--;
        if (g_renewInitCtr_1 == 0x00) {
            WIFI_Init();
        }
    }
}

void WIFI_ReceiveProcess(u8 *buf)
{
    char *str;
    char *strPosition;

    str = "+WEVENT:STATION_UP"; /* WIFI CONNECTED */
    if (strstr((char *)buf, str) != NULL) {
        g_connect = CONNECT;
        g_getTimeCtr = _GET_TIME_16S_;
    }
    str = "+WEVENT:STATION_DOWN"; /* WIFI DISCONNECT */
    if (strstr((char *)buf, str) != NULL) {
        WIFI_PowerOnOff(POWER_OFF);
        g_renewInitCtr = 2;
    }
    // str = "+CWJAP:2";//"WIFI CONNECT FAIL";
    // if (strstr(buf, str) != NULL) {
    //   WIFI_PowerOnOff(_POWER_OFF_);
    //   EMW3060_AT_RenewInitCtr = 2;
    // }
    if (g_connect == CONNECT) {
        str = "+SNTPTIME:20";
        strPosition = strstr((char *)buf, str);
        if (strPosition != NULL) {
            EMW3060_AT_ProcessClock(strPosition);
            g_timeDataOkFlag = 1;
        }
    }
}

BOOLEAN WIFI_GetTimeDataFlag(void)
{
    return g_timeDataOkFlag;
}

void WIFI_SetTimeDataFlag(BOOLEAN value)
{
    g_timeDataOkFlag = value;
}

#endif
