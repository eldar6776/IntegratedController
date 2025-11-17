/**
  ******************************************************************************
  * @file    stm32746g_discovery.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions to manage LEDs, 
  *          push-buttons and COM ports available on STM32746G-Discovery
  *          board(MB1191) from STMicroelectronics.
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

/* Dependencies
- stm32f7xx_hal_cortex.c
- stm32f7xx_hal_gpio.c
- stm32f7xx_hal_uart.c
- stm32f7xx_hal_i2c.c
EndDependencies */

/* Includes ------------------------------------------------------------------*/
#include "stm32746g.h"
#include "main.h"


#if (__STM32746G_H__ != FW_BUILD)
    #error "bsp drivers header version mismatch"
#endif

/* Imported Types  -----------------------------------------------------------*/
/* Imported Variables --------------------------------------------------------*/
/* Imported Functions    -----------------------------------------------------*/
/* Private Variables  --------------------------------------------------------*/
static void I2Cx_Error(void);
static void I2Cx_Init(I2C_HandleTypeDef *hi2c);
static void I2Cx_MspInit(I2C_HandleTypeDef *hi2c);
static HAL_StatusTypeDef I2Cx_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials);
static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *hi2c, uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *hi2c, uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
/* Private Macros    ---------------------------------------------------------*/
/* Private Prototypes    -----------------------------------------------------*/
/* Program code   ------------------------------------------------------------*/
/**
  * @brief  Initializes I2C MSP.
  * @param  hi2c : I2C handler
  * @retval None
  */
static void I2Cx_MspInit(I2C_HandleTypeDef *hi2c)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	/* AUDIO and LCD I2C MSP init */

	/*** Configure the GPIOs ***/
	/* Enable GPIO clock */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	/**I2C3 GPIO Configuration    
	PD13     ------> I2C4_SDA
	PD12     ------> I2C4_SCL 
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C4;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C4;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);
    
    GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	/* Peripheral clock enable */
	__HAL_RCC_I2C4_CLK_ENABLE();

	/* Force the I2C peripheral clock reset */
	__HAL_RCC_I2C4_FORCE_RESET();

	/* Release the I2C peripheral clock reset */
	__HAL_RCC_I2C4_RELEASE_RESET();

	/* Enable and set I2Cx Interrupt to a lower priority */
//	HAL_NVIC_SetPriority(I2C4_EV_IRQn, 0x0F, 0);
//	HAL_NVIC_EnableIRQ(I2C4_EV_IRQn);

	/* Enable and set I2Cx Interrupt to a lower priority */
//	HAL_NVIC_SetPriority(I2C4_ER_IRQn, 0x0F, 0);
//	HAL_NVIC_EnableIRQ(I2C4_ER_IRQn);
}



/**
  * @brief  Initializes I2C HAL.
  * @param  hi2c : I2C handler
  * @retval None
  */
static void I2Cx_Init(I2C_HandleTypeDef *hi2c)
{
	if(HAL_I2C_GetState(&hi2c4) == HAL_I2C_STATE_RESET)
	{
		hi2c4.Instance = I2C4;
		hi2c4.Init.Timing           = DISCOVERY_I2Cx_TIMING;
		hi2c4.Init.OwnAddress1      = 0;
		hi2c4.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
		hi2c4.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
		hi2c4.Init.OwnAddress2      = 0;
		hi2c4.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
		hi2c4.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
		/* Init the I2C */
		I2Cx_MspInit(&hi2c4);
		HAL_I2C_Init(&hi2c4);
	}
}

/**
  * @brief  Reads multiple data.
  * @param  hi2c : I2C handler
  * @param  Addr: I2C address
  * @param  Reg: Reg address 
  * @param  MemAddress: Memory address 
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval Number of read data
  */
static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *hi2c,uint8_t Addr,uint16_t Reg,uint16_t MemAddress,uint8_t *Buffer,uint16_t Length)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Read(hi2c, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

   /* Check the communication status */
    if(status != HAL_OK)
    {
        /* Re-Initiaize the I2C Bus */
        if      (hi2c->Instance == I2C4) I2Cx_Error();
        else if (hi2c->Instance == I2C3)  
        {
            __HAL_RCC_I2C3_CLK_DISABLE();
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
            HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
            HAL_I2C_DeInit(&hi2c3);
            TS_IO_Init();
        }
    }
    return status;    
}

/**
  * @brief  Writes a value in a register of the device through BUS in using DMA mode.
  * @param  hi2c : I2C handler
  * @param  Addr: Device address on BUS Bus.  
  * @param  Reg: The target register address to write
  * @param  MemAddress: Memory address 
  * @param  Buffer: The target register value to be written 
  * @param  Length: buffer size to be written
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *hi2c,uint8_t Addr,uint16_t Reg,uint16_t MemAddress,uint8_t *Buffer,uint16_t Length)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Write(hi2c, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

    /* Check the communication status */
    if(status != HAL_OK)
    {
        /* Re-Initiaize the I2C Bus */
        if      (hi2c->Instance == I2C4) I2Cx_Error();
        else if (hi2c->Instance == I2C3)  
        {
            __HAL_RCC_I2C3_CLK_DISABLE();
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
            HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
            HAL_I2C_DeInit(&hi2c3);
            TS_IO_Init();
        }
    }
    return status;
}

/**
  * @brief  Checks if target device is ready for communication. 
  * @note   This function is used with Memory devices
  * @param  hi2c : I2C handler
  * @param  DevAddress: Target device address
  * @param  Trials: Number of trials
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials)
{ 
  return (HAL_I2C_IsDeviceReady(hi2c, DevAddress, Trials, 1000));
}

/**
  * @brief  Manages error callback by re-initializing I2C.
  * @param  hi2c : I2C handler
  * @param  Addr: I2C Address
  * @retval None
  */
static void I2Cx_Error(void)
{
  /* De-initialize the I2C communication bus */
  HAL_I2C_DeInit(&hi2c4);
  
  /* Re-Initialize the I2C communication bus */
  I2Cx_Init(&hi2c4);
}

/*******************************************************************************
                            LINK OPERATIONS
*******************************************************************************/

/******************************** LINK I2C EEPROM *****************************/

/**
  * @brief  Initializes peripherals used by the I2C EEPROM driver.
  * @retval None
  */
void EE_IO_Init(void)
{
  I2Cx_Init(&hi2c4);
}

/**
  * @brief  Write data to I2C EEPROM driver in using DMA channel.
  * @param  DevAddress: Target device address
  * @param  MemAddress: Internal memory address
  * @param  pBuffer: Pointer to data buffer
  * @param  BufferSize: Amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef EE_WriteData(uint16_t DevAddress, uint16_t MemAddress, uint8_t* pBuffer, uint32_t BufferSize)
{
  return (I2Cx_WriteMultiple(&hi2c4, DevAddress, MemAddress, I2C_MEMADD_SIZE_16BIT, pBuffer, BufferSize));
}

/**
  * @brief  Read data from I2C EEPROM driver in using DMA channel.
  * @param  DevAddress: Target device address
  * @param  MemAddress: Internal memory address
  * @param  pBuffer: Pointer to data buffer
  * @param  BufferSize: Amount of data to be read
  * @retval HAL status
  */
HAL_StatusTypeDef EE_ReadData(uint16_t DevAddress, uint16_t MemAddress, uint8_t* pBuffer, uint32_t BufferSize)
{
  return (I2Cx_ReadMultiple(&hi2c4, DevAddress, MemAddress, I2C_MEMADD_SIZE_16BIT, pBuffer, BufferSize));
}

/**
  * @brief  Checks if target device is ready for communication. 
  * @note   This function is used with Memory devices
  * @param  DevAddress: Target device address
  * @param  Trials: Number of trials
  * @retval HAL status
  */
HAL_StatusTypeDef EE_IsDeviceReady(uint16_t DevAddress, uint32_t Trials)
{ 
  return (I2Cx_IsDeviceReady(&hi2c4, DevAddress, Trials));
}

/********************************* LINK TOUCHSCREEN *********************************/

/**
  * @brief  Initializes Touchscreen low level.
  * @retval None
  */
void TS_IO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    
    if(HAL_I2C_GetState(&hi2c3) == HAL_I2C_STATE_RESET)
    {
        hi2c3.Instance = I2C3;
        hi2c3.Init.Timing           = DISCOVERY_I2Cx_TIMING;
        hi2c3.Init.OwnAddress1      = 0;
        hi2c3.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
        hi2c3.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
        hi2c3.Init.OwnAddress2      = 0;
        hi2c3.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
        hi2c3.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
        /* Init the I2C */
        /* AUDIO and LCD I2C MSP init */

        /*** Configure the GPIOs ***/
        /* Enable GPIO clock */
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**I2C3 GPIO Configuration    
        PC9     ------> I2C3_SDA
        PA8     ------> I2C3_SCL 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C3_CLK_ENABLE();

        /* Force the I2C peripheral clock reset */
        __HAL_RCC_I2C3_FORCE_RESET();

        /* Release the I2C peripheral clock reset */
        __HAL_RCC_I2C3_RELEASE_RESET();
        
        HAL_I2C_Init(&hi2c3);
    }
}

/**
  * @brief  Writes a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
    I2Cx_WriteMultiple(&hi2c3, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT,(uint8_t*)&Value, 1);
}

/**
  * @brief  Reads a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @retval Data to be read
  */
uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg)
{
    uint8_t read_value = 0;

    I2Cx_ReadMultiple(&hi2c3, Addr, Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&read_value, 1);
    return read_value;
}

/**
  * @brief  TS delay
  * @param  Delay: Delay in ms
  * @retval None
  */
void TS_IO_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

    
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
