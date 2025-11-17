/**
  ******************************************************************************
  * @file    stm32746g_discovery_eeprom.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage an I2C M24LR64 
  *          EEPROM memory.
  @verbatim
             To be able to use this driver, the switch EE_M24LR64 must be defined
             in your toolchain compiler preprocessor
             
             =================================================================== 
             Notes:
              - The I2C EEPROM memory (M24LR64) is available on separate daughter 
                board ANT7-M24LR-A, which is not provided with the STM32746G_DISCOVERY
                board.
                To use this driver you have to connect the ANT7-M24LR-A to CN3 
                connector of STM32746G_DISCOVERY board.
             ===================================================================
                 
             It implements a high level communication layer for read and write 
             from/to this memory. The needed STM32F7xx hardware resources (I2C and
             GPIO) are defined in stm32746g_discovery.h file, and the initialization is
             performed in EE_IO_Init() function declared in stm32746g_discovery.c
             file.
             You can easily tailor this driver to any other development board, 
             by just adapting the defines for hardware resources and 
             EE_IO_Init() function. 
           
             @note In this driver, basic read and write functions (EE_ReadBuffer() 
                   and EE_WritePage()) use DMA mode to perform the data 
                   transfer to/from EEPROM memory.
   
            @note   Regarding EE_WritePage(), it is an optimized function to perform
                   small write (less than 1 page) BUT the number of bytes (combined to write start address) must not
                   cross the EEPROM page boundary. This function can only writes into
                   the boundaries of an EEPROM page.
                   This function doesn't check on boundaries condition (in this driver 
                   the function EE_WriteBuffer() which calls EE_WritePage() is 
                   responsible of checking on Page boundaries).
    
                
        +-----------------------------------------------------------------+
        |               Pin assignment for M24LR64 EEPROM                 |
        +---------------------------------------+-----------+-------------+
        |  STM32F7xx I2C Pins                   |   EEPROM  |   Pin       |
        +---------------------------------------+-----------+-------------+
        | .                                     |   E0(GND) |    1  (0V)  |
        | .                                     |   AC0     |    2        |
        | .                                     |   AC1     |    3        |
        | .                                     |   VSS     |    4  (0V)  |
        | SDA                                   |   SDA     |    5        |
        | SCL                                   |   SCL     |    6        |
        | .                                     |   E1(GND) |    7  (0V)  |
        | .                                     |   VDD     |    8 (3.3V) |
        +---------------------------------------+-----------+-------------+
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
- stm32746g_discovery.c
EndDependencies */

/* Includes ------------------------------------------------------------------*/
#include "stm32746g_eeprom.h"
#include "main.h"


#if (__EEPROM_H__ != FW_BUILD)
    #error "eeprom driver header version mismatch"
#endif

/* Imported Types  -----------------------------------------------------------*/
/* Imported Variables --------------------------------------------------------*/
/* Imported Functions    -----------------------------------------------------*/
/* Private Variables  --------------------------------------------------------*/
/* Private Macros    ---------------------------------------------------------*/
/* Private Prototypes    -----------------------------------------------------*/
uint32_t EE_WritePage (uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);
/* Program code   ------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void EE_Init(void)
{ 
    /* I2C Initialization */
    EE_IO_Init();
    if (EE_IsDeviceReady(EE_ADDR, EE_MAX_TRIALS) != HAL_OK)  ErrorHandler(EEPROM_FUNC, I2C_DRV);
}
/**
  * @brief  Reads a block of data from the EEPROM.
  * @param  pBuffer: pointer to the buffer that receives the data read from 
  *         the EEPROM.
  * @param  ReadAddr: EEPROM's internal address to start reading from.
  * @param  NumByteToRead: pointer to the variable holding number of bytes to 
  *         be read from the EEPROM.
  * 
  *        @note The variable pointed by NumByteToRead is reset to 0 when all the 
  *              data are read from the EEPROM. Application should monitor this 
  *              variable in order know when the transfer is complete.
  * 
  * @retval EE_OK (0) if operation is correctly performed, else return value 
  *         different from EE_OK (0) or the timeout user callback.
  */
uint32_t EE_ReadBuffer (uint8_t *pBuffer, uint16_t ReadAddr,  uint16_t NumByteToRead)
{  
  uint32_t buffersize = NumByteToRead;
  uint32_t status = EE_OK;
  /* Set the pointer to the Number of data to be read. This pointer will be used 
     by the DMA Transfer Completer interrupt Handler in order to reset the 
     variable to 0. User should check on this variable in order to know if the 
     DMA transfer has been complete or not. */
  
  status = EE_ReadData (EE_ADDR, ReadAddr, pBuffer, buffersize);

  /* If all operations OK, return EE_OK (0) */
  return status;
}

/**
  * @brief  Writes buffer of data to the I2C EEPROM.
  * @param  pBuffer: pointer to the buffer  containing the data to be written 
  *         to the EEPROM.
  * @param  WriteAddr: EEPROM's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the EEPROM.
  * @retval EE_OK (0) if operation is correctly performed, else return value 
  *         different from EE_OK (0) or the timeout user callback.
  */
uint32_t EE_WriteBuffer(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t numofpage = 0, numofsingle = 0, count = 0;
    uint16_t addr = 0;
    uint16_t  dataindex = 0;
    uint32_t status = EE_OK;

    addr = WriteAddr % EE_PGSIZE;
    count = EE_PGSIZE - addr;
    numofpage =  NumByteToWrite / EE_PGSIZE;
    numofsingle = NumByteToWrite % EE_PGSIZE;

    /* If WriteAddr is EE_PGSIZE aligned */
    if (!addr) 
    {
        /* If NumByteToWrite < EE_PGSIZE */
        if (!numofpage) 
        {
            dataindex = numofsingle;// Store the number of data to be written    
            /* Start writing data */
            status = EE_WritePage(pBuffer, WriteAddr, dataindex);
            if (status != EE_OK) return status;
        }
        /* If NumByteToWrite > EE_PGSIZE */
        else  
        {
            while (numofpage--)
            {
                dataindex = EE_PGSIZE;// Store the number of data to be written    
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
                WriteAddr +=  EE_PGSIZE;
                pBuffer += EE_PGSIZE;
            }

            if (numofsingle)
            {
                /* Store the number of data to be written */
                dataindex = numofsingle;          
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
            }
        }
    }
    /* If WriteAddr is not EE_PGSIZE aligned */
    else 
    {
        /* If NumByteToWrite < EE_PGSIZE */
        if (!numofpage) 
        {
            /* If the number of data to be written is more than the remaining space in the current page: */
            if(NumByteToWrite > count)
            {
                dataindex = count;// Store the number of data to be written    
                /* Write the data contained in same page */
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
                /* Store the number of data to be written */
                dataindex = (NumByteToWrite - count);          
                /* Write the remaining data in the following page */
                status = EE_WritePage((uint8_t*)(pBuffer + count), (WriteAddr + count), dataindex);
                if (status != EE_OK)  return status;
            }      
            else      
            {
                dataindex = numofsingle;// Store the number of data to be written    
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
            }     
        }
        /* If NumByteToWrite > EE_PGSIZE */
        else
        {
            NumByteToWrite -= count;
            numofpage =  NumByteToWrite / EE_PGSIZE;
            numofsingle = NumByteToWrite % EE_PGSIZE;
            /* Store the number of data to be written */
            if (count)
            {
                dataindex = count;  // Store the number of data to be written    
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
                WriteAddr += count;
                pBuffer += count;
            }
            
            while(numofpage--)
            {
                dataindex = EE_PGSIZE; // Store the number of data to be written        
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
                WriteAddr +=  EE_PGSIZE;
                pBuffer += EE_PGSIZE;  
            }
            
            if(numofsingle)
            {
                /* Store the number of data to be written */
                dataindex = numofsingle;           
                status = EE_WritePage(pBuffer, WriteAddr, dataindex);
                if (status != EE_OK)  return status;
            }
        }
    }
    /* If all operations OK, return EE_OK (0) */
    return EE_OK;
}
/**
  * @brief  Writes more than one byte to the EEPROM with a single WRITE cycle.
  *
  * @note   The number of bytes (combined to write start address) must not 
  *         cross the EEPROM page boundary. This function can only write into
  *         the boundaries of an EEPROM page.
  *         This function doesn't check on boundaries condition (in this driver 
  *         the function EE_WriteBuffer() which calls EE_WritePage() is 
  *         responsible of checking on Page boundaries).
  * 
  * @param  pBuffer: pointer to the buffer containing the data to be written to 
  *         the EEPROM.
  * @param  WriteAddr: EEPROM's internal address to write to.
  * @param  NumByteToWrite: pointer to the variable holding number of bytes to 
  *         be written into the EEPROM. 
  * 
  *        @note The variable pointed by NumByteToWrite is reset to 0 when all the 
  *              data are written to the EEPROM. Application should monitor this 
  *              variable in order know when the transfer is complete.
  * 
  *        @note This function just configure the communication and enable the DMA 
  *              channel to transfer data. Meanwhile, the user application may perform 
  *              other tasks in parallel.
  * 
  * @retval EE_OK (0) if operation is correctly performed, else return value 
  *         different from EE_OK (0) or the timeout user callback.
  */
uint32_t EE_WritePage(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{ 
    uint32_t buffersize = NumByteToWrite;
    uint32_t status = EE_OK;
    /* Set the pointer to the Number of data to be written. This pointer will be used 
    by the DMA Transfer Completer interrupt Handler in order to reset the 
    variable to 0. User should check on this variable in order to know if the 
    DMA transfer has been complete or not. */
    status = EE_WriteData (EE_ADDR, WriteAddr, pBuffer, buffersize); 
    if (status != EE_OK)  return status;
    status = EE_IsDeviceReady(EE_ADDR, EE_MAX_TRIALS);
    return status;
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
