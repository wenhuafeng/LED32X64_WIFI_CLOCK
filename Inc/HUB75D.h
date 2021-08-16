#ifndef HUB75D_H
#define HUB75D_H

#include "TypeDefine.h"
#include "gpio_bit_ctrl.h"
#include "RTC_software.h"

#define HUB_A   PBout(13)
#define HUB_B   PBout(14)
#define HUB_C   PBout(15)
#define HUB_D   PAout(8)
#define HUB_OE  PBout(4)
#define HUB_LAT PBout(3)
#define HUB_CLK PAout(15)
#define HUB_R1  PAout(6)
#define HUB_G1  PAout(5)
#define HUB_B1  PAout(7)
#define HUB_R2  PBout(0)
#define HUB_G2  PBout(1)
#define HUB_B2  PBout(10)

extern u8 const Fu_Tbl[];
#define _DISP_OFF_TIME_5MIN_ (5 * 60)
extern u16 HUB75D_DispOffCtr;

typedef enum { _DISP_T_, _DISP_H_ } HUB75D_DispTHFlag;
typedef enum { _DISP_OFF_, _DISP_ON_ } HUB75D_DispOnOffFlag;

//void Scan_Mode(void);
void HUB75D_DispScan(void);

void HUB75D_CalcClock(rtc_counter_value_t *time);
void HUB75D_CalcYinli(rtc_counter_value_t *time);
void HUB75D_CtrDec(void);

void HUB75D_DispOnOff(HUB75D_DispOnOffFlag F_tmp);

#endif
