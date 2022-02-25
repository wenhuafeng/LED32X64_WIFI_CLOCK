#include "htu21d.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "main.h"
#include "trace_printf.h"

#define HTU21D_I2C_DELAY 2

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

enum SdaIoType {
    HTU21D_SDA_OUTPUT,
    HTU21D_SDA_INPUT,
};

#define HTU21D_SDAIN()                    \
    do {                                  \
        HTU21D_SDA_SET(HTU21D_SDA_INPUT); \
    } while (0)
#define HTU21D_SDAOUT()                    \
    do {                                   \
        HTU21D_SDA_SET(HTU21D_SDA_OUTPUT); \
    } while (0)
#define HTU21D_SCL_HIGH()                                                      \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_SET); \
    } while (0)
#define HTU21D_SCL_LOW()                                                         \
    do {                                                                         \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_RESET); \
    } while (0)
#define HTU21D_SDA_HIGH()                                                      \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_SET); \
    } while (0)
#define HTU21D_SDA_LOW()                                                         \
    do {                                                                         \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_RESET); \
    } while (0)

struct Htu21dDataType {
    int16_t temperature;
    uint16_t humidity;
};
struct Htu21dDataType g_thData;
static uint8_t g_i2cFial;

static void DelayUs(uint8_t c)
{
    uint16_t count = c;

    while (count--);
}

static void HTU21D_SDA_SET(enum SdaIoType io)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    if (io == HTU21D_SDA_OUTPUT) {
        GPIO_InitStructure.Pin = HTU21D_SDA_Pin;
        GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);
    } else {
        GPIO_InitStructure.Pin = HTU21D_SDA_Pin;
        GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        GPIO_InitStructure.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);
    }
}

static void StartBus(void)
{
    HTU21D_SDA_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SDA_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_LOW();
    DelayUs(HTU21D_I2C_DELAY);
}

static void StopBus(void)
{
    HTU21D_SDA_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SDA_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
}

static void AckBus(void)
{
    HTU21D_SDA_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SDA_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
}

static void NoAckBus(void)
{
    HTU21D_SDA_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_HIGH();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_LOW();
}

static void HTU21D_I2cInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.Pin = HTU21D_SDA_Pin;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = HTU21D_SCL_Pin;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HTU21D_SCL_GPIO_Port, &GPIO_InitStructure);

    HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_SET);
}

static uint8_t WriteByte(uint8_t send_data)
{
    uint8_t temp;
    uint8_t ack = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        HTU21D_SCL_LOW();
        if ((send_data << i) & 0x80) {
            HTU21D_SDA_HIGH();
        } else {
            HTU21D_SDA_LOW();
        }
        DelayUs(HTU21D_I2C_DELAY);
        HTU21D_SCL_HIGH();
        DelayUs(HTU21D_I2C_DELAY);
    }

    HTU21D_SCL_LOW();
    DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SDA_HIGH();
    DelayUs(HTU21D_I2C_DELAY);

    //DelayUs(HTU21D_I2C_DELAY);
    HTU21D_SCL_HIGH();
    DelayUs(HTU21D_I2C_DELAY);

    HTU21D_SDAIN();
    temp = 200;
    while (--temp) {
        DelayUs(HTU21D_I2C_DELAY);
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) == GPIO_PIN_RESET) {
            ack = 1;
            break;
        }
    }
    HTU21D_SDAOUT();

    if (temp == 0) {
        ack = 0;
    }

    HTU21D_SCL_LOW();
    DelayUs(HTU21D_I2C_DELAY);

    return ack;
}

static uint8_t ReadByte(void)
{
    uint8_t value = 0;
    uint8_t i;

    HTU21D_SDAIN();
    for (i = 0; i < 8; i++) {
        HTU21D_SCL_HIGH();
        DelayUs(HTU21D_I2C_DELAY);
        value <<= 1;
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) == GPIO_PIN_SET) {
            value += 1;
        }
        HTU21D_SCL_LOW();
    }
    HTU21D_SDAOUT();

    return value;
}

static uint8_t HTU21D_GetData(void)
{
    uint16_t htu_data;
    float htu;
    uint16_t temp_data;
    float temp;
    uint8_t status;

    StartBus();
    status = WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = WriteByte(HUM_HOLD);
    if (status == 0) {
        goto i2c_fail;
    }

    StartBus();
    status = WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    HAL_Delay(20);
    htu_data = ReadByte();
    htu_data = htu_data << 8;
    AckBus();
    htu_data += ReadByte();
    NoAckBus();
    StopBus();
    htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0);
    g_thData.humidity = (uint16_t)htu * 10;

    HAL_Delay(10);

    StartBus();
    status = WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = WriteByte(TEM_HOLD);
    if (status == 0) {
        goto i2c_fail;
    }

    StartBus();
    status = WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    HAL_Delay(60); /* must 58ms the above */
    temp_data = ReadByte();
    temp_data = temp_data << 8;
    AckBus();
    temp_data += ReadByte();
    NoAckBus();
    StopBus();
    temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    g_thData.temperature = (int16_t)temp;

    TRACE_PRINTF("\r\nHumi: %d \r\n", g_thData.humidity);
    TRACE_PRINTF("Temp: %d \r\n", g_thData.temperature);
    g_i2cFial = 0;
    return true;

i2c_fail:
    g_thData.temperature = 0x00;
    g_thData.humidity = 0x00;
    TRACE_PRINTF("\r\ntemperature read fail. \r\n");
    g_i2cFial = 1;
    return false;
}

static uint8_t HTU21D_FuncInit(void)
{
    uint8_t usr_reg;
    uint8_t status;

    StartBus();
    status = WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = WriteByte(RDSR);
    if (status == 0) {
        goto i2c_fail;
    }

    StartBus();
    status = WriteByte(DEVICE_ADDR | READ);
    if (status == 0) {
        goto i2c_fail;
    }

    usr_reg = ReadByte();
    NoAckBus();

    StartBus();
    status = WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto i2c_fail;
    }

    status = WriteByte(WRSR);
    if (status == 0) {
        goto i2c_fail;
    }

    status = WriteByte((usr_reg & 0x38) | 0x02);
    if (status == 0) {
        goto i2c_fail;
    }

    StopBus();

    g_i2cFial = 0;
    return true;

i2c_fail:
    g_i2cFial = 1;
    return false;
}

static uint8_t HTU21D_Reset(void)
{
    uint8_t status;

    StartBus();
    status = WriteByte(DEVICE_ADDR | WRITE);
    if (status == 0) {
        goto error;
    }
    status = WriteByte(HTU21D_RESET);
    if (status == 0) {
        goto error;
    }
    StopBus();
    HAL_Delay(15);

    g_i2cFial = 0;
    TRACE_PRINTF("\r\nHTU21D Reset OK\r\n");
    return true;

error:
    TRACE_PRINTF("\r\nHTU21D Reset NG\r\n");
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
    HTU21D_I2cInit();
    if (HTU21D_FuncInit() == false) {
        TRACE_PRINTF("\r\nHTU21D Init NG\r\n");
        return;
    }
    HAL_Delay(10);
    HTU21D_Reset();
    HAL_Delay(100);
    HTU21D_GetData();
}
