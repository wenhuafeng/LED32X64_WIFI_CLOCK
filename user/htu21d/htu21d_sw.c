#include "main.h"
#if defined(HTU21D_I2C_SOFTWARE) && HTU21D_I2C_SOFTWARE
#include "htu21d.h"
#include <stdint.h>
#include <stdbool.h>
#include "htu21d_iic_sw.h"
#include "trace_printf.h"

#define HUMI_MAX_VALUE 999 /* 99.9% */
#define TEMP_MAX_VALUE 999 /* 99.9C */

#define WRITE 0
#define READ 1

#define DEVICE_ADDR 0x80

#define TEMP_HOLD 0xe3
#define TEMP_NO_HOLD 0xf3

#define HUMI_HOLD 0xe5
#define HUMI_NO_HOLD 0xf5

#define SOFT_RESET 0xfe

// data byte define
#define DATA_HIGH 0
#define DATA_LOW  1
#define DATA_CRC8 2

// user register
#define WRITE_USER_REGISTER 0xe6
#define READ_USER_REGISTER  0xe7
#define DISABLE_OTP_RELOAD  0x02

// crc8
#define POLYNOMIAL 0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 00110001

struct Htu21dDataType {
    int16_t temperature;
    uint16_t humidity;
};
struct Htu21dDataType g_thData;

static uint8_t CalcCrc(uint8_t *crcData, uint8_t len)
{
    uint8_t i, bit;
    uint8_t crc = 0x00;

    // calculates 8-Bit checksum with given polynomial
    for (i = 0; i < len; i++) {
        crc ^= (crcData[i]);
        for (bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

static bool HTU21D_GetData(void)
{
    uint16_t htu_data;
    float htu;
    uint16_t temp_data;
    float temp;
    uint8_t read[3];
    bool status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == false) {
        goto i2c_fail;
    }
    status = I2C_WriteByte(HUMI_NO_HOLD);
    if (status == false) {
        goto i2c_fail;
    }

    HAL_Delay(60); /* must 58ms the above */

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == false) {
        goto i2c_fail;
    }
    read[DATA_HIGH] = I2C_ReadByte();
    I2C_Ack();
    read[DATA_LOW] = I2C_ReadByte();
    I2C_Ack();
    read[DATA_CRC8] = I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();

    if (CalcCrc(read, 2) != read[DATA_CRC8]) {
        TRACE_PRINTF("humi crc8 error\r\n");
        return false;
    }

    htu_data = read[DATA_HIGH];
    htu_data = htu_data << 8;
    htu_data += read[DATA_LOW];
    htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0) * 10;
    g_thData.humidity = (uint16_t)htu;
    if (g_thData.humidity > HUMI_MAX_VALUE) {
        g_thData.humidity = HUMI_MAX_VALUE;
    }

    HAL_Delay(10);

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == false) {
        goto i2c_fail;
    }
    status = I2C_WriteByte(TEMP_NO_HOLD);
    if (status == false) {
        goto i2c_fail;
    }

    HAL_Delay(60); /* must 58ms the above */

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == false) {
        goto i2c_fail;
    }
    read[DATA_HIGH] = I2C_ReadByte();
    I2C_Ack();
    read[DATA_LOW] = I2C_ReadByte();
    I2C_Ack();
    read[DATA_CRC8] = I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();

    if (CalcCrc(read, 2) != read[DATA_CRC8]) {
        TRACE_PRINTF("temp crc8 error\r\n");
        return false;
    }
    temp_data = read[DATA_HIGH];
    temp_data = temp_data << 8;
    temp_data += read[DATA_LOW];
    temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    g_thData.temperature = (int16_t)temp;
    if (g_thData.temperature > TEMP_MAX_VALUE) {
        g_thData.temperature = TEMP_MAX_VALUE;
    }

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
    uint8_t userReg;
    bool status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == false) {
        TRACE_PRINTF("err0\r\n");
        goto i2c_fail;
    }
    status = I2C_WriteByte(READ_USER_REGISTER);
    if (status == false) {
        TRACE_PRINTF("err1\r\n");
        goto i2c_fail;
    }

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | READ);
    if (status == false) {
        TRACE_PRINTF("err2\r\n");
        goto i2c_fail;
    }
    userReg = I2C_ReadByte();
    I2C_Nack();
    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == false) {
        TRACE_PRINTF("err3\r\n");
        goto i2c_fail;
    }
    status = I2C_WriteByte(WRITE_USER_REGISTER);
    if (status == false) {
        TRACE_PRINTF("err4\r\n");
        goto i2c_fail;
    }
    status = I2C_WriteByte(userReg | 0x02);
    if (status == false) {
        TRACE_PRINTF("err5\r\n");
        goto i2c_fail;
    }
    I2C_Stop();
    return true;

i2c_fail:
    return false;
}

static bool HTU21D_SoftReset(void)
{
    bool status;

    I2C_Start();
    status = I2C_WriteByte(DEVICE_ADDR | WRITE);
    if (status == false) {
        TRACE_PRINTF("err6\r\n");
        goto i2c_fail;
    }
    status = I2C_WriteByte(SOFT_RESET);
    if (status == false) {
        TRACE_PRINTF("err7\r\n");
        goto i2c_fail;
    }
    I2C_Stop();
    HAL_Delay(15);

    TRACE_PRINTF("HTU21D Reset OK\r\n");
    return true;

i2c_fail:
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
    HAL_Delay(10);
    (void)I2C_WriteByte(DEVICE_ADDR | WRITE);
    I2C_Stop();
    HAL_Delay(10);
    HTU21D_SoftReset();
    HAL_Delay(10);
    if (HTU21D_FuncInit() == false) {
        TRACE_PRINTF("HTU21D Init NG\r\n");
        return;
    } else {
        TRACE_PRINTF("HTU21D Init OK\r\n");
    }

    HAL_Delay(100);
    HTU21D_GetData();
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
