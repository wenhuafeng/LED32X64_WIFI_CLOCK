/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

/* USER CODE BEGIN Private defines */
#define RECEIVELEN 100
#define USART_DMA_SENDING 1//?送未完成  
#define USART_DMA_SENDOVER 0//?送完成  

typedef struct
{
  uint8_t receive_flag:1;//空?接收??  
  uint8_t dmaSend_flag:1;//?送完成??  
  uint16_t rx_len;//接收?度  
  uint8_t usartDMA_rxBuf[RECEIVELEN];//DMA接收?存  
}USART_RECEIVETYPE;

extern USART_RECEIVETYPE UsartType1;//,UsartType2;

void UsartReceive_IDLE(UART_HandleTypeDef *huart);
void Usart1SendData_DMA(uint8_t *pdata, uint16_t Length);
//void Usart2SendData_DMA(uint8_t *pdata, uint16_t Length);
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
