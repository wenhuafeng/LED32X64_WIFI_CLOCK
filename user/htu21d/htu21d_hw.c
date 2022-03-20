#include "main.h"
#if defined(HTU21D_I2C_HARDWARE) && HTU21D_I2C_HARDWARE
#include "htu21d.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "trace_printf.h"

#define DEVICE_WRITE_ADDR 0x80
#define DEVICE_READ_ADDR 0x81

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
    uint16_t humiData;
    float humi;
    uint16_t tempData;
    float temp;
    uint8_t write[1];
    uint8_t read[2];
    HAL_StatusTypeDef status;

    write[0] = HUMI_HOLD;
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    status = HAL_I2C_Master_Receive(&hi2c1, DEVICE_READ_ADDR, read, 2, 100);
    if (status != HAL_OK) {
        goto error;
    }

    humiData = read[0];
    humiData <<= 8;
    humiData |= read[1];
    humi = ((humiData & 0xfffc) / 65536.0 * 125.0 - 6.0) * 10;
    g_thData.humidity = (uint16_t)humi;
    HAL_Delay(10);

    write[0] = TEMP_HOLD;
    status = HAL_I2C_Master_Transmit(&hi2c1, DEVICE_WRITE_ADDR, write, 1, 100);
    if (status != HAL_OK) {
        goto error;
    }
    status = HAL_I2C_Master_Receive(&hi2c1, DEVICE_READ_ADDR, read, 2, 100);
    if (status != HAL_OK) {
        goto error;
    }
    HAL_Delay(60); /* must 58ms the above */
    tempData = read[0];
    tempData <<= 8;
    tempData |= read[1];
    temp = ((tempData & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    g_thData.temperature = (int16_t)temp;

    TRACE_PRINTF("Humi: %d \r\n", g_thData.humidity);
    TRACE_PRINTF("Temp: %d \r\n", g_thData.temperature);
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
    write[1] = (read[0] & 0x38) | 0x02;
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
