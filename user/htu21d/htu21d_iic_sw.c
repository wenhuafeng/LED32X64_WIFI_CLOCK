#include "main.h"
#if defined(HTU21D_I2C_SOFTWARE) && HTU21D_I2C_SOFTWARE
#include "htu21d_iic_sw.h"
#include <stdint.h>
#include <stdbool.h>

#define HTU21D_I2C_DELAY 2

#define HTU21D_SCL_Pin       GPIO_PIN_6
#define HTU21D_SCL_GPIO_Port GPIOB
#define HTU21D_SDA_Pin       GPIO_PIN_7
#define HTU21D_SDA_GPIO_Port GPIOB

enum SdaIoType {
    HTU21D_SDA_OUTPUT,
    HTU21D_SDA_INPUT,
};

#define I2C_SDA_IN()                      \
    do {                                  \
        HTU21D_SDA_SET(HTU21D_SDA_INPUT); \
    } while (0)
#define I2C_SDA_OUT()                      \
    do {                                   \
        HTU21D_SDA_SET(HTU21D_SDA_OUTPUT); \
    } while (0)
#define I2C_SCL_HIGH()                                                         \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_SET); \
    } while (0)
#define I2C_SCL_LOW()                                                            \
    do {                                                                         \
        HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_RESET); \
    } while (0)
#define I2C_SDA_HIGH()                                                         \
    do {                                                                       \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_SET); \
    } while (0)
#define I2C_SDA_LOW()                                                            \
    do {                                                                         \
        HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_RESET); \
    } while (0)

static void I2C_DelayUs(uint8_t c)
{
    uint16_t count = c;

    while (count--);
}

static void HTU21D_SDA_SET(enum SdaIoType io)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    if (io == HTU21D_SDA_OUTPUT) {
        GPIO_InitStructure.Pin   = HTU21D_SDA_Pin;
        GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);
    } else {
        GPIO_InitStructure.Pin  = HTU21D_SDA_Pin;
        GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        GPIO_InitStructure.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);
    }
}

void I2C_Start(void)
{
    I2C_SDA_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SDA_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
}

void I2C_Stop(void)
{
    I2C_SDA_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SDA_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
}

void I2C_Ack(void)
{
    I2C_SDA_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SDA_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
}

void I2C_Nack(void)
{
    I2C_SDA_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_LOW();
}

void I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.Pin   = HTU21D_SDA_Pin;
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HTU21D_SDA_GPIO_Port, &GPIO_InitStructure);

    GPIO_InitStructure.Pin   = HTU21D_SCL_Pin;
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HTU21D_SCL_GPIO_Port, &GPIO_InitStructure);

    HAL_GPIO_WritePin(HTU21D_SCL_GPIO_Port, HTU21D_SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin, GPIO_PIN_SET);
}

bool I2C_WriteByte(uint8_t send)
{
    uint8_t temp;
    uint8_t i;
    bool ret = false;

    for (i = 0; i < 8; i++) {
        I2C_SCL_LOW();
        if ((send << i) & 0x80) {
            I2C_SDA_HIGH();
        } else {
            I2C_SDA_LOW();
        }
        I2C_DelayUs(HTU21D_I2C_DELAY);
        I2C_SCL_HIGH();
        I2C_DelayUs(HTU21D_I2C_DELAY);
    }

    I2C_SCL_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SDA_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);

    //I2C_DelayUs(HTU21D_I2C_DELAY);
    I2C_SCL_HIGH();
    I2C_DelayUs(HTU21D_I2C_DELAY);

    // read ack
    I2C_SDA_IN();
    temp = 10;
    while (--temp) {
        I2C_DelayUs(HTU21D_I2C_DELAY);
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) == GPIO_PIN_RESET) {
            ret = true;
            break;
        }
    }
    I2C_SDA_OUT();
    I2C_SCL_LOW();
    I2C_DelayUs(HTU21D_I2C_DELAY);
    if (temp == 0) {
        ret = false;
    }

    return ret;
}

uint8_t I2C_ReadByte(void)
{
    uint8_t value = 0;
    uint8_t i;

    I2C_SDA_IN();
    for (i = 0; i < 8; i++) {
        I2C_SCL_HIGH();
        I2C_DelayUs(HTU21D_I2C_DELAY);
        value <<= 1;
        if (HAL_GPIO_ReadPin(HTU21D_SDA_GPIO_Port, HTU21D_SDA_Pin) == GPIO_PIN_SET) {
            value += 1;
        }
        I2C_SCL_LOW();
    }
    I2C_SDA_OUT();

    return value;
}

#endif
