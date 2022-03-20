#ifndef HTU21D_IIC_H
#define HTU21D_IIC_H

#include <stdint.h>
#include <stdbool.h>

extern void I2C_Start(void);
extern void I2C_Stop(void);
extern void I2C_Ack(void);
extern void I2C_Nack(void);
extern void I2C_Init(void);
extern bool I2C_WriteByte(uint8_t send);
extern uint8_t I2C_ReadByte(void);

#endif
