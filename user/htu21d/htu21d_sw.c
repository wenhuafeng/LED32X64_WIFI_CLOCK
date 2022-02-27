#include "htu21d_sw.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "htu21d_iic_sw.h"
#include "trace_printf.h"

#define WRITE 0
#define READ 1

#define DEVICE_ADDR 0x80

#define TEM_HOLD 0xe3
#define TEM_NO 0xf3

#define HUM_HOLD 0xe5
#define HUM_NO 0xf5

#define WRSR 0xe6
#define RDSR 0xe7
#define HTU21D_RESET 0xfe

struct Htu21dDataType {
    int16_t temperature;
    uint16_t humidity;
};
struct Htu21dDataType g_thData;
static uint8_t g_i2cFial;

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

    status = I2C_WriteByte(HUM_HOLD);
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
    htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0);
    g_thData.humidity = (uint16_t)htu * 10;

    HAL_Delay(10);

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte(TEM_HOLD);
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
    g_i2cFial = 0;
    return true;

i2c_fail:
    g_thData.temperature = 0x00;
    g_thData.humidity = 0x00;
    TRACE_PRINTF("temperature read fail. \r\n");
    g_i2cFial = 1;
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

    status = I2C_WriteByte(RDSR);
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

    status = I2C_WriteByte(WRSR);
    if (status == 0) {
        goto i2c_fail;
    }

    status = I2C_WriteByte((usr_reg & 0x38) | 0x02);
    if (status == 0) {
        goto i2c_fail;
    }

    I2C_Stop();

    g_i2cFial = 0;
    return true;

i2c_fail:
    g_i2cFial = 1;
    return false;
}

static bool HTU21D_Reset(void)
{
    uint8_t status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto error;
    }
    status = I2C_WriteByte(HTU21D_RESET);
    if (status == 0) {
        goto error;
    }
    I2C_Stop();
    HAL_Delay(15);

    g_i2cFial = 0;
    TRACE_PRINTF("HTU21D Reset OK\r\n");
    return true;

error:
    TRACE_PRINTF("HTU21D Reset NG\r\n");
    g_i2cFial = 1;
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

void HTU21D_Sampling(void)
{
    static uint8_t count;

    if (g_i2cFial) {
        g_i2cFial = 0;
        count = 0x00;
        HTU21D_Init();
    }
    count++;
    if (count > 9) {
        count = 0;
        HTU21D_GetData();
    }
}

void HTU21D_Init(void)
{
    I2C_Init();
    if (HTU21D_FuncInit() == false) {
        TRACE_PRINTF("HTU21D Init NG\r\n");
        return;
    }
    HAL_Delay(10);
    HTU21D_Reset();
    HAL_Delay(100);
    HTU21D_GetData();
}
