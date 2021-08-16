#ifndef HTU21D_H_
#define HTU21D_H_

#include <stdint.h>

#define Humidity_Sensor_ID 0x50 //0x40//

#define Trigger_Temperature_MeasurementH  0xE3 //Hold master
#define Trigger_Humidity_MeasurementH     0xE5 //Hold master
#define Trigger_Temperature_MeasurementNH 0xF3 //No Hold master
#define Trigger_Humidity_MeasurementNH    0xF5 //No Hold master
#define Write_User_Register               0xE6
#define Read_User_Register                0xE7
#define Soft_Reset                        0xFE

enum {
    READ_TEMP_1,
    READ_TEMP_2,
    READ_HUMI_1,
    READ_HUMI_2,
};

extern s16 Temperature;
extern u16 Humidity;

void HTU21D_I2cInit(void);
u8 HTU21D_Init(void);
u8 HTU21D_Reset(void);
u8 HTU21D_GetData(void);
void HTU21D_Sampling(void);

#endif
