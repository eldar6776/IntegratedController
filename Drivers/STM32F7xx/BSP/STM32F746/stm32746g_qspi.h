/**
  ******************************************************************************
  * @file    stm32746g_discovery_qspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32746g_discovery_qspi.c driver.
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
#ifndef __QSPI_H__
#define __QSPI_H__                          FW_BUILD // version



/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "../Components/n25q128a/n25q128a.h"



/* Exported constants --------------------------------------------------------*/ 
/* QSPI Error codes */
#define QSPI_OK                     ((uint8_t)0x00)
#define QSPI_ERROR                  ((uint8_t)0x01)
#define QSPI_BUSY                   ((uint8_t)0x02)
#define QSPI_NOT_SUPPORTED          ((uint8_t)0x04)
#define QSPI_SUSPENDED              ((uint8_t)0x08)


/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE()           __HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE()          __HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CS_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_CLK_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_D0_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_D1_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_D2_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_D3_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_DMA_CLK_ENABLE()       __HAL_RCC_DMA2_CLK_ENABLE()
#define QSPI_FORCE_RESET()          __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()        __HAL_RCC_QSPI_RELEASE_RESET()


/* Definition for QSPI Pins */
#define QSPI_CS_PIN                 GPIO_PIN_6
#define QSPI_CS_GPIO_PORT           GPIOB
#define QSPI_CLK_PIN                GPIO_PIN_2
#define QSPI_CLK_GPIO_PORT          GPIOB
#define QSPI_D0_PIN                 GPIO_PIN_8
#define QSPI_D0_GPIO_PORT           GPIOF
#define QSPI_D1_PIN                 GPIO_PIN_9
#define QSPI_D1_GPIO_PORT           GPIOF
#define QSPI_D2_PIN                 GPIO_PIN_7
#define QSPI_D2_GPIO_PORT           GPIOF
#define QSPI_D3_PIN                 GPIO_PIN_6
#define QSPI_D3_GPIO_PORT           GPIOF

/* Definition for QSPI DMA */
#define QSPI_DMA_INSTANCE           DMA2_Stream7
#define QSPI_DMA_CHANNEL            DMA_CHANNEL_3
#define QSPI_DMA_IRQ                DMA2_Stream7_IRQn
#define QSPI_DMA_IRQ_HANDLER        DMA2_Stream7_IRQHandler

/* N25Q128A13EF840E Micron memory */
/* Size of the flash */
#define QSPI_FLASH_SIZE             23     /* Address bus width to access whole memory space */
#define QSPI_PAGE_SIZE              256


void    MX_QSPI_Init    (void);
uint8_t QSPI_MemMapMode (void);
uint8_t QSPI_GetStatus  (void);
uint8_t QSPI_EraseChip  (uint32_t staddr);
uint8_t QSPI_Erase      (uint32_t staddr, uint32_t enaddr);
uint8_t QSPI_Read       (uint8_t   *pbuf, uint32_t rdaddr, uint32_t size);
uint8_t QSPI_Write      (uint8_t   *pbuf, uint32_t wraddr, uint32_t size);
uint8_t FLASH2QSPI_Copy (uint32_t rdaddr, uint32_t wraddr, uint32_t size);
uint8_t QSPI2QSPI_Copy  (uint32_t rdaddr, uint32_t wraddr, uint32_t size);
uint8_t QSPI2FLASH_Copy (uint32_t rdaddr, uint32_t wraddr, uint32_t size);



#endif /* __STM32746G_DISCOVERY_QSPI_H */

/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
