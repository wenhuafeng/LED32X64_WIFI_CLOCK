#include <stdio.h>
#include "TypeDefine.h"
#include "stm32f1xx_hal.h"
#include "RTC_software.h"
#include "HTU21D.h"
#include "main.h"

#define Write 0
#define Read 1

#define HIGH 1
#define LOW 0

#define TRUE 1
#define FALSE 0

#define _HTU21D_I2C_DELAY_ 2

#define SlaveAddr 0x80
#define resolution 0

#define TEM_HOLD 0xe3
#define TEM_NO 0xf3

#define HUM_HOLD 0xe5
#define HUM_NO 0xf5

#define WRSR 0xe6
#define RDSR 0xe7
#define HT_RST 0xfe

typedef enum {
    _HTU21D_SDA_OUTPUT_,
    _HTU21D_SDA_INPUT_,
} SDA_IO_Type;

#define HTU21D_SDAIN()                      \
    do {                                    \
        HTU21D_SDA_SET(_HTU21D_SDA_INPUT_); \
    } while (0)
#define HTU21D_SDAOUT()                      \
    do {                                     \
        HTU21D_SDA_SET(_HTU21D_SDA_OUTPUT_); \
    } while (0)
#define HTU21D_SCL_HIGH()                                                      \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_SET); \
    } while (0)
#define HTU21D_SCL_LOW()                                        \
    do {                                                        \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, \
                          GPIO_PIN_RESET);                      \
    } while (0)
#define HTU21D_SDA_HIGH()                                                      \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_SET); \
    } while (0)
#define HTU21D_SDA_LOW()                                        \
    do {                                                        \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, \
                          GPIO_PIN_RESET);                      \
    } while (0)

s16 Temperature;
u16 Humidity;
u8 flag_i2c_fial;

static void DelayUs(u8 j)
{
    u16 i = j;

    while (i--);
}

static void HTU21D_SDA_SET(SDA_IO_Type ioset)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    if (ioset == _HTU21D_SDA_OUTPUT_) {
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
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SDA_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
}

static void StopBus(void)
{
    HTU21D_SDA_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SDA_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
}

static void AckBus(void)
{
    HTU21D_SDA_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SDA_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
}

static void NoAckBus(void)
{
    HTU21D_SDA_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_LOW();
}

void HTU21D_I2cInit(void)
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

static u8 WriteByte(u8 send_data)
{
    u8 temp;
    u8 ack = 0;
    u8 i;

    for (i = 0; i < 8; i++) {
        HTU21D_SCL_LOW();
        if ((send_data << i) & 0x80) {
            HTU21D_SDA_HIGH();
        } else {
            HTU21D_SDA_LOW();
        }
        DelayUs(_HTU21D_I2C_DELAY_);
        HTU21D_SCL_HIGH();
        DelayUs(_HTU21D_I2C_DELAY_);
    }

    HTU21D_SCL_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SDA_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);

    //DelayUs(_HTU21D_I2C_DELAY_);
    HTU21D_SCL_HIGH();
    DelayUs(_HTU21D_I2C_DELAY_);

    HTU21D_SDAIN();
    temp = 200;
    while (--temp) {
        DelayUs(_HTU21D_I2C_DELAY_);
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) ==
            GPIO_PIN_RESET) {
            ack = 1;
            break;
        }
    }
    HTU21D_SDAOUT();

    if (temp == 0) {
        ack = 0;
    }

    HTU21D_SCL_LOW();
    DelayUs(_HTU21D_I2C_DELAY_);

    return ack;
}

static u8 ReadByte(void)
{
    u8 value = 0;
    u8 i;

    HTU21D_SDAIN();
    for (i = 0; i < 8; i++) {
        HTU21D_SCL_HIGH();
        DelayUs(_HTU21D_I2C_DELAY_);
        value <<= 1;
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) ==
            GPIO_PIN_SET) {
            value += 1;
        }
        HTU21D_SCL_LOW();
    }
    HTU21D_SDAOUT();

    return (value);
}

u8 HTU21D_GetData(void)
{
    u16 htu_data;
    float htu;
    u16 temp_data;
    float temp;
    u8 err;

    StartBus();
    err = WriteByte(SlaveAddr | Write);
    if (err == 0)
        goto i2c_fail;
    err = WriteByte(HUM_HOLD);
    if (err == 0)
        goto i2c_fail;

    StartBus();
    err = WriteByte(SlaveAddr | Read);
    if (err == 0)
        goto i2c_fail;
    HAL_Delay(20);
    htu_data = ReadByte();
    htu_data = htu_data << 8;
    AckBus();
    htu_data += ReadByte();
    NoAckBus();
    StopBus();
    htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0);
    Humidity = (u16)htu * 10;

    HAL_Delay(10);

    StartBus();
    err = WriteByte(SlaveAddr | Write);
    if (err == 0)
        goto i2c_fail;
    err = WriteByte(TEM_HOLD);
    if (err == 0)
        goto i2c_fail;

    StartBus();
    err = WriteByte(SlaveAddr | Read);
    if (err == 0)
        goto i2c_fail;
    HAL_Delay(60); //must 58ms the above
    temp_data = ReadByte();
    temp_data = temp_data << 8;
    AckBus();
    temp_data += ReadByte();
    NoAckBus();
    StopBus();
    temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85) * 10;
    Temperature = (s16)temp;

    printf("\r\nHumi: %d \r\n", Humidity);
    printf("Temp: %d \r\n", Temperature);
    printf("Time: %d-%d-%d   %02d:%02d:%02d \r\n", TIME.year, TIME.month,
           TIME.day, TIME.hour, TIME.min, TIME.sec);
    flag_i2c_fial = 0;
    return TRUE;

i2c_fail:
    Temperature = 0x00;
    Humidity = 0x00;
    printf("\r\nTemp test fail. \r\n");
    flag_i2c_fial = 1;
    return FALSE;
}

u8 HTU21D_Init(void)
{
    u8 usr_reg;
    u8 err;

    StartBus();
    err = WriteByte(SlaveAddr | Write);
    if (err == 0)
        goto i2c_fail;
    err = WriteByte(RDSR);
    if (err == 0)
        goto i2c_fail;

    StartBus();
    err = WriteByte(SlaveAddr | Read);
    if (err == 0)
        goto i2c_fail;
    usr_reg = ReadByte();
    NoAckBus();

    StartBus();
    err = WriteByte(SlaveAddr | Write);
    if (err == 0)
        goto i2c_fail;
    err = WriteByte(WRSR);
    if (err == 0)
        goto i2c_fail;
    err = WriteByte((usr_reg & 0x38) | 0x02 | resolution);
    if (err == 0)
        goto i2c_fail;
    StopBus();

    flag_i2c_fial = 0;
    printf("\r\nHTU21D Init OK\r\n");
    return TRUE;

i2c_fail:
    printf("\r\nHTU21D Init NG\r\n");
    flag_i2c_fial = 1;
    return FALSE;
}

void HTU21D_Sampling(void)
{
    static u8 count;

    if (flag_i2c_fial) {
        flag_i2c_fial = 0;
        count = 0x00;
        HTU21D_Init();
        HTU21D_Reset();
        HAL_Delay(10);
        HTU21D_GetData();
    }
    count++;
    if (count >= 10) {
        count = 0;
        HTU21D_GetData();
    }
    //else if (i > 26) {
    //  //Read_TH();
    //}
}

u8 HTU21D_Reset(void)
{
    u8 status;

    StartBus();
    status = WriteByte(SlaveAddr | Write);
    if (status == 0)
        goto error;
    status = WriteByte(HT_RST);
    if (status == 0)
        goto error;
    StopBus();
    HAL_Delay(15);

    flag_i2c_fial = 0;
    printf("\r\nHTU21D Reset OK\r\n");
    return TRUE;

error:
    printf("\r\nHTU21D Reset NG\r\n");
    flag_i2c_fial = 1;
    return FALSE;
}

#if 0
void TEMP_READ(void)
{
    u16 temp_data;
    float temp;

    StartBus();
    WriteByte(SlaveAddr | Write);
    WriteByte(TEM_HOLD);

    StartBus();
    WriteByte(SlaveAddr | Read);
    HAL_Delay(60);
    temp_data = ReadByte();
    temp_data = temp_data << 8;
    AckBus();
    temp_data += ReadByte();
    NoAckBus();
    StopBus();
    temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85);
    Temperature = temp * 10;
}

void ReadTH(void)
{
    u16 temp_data;
    FP32 temp;

    u16 htu_data;
    FP32 htu;
    static u8 i;

    if (i == _READ_TEMP_1_) {
        StartBus();
        WriteByte(SlaveAddr | Write);
        WriteByte(TEM_HOLD);
        StartBus();
        WriteByte(SlaveAddr | Read);
        i = _READ_TEMP_2_;
    } else if (i == _READ_TEMP_2_) {
        temp_data = ReadByte();
        temp_data = temp_data << 8;
        AckBus();
        temp_data += ReadByte();
        NoAckBus();
        StopBus();
        temp = ((temp_data & 0xfffc) / 65536.0 * 175.72 - 46.85);
        Temperature = temp * 10;
        i = _READ_HUMI_1_;
    } else if (i == _READ_HUMI_1_) {
        StartBus();
        WriteByte(SlaveAddr | Write);
        WriteByte(HUM_HOLD);
        StartBus();
        WriteByte(SlaveAddr | Read);
        i = _READ_HUMI_2_;
    } else if (i == _READ_HUMI_2_) {
        htu_data = ReadByte();
        htu_data = htu_data << 8;
        AckBus();
        htu_data += ReadByte();
        NoAckBus();
        StopBus();
        htu = ((htu_data & 0xfffc) / 65536.0 * 125.0 - 6.0);
        Humidy = htu * 10;
        i = _READ_TEMP_1_;
    }
}
#endif