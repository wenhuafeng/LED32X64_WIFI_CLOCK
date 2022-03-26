#include "main.h"
#if defined(HTU21D_I2C_HARDWARE) && HTU21D_I2C_HARDWARE
#include "htu21d.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "trace_printf.h"

#define HUMI_MAX_VALUE 999 /* 99.9% */
#define TEMP_MAX_VALUE 999 /* 99.9C */

#define DEVICE_WRITE_ADDR 0x80
#define DEVICE_READ_ADDR 0x81

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
    uint16_t humiData;
    float humi;
    uint16_t tempData;
    float temp;
    uint8_t write[1];
    uint8_t read[3];
    HAL_StatusTypeDef status;

    write[0] = HUMI_NO_HOLD;
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    HAL_Delay(60); /* must 58ms the above */
    status = HAL_I2C_Master_Receive(&hi2c1, DEVICE_READ_ADDR, read, sizeof(read), 100);
    if (status != HAL_OK) {
        goto error;
    }

    if (CalcCrc(read, 2) != read[DATA_CRC8]) {
        TRACE_PRINTF("humi crc8 error\r\n");
        return false;
    }
    humiData = read[DATA_HIGH];
    humiData <<= 8;
    humiData |= read[DATA_LOW];
    humi = ((humiData & 0xfffc) / 65536.0 * 125.0 - 6.0) * 10;
    g_thData.humidity = (uint16_t)humi;
    if (g_thData.humidity > HUMI_MAX_VALUE) {
        g_thData.humidity = HUMI_MAX_VALUE;
    }
    HAL_Delay(10);

    write[0] = TEMP_NO_HOLD;
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    HAL_Delay(60); /* must 58ms the above */
    status = HAL_I2C_Master_Receive(&hi2c1, DEVICE_READ_ADDR, read, sizeof(read), 100);
    if (status != HAL_OK) {
        goto error;
    }

    if (CalcCrc(read, 2) != read[DATA_CRC8]) {
        TRACE_PRINTF("temp crc8 error\r\n");
        return false;
    }
    tempData = read[DATA_HIGH];
    tempData <<= 8;
    tempData |= read[DATA_LOW];
    temp                 = ((tempData & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    g_thData.temperature = (int16_t)temp;
    if (g_thData.temperature > TEMP_MAX_VALUE) {
        g_thData.temperature = TEMP_MAX_VALUE;
    }

    TRACE_PRINTF("Humi: %d, Temp: %d\r\n", g_thData.humidity, g_thData.temperature);
    return true;

error:
    TRACE_PRINTF("error! read temperature and humidity\r\n");
    return false;
}

static bool HTU21D_FuncInit(void)
{
    uint8_t read[1];
    uint8_t write[2];
    HAL_StatusTypeDef status;

    write[0] = READ_USER_REGISTER;
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    status = HAL_I2C_Master_Receive(&hi2c1, DEVICE_READ_ADDR, read, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    HAL_Delay(1);

    write[0] = WRITE_USER_REGISTER;
    write[1] = (read[0] | DISABLE_OTP_RELOAD);
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 2, 100);
    if (status != HAL_OK) {
        goto error;
    }
    return true;

error:
    TRACE_PRINTF("error, HTU21D function init\r\n");
    return false;
}

static bool HTU21D_SoftReset(void)
{
    HAL_StatusTypeDef status;
    uint8_t data = SOFT_RESET;

    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, &data, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }

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
    MX_I2C1_Init();
    HAL_I2C_MspInit(&hi2c1);
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
