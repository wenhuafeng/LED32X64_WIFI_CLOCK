#ifndef HTU21D_H
#define HTU21D_H

#include "type_define.h"

s16 GetTemperature(void);
u16 GetHumidity(void);
void HTU21D_I2cInit(void);
u8 HTU21D_Init(void);
u8 HTU21D_Reset(void);
u8 HTU21D_GetData(void);
void HTU21D_Sampling(void);

#endif
