/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define HTU21D_I2C_SOFTWARE  0
#define HTU21D_I2C_HARDWARE  1
#define WIFI_GET_TIME        1
#define GPS_GET_TIME         0 /* You need to use smt32cubemx configuration to open usart3 */
#define UNIX_TIME_STAMP      0
#define SUPPORT_OLED_DISPLAY 1 /* You need to use smt32cubemx configuration to open i2c2 */

#if (GPS_GET_TIME == 1) && (SUPPORT_OLED_DISPLAY == 1)
#error "The GPS OLED function cannot be turned on at the same time because it shares the same set of IO ports."
#endif
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PIR_INT_Pin GPIO_PIN_0
#define PIR_INT_GPIO_Port GPIOA
#define PIR_INT_EXTI_IRQn EXTI0_IRQn
#define DISP_POWER_Pin GPIO_PIN_1
#define DISP_POWER_GPIO_Port GPIOA
#define HUB_B2_Pin GPIO_PIN_4
#define HUB_B2_GPIO_Port GPIOA
#define HUB_G1_Pin GPIO_PIN_5
#define HUB_G1_GPIO_Port GPIOA
#define HUB_R1_Pin GPIO_PIN_6
#define HUB_R1_GPIO_Port GPIOA
#define HUB_B1_Pin GPIO_PIN_7
#define HUB_B1_GPIO_Port GPIOA
#define GPS_LED_Pin GPIO_PIN_0
#define GPS_LED_GPIO_Port GPIOB
#define GPS_ENABLE_Pin GPIO_PIN_1
#define GPS_ENABLE_GPIO_Port GPIOB
#define WORK_LED_Pin GPIO_PIN_12
#define WORK_LED_GPIO_Port GPIOB
#define HUB_A_Pin GPIO_PIN_13
#define HUB_A_GPIO_Port GPIOB
#define HUB_B_Pin GPIO_PIN_14
#define HUB_B_GPIO_Port GPIOB
#define HUB_C_Pin GPIO_PIN_15
#define HUB_C_GPIO_Port GPIOB
#define HUB_D_Pin GPIO_PIN_8
#define HUB_D_GPIO_Port GPIOA
#define HUB_R2_Pin GPIO_PIN_11
#define HUB_R2_GPIO_Port GPIOA
#define HUB_G2_Pin GPIO_PIN_12
#define HUB_G2_GPIO_Port GPIOA
#define HUB_CA15_Pin GPIO_PIN_15
#define HUB_CA15_GPIO_Port GPIOA
#define HUB_LAT_Pin GPIO_PIN_3
#define HUB_LAT_GPIO_Port GPIOB
#define WIFI_POWER_Pin GPIO_PIN_5
#define WIFI_POWER_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
void SystemClock_Config(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
