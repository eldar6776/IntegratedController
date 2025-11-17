/**
 ******************************************************************************
 * File Name          : main.c
 * Date               : 03.03.2019.
 * Description        : Thermostat Bootloader Code
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

#if (__MAIN_H__ != FW_BUILD)
    #error "main header version mismatch"
#endif

#ifndef ROOM_THERMOSTAT
    #error "room thermostat not selected for application in common.h"
#endif

#ifndef BOOTLOADER
    #error "bootloader not selected for application type in common.h"
#endif
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32746g_qspi.h"
/* Imported Type  ------------------------------------------------------------*/
/* Imported Variable  --------------------------------------------------------*/
/* Imported Function  --------------------------------------------------------*/
/* Private Define ------------------------------------------------------------*/
/* Private Type --------------------------------------------------------------*/
CRC_HandleTypeDef hcrc;
DMA_HandleTypeDef hdma;
QSPI_HandleTypeDef hqspi;
IWDG_HandleTypeDef hiwdg;
FwInfoTypeDef RunFwInfo;
FwInfoTypeDef NewFwInfo;
FwInfoTypeDef BkpFwInfo;
/* Private Variable ----------------------------------------------------------*/
uint8_t runfw = 0x0U;
uint8_t newfw = 0x0U;
uint8_t bkpfw = 0x0U;
uint8_t updfw = 0x0U;
/* Private Function Prototype ------------------------------------------------*/
void MX_IWDG_Init(void);
void HAL_Deinit(void);
void MX_CRC_Init(void);
void MX_CRC_DeInit(void);
void CPU_CACHE_Enable(void);
void SystemClock_Config(void);
void RunApplication(uint32_t addr);
/* Program Code  -------------------------------------------------------------*/
int main(void)
{
    /* Enable the CPU Cache */
    CPU_CACHE_Enable();
	HAL_Init(); 
	SystemClock_Config();
#ifdef	USE_WATCHDOG
	MX_IWDG_Init();
#endif    
    MX_CRC_Init();
	MX_QSPI_Init();
    QSPI_MemMapMode();
    ResetFwInfo(&RunFwInfo);
    ResetFwInfo(&NewFwInfo);
    RunFwInfo.ld_addr = RT_APPL_ADDR;
    NewFwInfo.ld_addr = RT_NEW_FILE_ADDR;
    runfw = GetFwInfo (&RunFwInfo); // working version info
    newfw = GetFwInfo (&NewFwInfo); // new file version info
    updfw = IsNewFwUpdate(&RunFwInfo, &NewFwInfo); // check is new firmware update
    
    if (!updfw)
    {   /* create application backup */
        if (FLASH2QSPI_Copy (RunFwInfo.ld_addr, RT_APPL_BKP_ADDR, RunFwInfo.size) != QSPI_OK)  Restart();
        MX_QSPI_Init(); // reinit interface again to 
        QSPI_MemMapMode(); // reinit qspi interface to execute sector erase command
        /* if valid backup created, replace running app with new firmware */
#ifdef	USE_WATCHDOG
		HAL_IWDG_Refresh(&hiwdg);
#endif
        if (QSPI2FLASH_Copy (NewFwInfo.ld_addr, NewFwInfo.wr_addr, NewFwInfo.size) != QSPI_OK) Restart();
        MX_QSPI_Init(); // reinit interface again to 
        QSPI_MemMapMode(); // reinit qspi interface to execute sector erase command
#ifdef	USE_WATCHDOG
		HAL_IWDG_Refresh(&hiwdg);
#endif
    }
    else if (runfw)
    {
        ResetFwInfo(&BkpFwInfo);
        BkpFwInfo.ld_addr = RT_APPL_BKP_ADDR;
        bkpfw = GetFwInfo (&BkpFwInfo); // backup of working firmware
        if (!bkpfw) QSPI2FLASH_Copy (BkpFwInfo.ld_addr, BkpFwInfo.wr_addr, BkpFwInfo.size);
        else if (!newfw) QSPI2FLASH_Copy (NewFwInfo.ld_addr, NewFwInfo.wr_addr, NewFwInfo.size); 
        
    }
        
    if (!runfw) RunApplication(RunFwInfo.wr_addr);
    RunApplication(RT_APPL_ADDR);
#ifdef	USE_WATCHDOG
    HAL_IWDG_Refresh(&hiwdg);
    while(1)
    {
    }
#endif 
    DelayMs(10000U); // slow down loop to prevent exscessive flash erase/write cycles
}
//int main(void){
//    /* Enable the CPU Cache */
//    CPU_CACHE_Enable();
//	HAL_Init(); 
//	SystemClock_Config();
//#ifdef	USE_WATCHDOG
//	MX_IWDG_Init();
//#endif    
//    MX_CRC_Init();
//	MX_QSPI_Init();
//    QSPI_MemMapMode();
//    ResetFwInfo(&RunFwInfo);
//    ResetFwInfo(&NewFwInfo);
//    RunFwInfo.ld_addr = RT_APPL_ADDR;
//    NewFwInfo.ld_addr = RT_NEW_FILE_ADDR;
//    runfw = GetFwInfo (&RunFwInfo); // working version info
//    newfw = GetFwInfo (&NewFwInfo); // new file version info
//    updfw = IsNewFwUpdate(&RunFwInfo, &NewFwInfo); // check is new firmware update    
//    if (!updfw){
//        /* create application backup */
//        if (FLASH2QSPI_Copy (RunFwInfo.ld_addr, RT_APPL_BKP_ADDR, RunFwInfo.size) != QSPI_OK){
//            Restart();
//        }
//        MX_QSPI_Init(); // reinit interface again to 
//        QSPI_MemMapMode(); // reinit qspi interface to execute sector erase command
//        /* if valid backup created, replace running app with new firmware */
//#ifdef	USE_WATCHDOG
//		HAL_IWDG_Refresh(&hiwdg);
//#endif
//        if (QSPI2FLASH_Copy (NewFwInfo.ld_addr, NewFwInfo.wr_addr, NewFwInfo.size) != QSPI_OK) Restart();
//        MX_QSPI_Init(); // reinit interface again to 
//        QSPI_MemMapMode(); // reinit qspi interface to execute sector erase command
//#ifdef	USE_WATCHDOG
//		HAL_IWDG_Refresh(&hiwdg);
//#endif
//    }
//    else if (runfw){
//        ResetFwInfo(&BkpFwInfo);
//        BkpFwInfo.ld_addr = RT_APPL_BKP_ADDR;
//        bkpfw = GetFwInfo (&BkpFwInfo); // backup of working firmware
//        if (!newfw){
//            if (QSPI2FLASH_Copy (NewFwInfo.ld_addr, NewFwInfo.wr_addr, NewFwInfo.size) == QSPI_OK){
//                ++bkpfw;    // skeep load fw backup copy 
//            }
//            MX_QSPI_Init();
//            QSPI_MemMapMode(); 
//        } 
//        if (!bkpfw){
//            QSPI2FLASH_Copy (BkpFwInfo.ld_addr, BkpFwInfo.wr_addr, BkpFwInfo.size);
//        }
//    }
//    ResetFwInfo(&RunFwInfo);
//    RunFwInfo.ld_addr = RT_APPL_ADDR;
//    runfw = GetFwInfo (&RunFwInfo);
//    if (!runfw){
//        RunApplication(RunFwInfo.wr_addr);
//    }
//    RunApplication(RT_APPL_ADDR);
//#ifdef	USE_WATCHDOG
//    HAL_IWDG_Refresh(&hiwdg);
//    while(1){
//    }
//#endif 
//    DelayMs(10000U); // slow down loop to prevent exscessive flash erase/write cycles
//}
/**
  * @brief  
  * @param  
  * @retval 
  */
void Restart(void){
    HAL_Deinit();
#ifdef	USE_WATCHDOG
		HAL_IWDG_Refresh(&hiwdg);
#endif
    NVIC_SystemReset();
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void MX_IWDG_Init(void){
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Window = 4095;
    hiwdg.Init.Reload = 4095;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK){
        Restart();
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_Deinit(void){
    HAL_QSPI_DeInit(&hqspi);
    MX_CRC_DeInit();
    HAL_RCC_DeInit();
    HAL_DeInit();
    SCB_DisableICache();
    SCB_DisableDCache();
}
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void){
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 200;  
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);    
    if(ret != HAL_OK){
        Restart();
    }
    ret = HAL_PWREx_EnableOverDrive();
    if(ret != HAL_OK){
        Restart();
    }
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
    if(ret != HAL_OK){
        Restart();
    }
}
/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
void CPU_CACHE_Enable(void){
  SCB_EnableICache();
  SCB_EnableDCache();
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void MX_CRC_Init(void){
    __HAL_RCC_CRC_CLK_ENABLE();    
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse      = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse       = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode    = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode   = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat                = CRC_INPUTDATA_FORMAT_WORDS;
    if (HAL_CRC_Init(&hcrc) != HAL_OK){
        Restart();
    }
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void MX_CRC_DeInit(void){
    __HAL_RCC_CRC_CLK_DISABLE();
    HAL_CRC_DeInit(&hcrc);
    __HAL_RCC_CRC_FORCE_RESET();
    __HAL_RCC_CRC_RELEASE_RESET();
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void RunApplication(uint32_t addr){
    typedef  void (*pFunction)(void);
    static pFunction StartApplication;    
    if (((*(__IO uint32_t*)addr) & 0x2FFC0000) == 0x20000000){
        HAL_Deinit();
		__IO uint32_t start_addr = *(__IO uint32_t*) (addr + 0x4U);
		StartApplication = (pFunction) start_addr;
		__set_MSP(*(__IO uint32_t*) addr);
		StartApplication();
    }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line){
    while (1){
    }
}
#endif
/***************** (C) COPYRIGHT JUBERA D.O.O. SARAJEVO ***********************/
