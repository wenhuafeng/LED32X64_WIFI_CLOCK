#ifndef HTU21D_H
#define HTU21D_H

#include <stdint.h>
#include <stdbool.h>

struct Htu21dDataType {
    int16_t temperature;
    uint16_t humidity;
};

void HTU21D_Init(void);
bool HTU21D_GetData(struct Htu21dDataType *th);

#endif
