#include "main.h"
#if defined(HTU21D_I2C_SOFTWARE) && HTU21D_I2C_SOFTWARE
#include "htu21d.h"
#include <stdint.h>
#include <stdbool.h>
#include "htu21d_iic_sw.h"
#include "trace_printf.h"

#define WRITE 0
#define READ 1

#define DEVICE_ADDR 0x80

#define TEMP_HOLD 0xe3
#define TEMP_NO_HOLD 0xf3

#define HUMI_HOLD 0xe5
#define HUMI_NO_HOLD 0xf5

#define WRITE_USER_REGISTER 0xe6
#define READ_USER_REGISTER 0xe7
#define SOFT_RESET 0xfe

struct Htu21dDataType {
    int16_t temperature;
    uint16_t humidity;
};
struct Htu21dDataType g_thData;

static bool HTU21D_GetData(void)
{
    uint16_t htu_data;
    float htu;
    uint16_t temp_data;
    float temp;
    uint8_t status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte(HUMI_HOLD);
    if (status == 0) {
        goto i2c_fail;
    }

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    HAL_Delay(20);
    htu_data = I2C_ReadByte();
    htu_data = htu_data << 8;
    I2C_Ack();
    htu_data += I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();
    htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0) * 10;
    g_thData.humidity = (uint16_t)htu;

    HAL_Delay(10);

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte(TEMP_HOLD);
    if (status == 0) {
        goto i2c_fail;
    }

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    HAL_Delay(60); /* must 58ms the above */
    temp_data = I2C_ReadByte();
    temp_data = temp_data << 8;
    I2C_Ack();
    temp_data += I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();
    temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    g_thData.temperature = (int16_t)temp;

    TRACE_PRINTF("Humi: %d \r\n", g_thData.humidity);
    TRACE_PRINTF("Temp: %d \r\n", g_thData.temperature);
    return true;

i2c_fail:
    g_thData.temperature = 0x00;
    g_thData.humidity = 0x00;
    TRACE_PRINTF("temperature read fail. \r\n");
    return false;
}

static bool HTU21D_FuncInit(void)
{
    uint8_t usr_reg;
    uint8_t status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte(READ_USER_REGISTER);
    if (status == 0) {
        goto i2c_fail;
    }

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    usr_reg = I2C_ReadByte();
    I2C_Nack();

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte(WRITE_USER_REGISTER);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte((usr_reg & 0x38) | 0x02);
    if (status == 0) {
        goto i2c_fail;
    }

    I2C_Stop();
    return true;

i2c_fail:
    return false;
}

static bool HTU21D_SoftReset(void)
{
    uint8_t status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto error;
    }
    status = I2C_WriteByte(SOFT_RESET);
    if (status == 0) {
        goto error;
    }
    I2C_Stop();
    HAL_Delay(15);

    TRACE_PRINTF("HTU21D Reset OK\r\n");
    return true;

error:
    TRACE_PRINTF("HTU21D Reset NG\r\n");
    return false;
}

int16_t HTU21D_GetTemperature(void)
{
    return g_thData.temperature;
}

uint16_t HTU21D_GetHumidity(void)
{
    return g_thData.humidity;
}

void HTU21D_Init(void)
{
    I2C_Init();
    if (HTU21D_FuncInit() == false) {
        TRACE_PRINTF("HTU21D Init NG\r\n");
        return;
    }
    HAL_Delay(10);
    HTU21D_SoftReset();
    HAL_Delay(100);
    HTU21D_GetData();
    TRACE_PRINTF("HTU21D Init OK\r\n");
}

void HTU21D_Sampling(void)
{
    static uint8_t count = 0x00;

    count++;
    if (count > 9) {
        count = 0;
        HTU21D_GetData();
    }
}

#endif
