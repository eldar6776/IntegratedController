/**
  ******************************************************************************
  * @file    stm32746g_discovery_qspi.c
  * @author  MCD Application Team
  * @brief   This file includes a standard driver for the N25Q128A QSPI
  *          memory mounted on STM32746G-Discovery board.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================  
  [..] 
   (#) This driver is used to drive the N25Q128A QSPI external
       memory mounted on STM32746G-Discovery board.
       
   (#) This driver need a specific component driver (N25Q128A) to be included with.

   (#) Initialization steps:
       (++) Initialize the QPSI external memory using the BSP_QSPI_Init() function. This 
            function includes the MSP layer hardware resources initialization and the
            QSPI interface with the external memory.
  
   (#) QSPI memory operations
       (++) QSPI memory can be accessed with read/write operations once it is
            initialized.
            Read/write operation can be performed with AHB access using the functions
            QSPI_Read()/BSP_QSPI_Write(). 
       (++) The function QSPI_GetInfo() returns the configuration of the QSPI memory. 
            (see the QSPI memory data sheet)
       (++) Perform erase block operation using the function QSPI_BlockErase() and by
            specifying the block address. You can perform an erase operation of the whole 
            chip by calling the function QSPI_EraseChip(). 
       (++) The function QSPI_GetStatus() returns the current status of the QSPI memory. 
            (see the QSPI memory data sheet)
  @endverbatim
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
- stm32f7xx_hal_qspi.c
- stm32f7xx_hal_gpio.c
- stm32f7xx_hal_cortex.c
- stm32f7xx_hal_rcc_ex.h
- n25q128a.h
EndDependencies */

#if (__QSPI_H__ != FW_BUILD)
    #error "qspi driver header version mismatch"
#endif
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32746g.h"
#include "stm32746g_qspi.h"
/* Private variables ---------------------------------------------------------*/
/** @defgroup STM32746G_DISCOVERY_QSPI_Private_Variables STM32746G_DISCOVERY QSPI Private Variables
  * @{
  */       
static QSPI_CommandTypeDef sCommand;
static QSPI_AutoPollingTypeDef sConfig;
static uint8_t MemMapModeState;


/* Private functions ---------------------------------------------------------*/
/** @defgroup STM32746G_DISCOVERY_QSPI_Private_Functions STM32746G_DISCOVERY QSPI Private Functions
  * @{
  */ 
static void QSPI_ResetMemory        (void);
static uint8_t QSPI_DummyCyclesCfg  (void);
static uint8_t QSPI_WriteEnable     (void);
static uint8_t FLASH_GetSector      (uint32_t addr);
static uint8_t QSPI_EraseSector     (uint32_t staddr);
static uint8_t QSPI_AutoPollMemRdy  (uint32_t Timeout);
static uint8_t QSPI_WritePage       (uint32_t addr, uint32_t size, uint8_t *buff);


/** @defgroup STM32746G_DISCOVERY_QSPI_Exported_Functions STM32746G_DISCOVERY QSPI Exported Functions
  * @{
  */ 
/**
  * @brief  Initializes the QSPI interface.
  * @retval QSPI memory status
  */
void MX_QSPI_Init(void)
{ 
    MemMapModeState = 0U;
    memset(&hqspi,   0, sizeof(hqspi));
    memset(&sCommand,0, sizeof(sCommand));
    memset(&sConfig, 0, sizeof(sConfig));

    hqspi.Instance = QUADSPI;	  
    HAL_QSPI_DeInit(&hqspi);
    hqspi.Init.ClockPrescaler      = 2; /* QSPI freq = 216 MHz/(1+1) = 108 Mhz */
    hqspi.Init.FifoThreshold       = 4;
    hqspi.Init.SampleShifting      = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize           = QSPI_FLASH_SIZE; //POSITION_VAL(N25Q128A_FLASH_SIZE) - 1;
    hqspi.Init.ChipSelectHighTime  = QSPI_CS_HIGH_TIME_2_CYCLE; /* Min 50ns for nonRead */
    hqspi.Init.ClockMode           = QSPI_CLOCK_MODE_0;
    hqspi.Init.DualFlash           = QSPI_DUALFLASH_DISABLE;
    hqspi.Init.FlashID             = QSPI_FLASH_ID_1;

    HAL_QSPI_Init(&hqspi);
    QSPI_ResetMemory();
    QSPI_AutoPollMemRdy(HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
}
/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  pbuf: Pointer to data to be read
  * @param  rdaddr: Read start address
  * @param  Size: Size of data to read    
  * @retval QSPI memory read status
  */
uint8_t QSPI_Read   (uint8_t *pbuf, uint32_t rdaddr, uint32_t size)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = QUAD_INOUT_FAST_READ_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = rdaddr & 0x0FFFFFFFU;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = N25Q128A_DUMMY_CYCLES_READ_QUAD;
    s_command.NbData            = size;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Set S# timing for Read command */
    MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_3_CYCLE);

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, pbuf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Restore S# timing for nonRead commands */
    MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_6_CYCLE);

    return QSPI_OK;
}
/**
  * @brief  Write an amount of data to the QSPI memory.
  * @param  buff: Pointer to data to be written
  * @param  wraddr: Write start address
  * @param  size: Size of data to read    
  * @retval QSPI memory write status
  */
uint8_t QSPI_Write  (uint8_t *pbuf, uint32_t wraddr, uint32_t size)
{
    uint32_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
    uint32_t   QSPI_DataNum = 0;

    Addr = wraddr % QSPI_PAGE_SIZE;
    count = QSPI_PAGE_SIZE - Addr;
    NumOfPage =  size / QSPI_PAGE_SIZE;
    NumOfSingle = size % QSPI_PAGE_SIZE;

    if (Addr == 0) /*!< wraddr is QSPI_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /*!< NumByteToWrite < QSPI_PAGESIZE */
        {
            QSPI_DataNum = size;      
            if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
        }
        else /*!< size > QSPI_PAGESIZE */
        {
            while (NumOfPage--)
            {
                QSPI_DataNum = QSPI_PAGE_SIZE;
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
                wraddr +=  QSPI_PAGE_SIZE;
                pbuf += QSPI_PAGE_SIZE;
            }

            QSPI_DataNum = NumOfSingle;
            if(QSPI_DataNum > 0) 
            {
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
            }
        }
    }
    else /*!< wraddr is not QSPI_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /*!< size < QSPI_PAGESIZE */
        {
            if (NumOfSingle > count) /*!< (size + wraddr) > QSPI_PAGESIZE */
            {
                temp = NumOfSingle - count;
                QSPI_DataNum = count;
                QSPI_WritePage(wraddr, QSPI_DataNum, pbuf);
                wraddr +=  count;
                pbuf += count;
                QSPI_DataNum = temp;
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
            }
            else
            {
                QSPI_DataNum = size; 
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
            }
        }
        else /*!< size > QSPI_PAGESIZE */
        {
            size -= count;
            NumOfPage =  size / QSPI_PAGE_SIZE;
            NumOfSingle = size % QSPI_PAGE_SIZE;
            QSPI_DataNum = count;
            if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
            wraddr +=  count;
            pbuf += count;

            while (NumOfPage--)
            {
                QSPI_DataNum = QSPI_PAGE_SIZE;
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
                wraddr +=  QSPI_PAGE_SIZE;
                pbuf += QSPI_PAGE_SIZE;
            }

            if (NumOfSingle != 0)
            {
                QSPI_DataNum = NumOfSingle;
                if (QSPI_WritePage(wraddr, QSPI_DataNum, pbuf) != QSPI_OK) return (QSPI_ERROR);
            }
        }
    }

    return (QSPI_OK);
}
/**
  * Description         : Erase a full 64kb sector in the device 										        
  * Inputs 	            : Start of sector 														        
  * EndSectrorAddress	: End of sector 																	
  * outputs 	        :																				
  * "1" 		        : Operation succeeded	
  * Note : Not Mandatory for SRAM PSRAM and NOR_FLASH
  */
uint8_t QSPI_Erase  (uint32_t staddr, uint32_t enaddr)
{      
    uint32_t blkaddr = (staddr - (staddr % N25Q128A_SECTOR_SIZE));

    while (blkaddr <= enaddr)
    {
        if (QSPI_EraseSector(blkaddr) != QSPI_OK) return QSPI_ERROR;
        blkaddr += N25Q128A_SECTOR_SIZE;
#ifdef	USE_WATCHDOG
        HAL_IWDG_Refresh(&hiwdg);
#endif  
    }
    return QSPI_OK;	
}
/**
  * @brief  Reads current status of the QSPI memory.
  * @retval QSPI memory status
  */
uint8_t QSPI_GetStatus(void)
{
    QSPI_CommandTypeDef s_command;
    uint8_t reg;

    /* Initialize the read flag status register command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_FLAG_STATUS_REG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 1;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Check the value of the register */
    if ((reg & (N25Q128A_FSR_PRERR | N25Q128A_FSR_VPPERR | N25Q128A_FSR_PGERR | N25Q128A_FSR_ERERR)) != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    else if ((reg & (N25Q128A_FSR_PGSUS | N25Q128A_FSR_ERSUS)) != QSPI_OK)
    {
        return QSPI_SUSPENDED;
    }
    else if ((reg & N25Q128A_FSR_READY) != QSPI_OK)
    {
        return QSPI_OK;
    }
    return QSPI_BUSY;
}
/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @retval QSPI memory status
  */
uint8_t QSPI_MemMapMode(void)
{
    if (MemMapModeState == 0U)	
    {	
        QSPI_MemoryMappedTypeDef sMemMappedCfg;

        if (QSPI_DummyCyclesCfg() != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        sCommand.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

        sCommand.AddressMode        = QSPI_ADDRESS_4_LINES;
        sCommand.Address            = 0;
        sCommand.DataMode           = QSPI_DATA_4_LINES;
        sCommand.NbData             = 0; // undefined length until end of memory
        sCommand.Instruction        = QUAD_INOUT_FAST_READ_CMD;
        sCommand.DummyCycles        = N25Q128A_DUMMY_CYCLES_READ_QUAD;

        sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

        if (HAL_QSPI_MemoryMapped(&hqspi, &sCommand, &sMemMappedCfg) != HAL_OK)
        {
            return QSPI_ERROR;
        }

        MemMapModeState = 1U;
    }
    return QSPI_OK;
}
/**
  * @} if this function fail, qspi interface will stay in indirect mode
  */
uint8_t FLASH2QSPI_Copy (uint32_t rdaddr, uint32_t wraddr, uint32_t size)
{
    uint8_t  buff[QSPI_PAGE_SIZE];
    uint32_t bcnt           = 0U;
    uint32_t scnt           = size;
    uint32_t FirstLoadAddr  = rdaddr;
    uint32_t FirstWriteAddr = wraddr;
    uint32_t LastWriteAddr  = wraddr + size;
    MX_QSPI_Init(); // set qspi interface for indirect r/w access
	if (QSPI_Erase(FirstWriteAddr, LastWriteAddr) != QSPI_OK) return (QSPI_ERROR); 
	while(scnt)                                             // erase space for new file
	{                                                       // copy data from mcu flash external qspi flash
		if (scnt >= QSPI_PAGE_SIZE) bcnt = QSPI_PAGE_SIZE;  // set page size to maximum 256 bytes for single page write
		else bcnt = scnt;                                   // or set to last buffer remaing size to finish last cycle 
        memcpy(buff, (uint8_t*)FirstLoadAddr, bcnt);        // copy from mcu flash data source to temp buffer for exchange
        if (QSPI_Write (buff, FirstWriteAddr, bcnt) != QSPI_OK) return (QSPI_ERROR); // and write from temp buffer to qspi flash
        FirstWriteAddr += bcnt; // increase addresse for read and write data 
        FirstLoadAddr += bcnt;  // counters to prepare next cycle
        scnt -= bcnt;   // and decrease for total data counter
	}                   // till now everything ok!  
    MX_QSPI_Init();     // after write process succesfully finished set qspi 
    QSPI_MemMapMode();  // flash to memory mapped fast read mode and perform data check
    scnt = memcmp((uint8_t*)rdaddr, (uint8_t*)wraddr, size); // use trusty c function for comparision
    if (scnt != 0x00U) return (QSPI_ERROR); // if there is data difference send error to calling process
    return (QSPI_OK);   // to try again or to select different source, also return copy success flag
}
/**
  * @} if this function fail, qspi interface will stay in indirect mode
  */
uint8_t QSPI2QSPI_Copy  (uint32_t rdaddr, uint32_t wraddr, uint32_t size)
{
    uint8_t  buff[QSPI_PAGE_SIZE];
    uint32_t bcnt           = 0U;
    uint32_t scnt           = size;
    uint32_t FirstLoadAddr  = rdaddr;
    uint32_t FirstWriteAddr = wraddr;
    uint32_t LastWriteAddr  = wraddr + size;
    MX_QSPI_Init();     // set qspi interface for indirect r/w access
	if (QSPI_Erase(FirstWriteAddr, LastWriteAddr) != QSPI_OK) return (QSPI_ERROR); // erase space for new file
    while(scnt)                                             
	{                                                       // copy data from mcu flash external qspi flash
		if (scnt >= QSPI_PAGE_SIZE) bcnt = QSPI_PAGE_SIZE;  // set page size to maximum 256 bytes for single page write
		else bcnt = scnt;                                   // or set to last buffer remaing size to finish last cycle 
        if (QSPI_Read  (buff, FirstLoadAddr,  bcnt) != QSPI_OK) return (QSPI_ERROR); // copy data from qspi flash to buffer
        if (QSPI_Write (buff, FirstWriteAddr, bcnt) != QSPI_OK) return (QSPI_ERROR); // and write from temp buffer to qspi flash
        FirstWriteAddr += bcnt; // increase addresse for read and write data 
        FirstLoadAddr += bcnt;  // counters to prepare next cycle
        scnt -= bcnt;   // and decrease for total data counter
	}                   // till now everything ok!  
    MX_QSPI_Init();     // after write process succesfully finished set qspi flash to memory mapped 
    QSPI_MemMapMode();  // mode for fast read to perform data check with another c libraries old timer
    scnt = memcmp((uint8_t*)rdaddr, (uint8_t*)wraddr, size); // use trusty c function for comparision
    if (scnt != 0x00U) return (QSPI_ERROR); // if there is data difference send error to calling process
    return (QSPI_OK);   // to try again or to select different source, also return copy success flag
}
/**
  * @}
  */
uint8_t QSPI2FLASH_Copy (uint32_t rdaddr, uint32_t wraddr, uint32_t size)
{
    uint32_t bcnt               = 0U;
    uint32_t stat               = 0U;
    FLASH_EraseInitTypeDef        FLASH_EraseInit;
    FLASH_EraseInit.TypeErase   = FLASH_TYPEERASE_SECTORS;
    FLASH_EraseInit.VoltageRange= FLASH_VOLTAGE_RANGE_3;
    FLASH_EraseInit.Sector      = FLASH_GetSector(wraddr);
    FLASH_EraseInit.NbSectors   = FLASH_GetSector(wraddr + size) - FLASH_EraseInit.Sector + 1U;  
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock(); // unlock flash conotroll register for access and erase memory space for data write to follow. if error occured
    if (HAL_FLASHEx_Erase (&FLASH_EraseInit, &stat) != HAL_OK) return (uint8_t)(stat & 0xFFU); // durring errase, return sector number
    while (bcnt < size) // if errase process passed ok, copy 32 bit word data from qspi flas to mcu flash one by one 
    {   // and if process fail send error flag to caller function. Error during bootloader update need recovery action to involve  
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,(wraddr+bcnt),*(__IO uint32_t*)(rdaddr+bcnt))!=HAL_OK)return(QSPI_ERROR);
        bcnt += 4U; // address is increased by 4 because of 32 bit = 4 byte data access
    } // After copy loop succesfully finished, lock the Flash to disable the flash control register access
    HAL_FLASH_Lock(); // to protect the FLASH memory against possible unwanted operation)
    stat = memcmp((uint8_t*)rdaddr, (uint8_t*)wraddr, size); // finaly, compare two memory data for possible error
    if (stat != 0x0U) return (QSPI_ERROR); // if there is data difference send error to calling process
    return (QSPI_OK); // to try again or to select different source, also return copy success flag
}
/**
  * @brief QSPI MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - NVIC configuration for QSPI interrupt
  * @retval None
  */
HAL_StatusTypeDef HAL_QSPI_DeInit(QSPI_HandleTypeDef *hqspi)
{
    /* Check the QSPI handle allocation */
    if(hqspi->Instance == NULL)
    {
        return HAL_ERROR;
    }

    /* Process locked */
    __HAL_LOCK(hqspi);

    /* Disable the QSPI Peripheral Clock */
    __HAL_QSPI_DISABLE(hqspi);

    /* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
    HAL_QSPI_MspDeInit(hqspi);

    /* Set QSPI error code to none */
    hqspi->ErrorCode = HAL_QSPI_ERROR_NONE;

    /* Initialize the QSPI state */
    hqspi->State = HAL_QSPI_STATE_RESET;

    /* Release Lock */
    __HAL_UNLOCK(hqspi);

    return HAL_OK;
}
/**
  * @brief QSPI MSP Init
  * @param hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct;
//    static DMA_HandleTypeDef hdma;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable the QuadSPI memory interface clock */
    QSPI_CLK_ENABLE();
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();
    /* Enable GPIO clocks */
    QSPI_CS_GPIO_CLK_ENABLE();
    QSPI_CLK_GPIO_CLK_ENABLE();
    QSPI_D0_GPIO_CLK_ENABLE();
    QSPI_D1_GPIO_CLK_ENABLE();
    QSPI_D2_GPIO_CLK_ENABLE();
    QSPI_D3_GPIO_CLK_ENABLE();
    /* Enable DMA clock */
//    QSPI_DMA_CLK_ENABLE();   

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* QSPI CS GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_CS_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(QSPI_CS_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI CLK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_CLK_PIN;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D0 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D0_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(QSPI_D0_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D1 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D1_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(QSPI_D1_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D2 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D2_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(QSPI_D2_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D3 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D3_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(QSPI_D3_GPIO_PORT, &GPIO_InitStruct);

    /*##-3- Configure the NVIC for QSPI #########################################*/
    /* NVIC configuration for QSPI interrupt */
    HAL_NVIC_SetPriority(QUADSPI_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);
}
/**
  * @brief QSPI MSP DeInit
  * @param hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *hqspi)
{
    static DMA_HandleTypeDef hdma;

    /*##-1- Disable the NVIC for QSPI and DMA ##################################*/
    HAL_NVIC_DisableIRQ(QSPI_DMA_IRQ);
    HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

    /*##-2- Disable peripherals ################################################*/
    /* De-configure DMA channel */
    hdma.Instance = QSPI_DMA_INSTANCE;
    HAL_DMA_DeInit(&hdma);
    /* De-Configure QSPI pins */
    HAL_GPIO_DeInit(QSPI_CS_GPIO_PORT, QSPI_CS_PIN);
    HAL_GPIO_DeInit(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN);
    HAL_GPIO_DeInit(QSPI_D0_GPIO_PORT, QSPI_D0_PIN);
    HAL_GPIO_DeInit(QSPI_D1_GPIO_PORT, QSPI_D1_PIN);
    HAL_GPIO_DeInit(QSPI_D2_GPIO_PORT, QSPI_D2_PIN);
    HAL_GPIO_DeInit(QSPI_D3_GPIO_PORT, QSPI_D3_PIN);

    /*##-3- Reset peripherals ##################################################*/
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();

    /* Disable the QuadSPI memory interface clock */
    QSPI_CLK_DISABLE();
}
/**
  * @brief  This function reset the QSPI memory.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static void QSPI_ResetMemory(void)
{
        /* Reset memory config, Cmd in 1 line */
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2166;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2199;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();

    /* Reset memory config, Cmd in 2 lines*/
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2266;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2299;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();

    /* Reset memory config, Cmd in 4 lines*/
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2366;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(hqspi.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */
    hqspi.Instance->CCR = 0x2399;
    hqspi.Instance->AR = 0;
    hqspi.Instance->ABR = 0;
    hqspi.Instance->DLR = 0;
    __DSB();
}
/**
  * @brief  This function configure the dummy cycles on memory side.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_DummyCyclesCfg(void)
{
    int rg;
    uint8_t reg;
    /* Read Volatile Configuration register --------------------------- */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_VOL_CFG_REG_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.NbData            = 1;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Enable write operations ---------------------------------------- */
    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    rg = (uint8_t) reg;
    /* Write Volatile Configuration register (with new dummy cycles) -- */  
    sCommand.Instruction = WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(rg, N25Q128A_VCR_NB_DUMMY, (N25Q128A_DUMMY_CYCLES_READ_QUAD << POSITION_VAL(N25Q128A_VCR_NB_DUMMY)));
    reg = (uint8_t) rg;
    
    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    if (HAL_QSPI_Transmit(&hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_WriteEnable(void)
{
    /* Enable write operations ------------------------------------------ */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = WRITE_ENABLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for write enabling ---- */    
    sConfig.Match           = 0x02U;
    sConfig.Mask            = 0x02U;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10U;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    sCommand.Instruction    = READ_STATUS_REG_CMD;
    sCommand.DataMode       = QSPI_DATA_1_LINE;
    sCommand.NbData         = 1;

    if (HAL_QSPI_AutoPolling(&hqspi, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hqspi: QSPI handle
  * @param  Timeout
  * @retval None
  */
static uint8_t QSPI_AutoPollMemRdy(uint32_t Timeout)
{
    /* Configure automatic polling mode to wait for memory ready */  
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_STATUS_REG_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    sConfig.Match              = 0x00U;
    sConfig.Mask               = 0x01U;
    sConfig.MatchMode          = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize    = 1;
    sConfig.Interval           = 0x10U;
    sConfig.AutomaticStop      = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(&hqspi, &sCommand, &sConfig, Timeout) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/**
  * @brief  This function write page size buffer of data to qspi flash
  * @param  addr: start write address
  * @param  size: number of bytes to write
  * @retval buff: pointer to data source buffer
  */
static uint8_t QSPI_WritePage (uint32_t addr, uint32_t size, uint8_t *buff)
{

    QSPI_CommandTypeDef s_command;
    
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = EXT_QUAD_IN_FAST_PROG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = addr & 0x0FFFFFFFU;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.NbData            = size;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


    /* Enable write operations */
    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, buff, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for end of program */  
    if (QSPI_AutoPollMemRdy(HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t FLASH_GetSector(uint32_t addr)
{
    if      ((addr < RT_ADDR_FLSECT_1) && (addr >= RT_ADDR_FLSECT_0)) return (uint8_t)(FLASH_SECTOR_0 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_2) && (addr >= RT_ADDR_FLSECT_1)) return (uint8_t)(FLASH_SECTOR_1 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_3) && (addr >= RT_ADDR_FLSECT_2)) return (uint8_t)(FLASH_SECTOR_2 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_4) && (addr >= RT_ADDR_FLSECT_3)) return (uint8_t)(FLASH_SECTOR_3 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_5) && (addr >= RT_ADDR_FLSECT_4)) return (uint8_t)(FLASH_SECTOR_4 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_6) && (addr >= RT_ADDR_FLSECT_5)) return (uint8_t)(FLASH_SECTOR_5 & 0xFFU);
    else if ((addr < RT_ADDR_FLSECT_7) && (addr >= RT_ADDR_FLSECT_6)) return (uint8_t)(FLASH_SECTOR_6 & 0xFFU);
    else if ((addr < FLASH_END_ADDR)   && (addr >= RT_ADDR_FLSECT_7)) return (uint8_t)(FLASH_SECTOR_7 & 0xFFU);
    return  (0xFFU);
}
/**
  * @brief  Erases the specified block of the QSPI memory. 
  * @param  BlockAddress: Block address to erase  
  * @retval QSPI memory status
  */
static uint8_t QSPI_EraseSector (uint32_t staddr)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = SECTOR_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = staddr & 0x0FFFFFFFU;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for end of erase */  
    if (QSPI_AutoPollMemRdy(N25Q128A_SECTOR_ERASE_MAX_TIME) != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/**
  * @brief  Erases the specified block of the QSPI memory. 
  * @param  BlockAddress: Block address to erase  
  * @retval QSPI memory status
  */
uint8_t QSPI_EraseChip(uint32_t staddr)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = BULK_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = staddr & 0x0FFFFFFFU;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for end of erase */  
    if (QSPI_AutoPollMemRdy(N25Q128A_BULK_ERASE_MAX_TIME) != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
