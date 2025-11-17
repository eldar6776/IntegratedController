/**
  ******************************************************************************
  * @file    stm32746g_discovery.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for STM32746G_DISCOVERY's LEDs,
  *          push-buttons and COM ports hardware resources.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32746G_H__
#define __STM32746G_H__                 FW_BUILD // version


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
   

#if !defined (USE_STM32746G_DISCO)
 #define USE_STM32746G_DISCO
#endif

#define TS_I2C_ADDRESS                          ((uint16_t)0x70)

/* I2C clock speed configuration (in Hz) 
   WARNING: 
   Make sure that this define is not already declared in other files (ie. 
   stm32746g_discovery.h file). It can be used in parallel by other modules. */
#ifndef I2C_SPEED
 #define I2C_SPEED                              ((uint32_t)100000)
#endif /* I2C_SPEED */


/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated from APB1 source clock = 50 MHz */
/* Due to the big MOFSET capacity for adapting the camera level the rising time is very large (>1us) */
/* 0x40912732 takes in account the big rising and aims a clock of 100khz */
#ifndef DISCOVERY_I2Cx_TIMING  
#define DISCOVERY_I2Cx_TIMING                      ((uint32_t)0x40912732)  
#endif /* DISCOVERY_I2Cx_TIMING */
/* TOUCHSCREEN IO functions */
void            TS_IO_Init(void);
void            TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
uint8_t         TS_IO_Read(uint8_t Addr, uint8_t Reg);
void            TS_IO_Delay(uint32_t Delay);

/* I2C EEPROM IO function */
void                EE_IO_Init(void);
HAL_StatusTypeDef   EE_IsDeviceReady(uint16_t DevAddress, uint32_t Trials);
HAL_StatusTypeDef   EE_WriteData    (uint16_t DevAddress, uint16_t MemAddress, uint8_t *pBuffer, uint32_t BufferSize);
HAL_StatusTypeDef   EE_ReadData     (uint16_t DevAddress, uint16_t MemAddress, uint8_t *pBuffer, uint32_t BufferSize);
#ifdef __cplusplus
}
#endif

#endif /* __STM32746G_DISCOVERY_H */

/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
