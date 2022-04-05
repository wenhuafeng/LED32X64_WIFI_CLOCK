#ifndef HTU21D_H
#define HTU21D_H

#include <stdint.h>
#include <stdbool.h>

int16_t HTU21D_GetTemperature(void);
uint16_t HTU21D_GetHumidity(void);
void HTU21D_Sampling(void);
void HTU21D_Init(void);
bool HTU21D_GetData(void);

#endif
