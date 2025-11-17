/**
 ******************************************************************************
 * Project          : KuceDzevadova
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

#ifndef APPLICATION
#error "application not selected for application type in common.h"
#endif
/* Includes ------------------------------------------------------------------*/
/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h" // Uvijek prvi - sada povlaci i QSPI/SDRAM headere
#include "firmware_update_agent.h"
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "security.h"
#include "curtain.h"
#include "display.h"
#include "lights.h"
#include "buzzer.h"
#include "rs485.h"
#include "scene.h"
#include "gate.h"

/* Constants -----------------------------------------------------------------*/
/* Imported Type  ------------------------------------------------------------*/
/* Imported Variable  --------------------------------------------------------*/
/* Imported Function  --------------------------------------------------------*/
/* Private Type --------------------------------------------------------------*/
RTC_t date_time;
RTC_TimeTypeDef rtctm;
RTC_DateTypeDef rtcdt;
RTC_HandleTypeDef hrtc;
CRC_HandleTypeDef hcrc;
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;
TIM_HandleTypeDef htim9;
I2C_HandleTypeDef hi2c4;
I2C_HandleTypeDef hi2c3;
IWDG_HandleTypeDef hiwdg;
QSPI_HandleTypeDef hqspi;
LTDC_HandleTypeDef hltdc;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA2D_HandleTypeDef hdma2d;
/* Private Define ------------------------------------------------------------*/
#define TS_UPDATE_TIME			            20U     // 50ms touch screen update period
#define AMBIENT_NTC_RREF                    10000U  // 10k NTC value of at 25 degrees
#define AMBIENT_NTC_B_VALUE                 3977U   // NTC beta parameter
#define AMBIENT_NTC_PULLUP                  10000U	// 10k pullup resistor
#define FANC_NTC_RREF                       2000U  	// 2k fancoil NTC value of at 25 degrees
#define FANC_NTC_B_VALUE                    3977U   // NTC beta parameter
#define FANC_NTC_PULLUP                     2200U	// 2k2 pullup resistor
#define ADC_READOUT_PERIOD                  345     // ntc conversion rate
#define SYSTEM_STARTUP_TIME                 8765U   // 8s application startup time
#define LSE_RESTART_ATTEMPTS                10      // Broj pokušaja prije nego što predemo na LSI
#define LSE_TIMEOUT                         2345    // Timeout za proveru stanja oscilatora (u milisekundama)
#define PCA9685_GENERAL_CALL_ACK			0x00U		// pca9685 general call address with ACK response
#define PCA9685_LED_0_ON_L_REG_ADDRESS      0x06U
#define PCA9685_PRE_SCALE_REG_ADDRESS       0xfeU
#define PCA9685_SW_RESET_COMMAND			0x06U		// i2c pwm controller reset command
#define PCA9685_PRE_SCALE_REGISTER          (pca9685_register[254])
#define I2CPWM0_WRADD                       0x90
#define I2CPWM_TOUT                         15
#define PWM_UPDATE_TIMEOUT					12U			// 20 ms pwm data transfer timeout
#define PWM_0_15_FREQUENCY_DEFAULT			1000U		// i2c pwm controller 1 default frequency in Hertz 
#define PWM_16_31_FREQUENCY_DEFAULT			1000U		// i2c pwm controller 2 default frequency in Hertz 
#define PCA9685_REGISTER_SIZE				256U        // nuber of pca9685 registers

/* Private Variable ----------------------------------------------------------*/
uint8_t sysfl   = 0, initfl = 0;
uint16_t sysid;
uint32_t rstsrc = 0;
static volatile float adc_refcor = 1.0f;
bool LSE_Failed = false;  // Flag koji pokazuje da je LSE trajno otkazan
bool pwminit = true;
static uint8_t pwm[32] = {0};
uint8_t pca9685_register[PCA9685_REGISTER_SIZE] = {0};
// Definišemo globalni fleg i postavljamo ga na `false` kao pocetno stanje.
bool g_high_precision_mode = false;
volatile uint32_t g_last_fw_packet_timestamp = 0; // Definicija globalne varijable
char system_pin[8]; // << NOVO: Definicija globalne varijable
/* Private Macro -------------------------------------------------------------*/
#define VREFIN_CAL_ADDRESS          ((uint16_t*) (0x1FF0F44A))
#define TEMPSENSOR_CAL1_ADDR        ((uint16_t*) (0x1FF0F44C))
#define TEMPSENSOR_CAL2_ADDR        ((uint16_t*) (0x1FF0F44E))
#define PWM_CalculatePrescale(FREQUNCY)	(PCA9685_PRE_SCALE_REGISTER = ((25000000U / (4096U * FREQUNCY)) - 1U))
/* Private Function Prototype ------------------------------------------------*/
static void RAM_Init(void);
static void ADC3_Read(void);
static void MPU_Config(void);
static void MX_IWDG_Init(void);
static void MX_RTC_Init(void);
static void MX_CRC_Init(void);
static void SaveResetSrc(void);
static void CACHE_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM9_Init(void);
static void MX_ADC3_Init(void);
static void MX_UART_Init(void);
static void MX_CRC_DeInit(void);
static void MX_RTC_DeInit(void);
static void MX_TIM9_DeInit(void);
static void MX_I2C3_DeInit(void);
static void MX_I2C4_DeInit(void);
static void MX_ADC3_DeInit(void);
static void MX_GPIO_DeInit(void);
static void MX_UART_DeInit(void);
static void CheckRTC_Clock(void);
static void SystemClock_Config(void);
static uint32_t RTC_GetUnixTimeStamp(RTC_t* data);
static float ROOM_GetTemperature(uint16_t adc_value);
static void PCA9685_Init(void);
static void PCA9685_Reset(void);
static void PCA9685_OutputUpdate(void);
static void PCA9685_SetOutputFrequency(uint16_t frequency);
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
int main(void) 
{
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Ventilator_Handle* pVen = Ventilator_GetInstance();
    Defroster_Handle* pDef = Defroster_GetInstance();
    
    SaveResetSrc();
    MPU_Config();
    CACHE_Config();
    HAL_Init();
    SystemClock_Config();
    MX_IWDG_Init();
    MX_CRC_Init();
    MX_RTC_Init();
    MX_ADC3_Init();
    MX_TIM9_Init();
    MX_GPIO_Init();
    MX_QSPI_Init();
    QSPI_MemMapMode();
    SDRAM_Init();
    EE_Init();
    TS_Init();
    RAM_Init();
    MX_UART_Init();
    RS485_Init();
    LIGHTS_Init();
    Curtains_Init();
    Gate_Init();
    Scene_Init(); 
    Defroster_Init(pDef);
    DISP_Init();
    Buzzer_Init();
    THSTAT_Init(pThst);
    PCA9685_Reset();
    PCA9685_Init();
    if(pwminit) PCA9685_SetOutputFrequency(PWM_0_15_FREQUENCY_DEFAULT);
    Ventilator_Init(pVen);
    Timer_Init();
    Security_Init();
#ifdef	USE_WATCHDOG
    HAL_IWDG_Refresh(&hiwdg);
#endif
    while(1) {
        ADC3_Read();
        TS_Service();
        DISP_Service();
        Timer_Service();
        LIGHT_Service();
        Curtain_Service();
        THSTAT_Service(pThst);
        Defroster_Service(pDef);
        Ventilator_Service(pVen);
        Gate_Service();
        Scene_Service();
        Timer_Service();
        RS485_Service(); // prvo sve obradi pa šalji
        Buzzer_Service();
        CheckRTC_Clock(); // provjera ispravnosti RTC oscilatora i prelazak na LSI
        FwUpdateAgent_Service();
#ifdef	USE_WATCHDOG
        HAL_IWDG_Refresh(&hiwdg);
#endif        
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void SYSRestart(void) 
{
    MX_GPIO_DeInit();
    MX_ADC3_DeInit();
    MX_I2C3_DeInit();
    MX_I2C4_DeInit();
    MX_TIM9_DeInit();
    MX_UART_DeInit();
    HAL_QSPI_DeInit(&hqspi);
    MX_RTC_DeInit();
    MX_CRC_DeInit();
    HAL_RCC_DeInit();
    HAL_DeInit();
    SCB_DisableICache();
    SCB_DisableDCache();
    HAL_NVIC_SystemReset();
}
/**
  * @brief
  * @param
  * @retval
  */
void ErrorHandler(uint8_t function, uint8_t driver) 
{
//    LogEvent.log_type = driver;
//    LogEvent.log_group = function;
//    LogEvent.log_event = FUNC_OR_DRV_FAIL;
//    if (driver != I2C_DRV) LOGGER_Write();
    SYSRestart();
}
/**
  * @brief  Convert from Binary to 2 digit BCD.
  * @param  Value: Binary value to be converted.
  * @retval Converted word
  */
void RTC_GetDateTime(RTC_t* data, uint32_t format) {
    uint32_t unix;

    /* Get rtctm */
    HAL_RTC_GetTime(&hrtc, &rtctm, format);
    /* Format hours */
    data->hours = rtctm.Hours;
    data->minutes = rtctm.Minutes;
    data->seconds = rtctm.Seconds;
    /* Get subseconds */
    data->subseconds = RTC->SSR;
    /* Get rtcdt */
    HAL_RTC_GetDate(&hrtc, &rtcdt, format);
    /* Format rtcdt */
    data->year = rtcdt.Year;
    data->month = rtcdt.Month;
    data->date = rtcdt.Date;
    data->day = rtcdt.WeekDay;
    /* Calculate unix offset */
    unix = RTC_GetUnixTimeStamp(data);
    data->unix = unix;
}
/**
  * @brief
  * @param
  * @retval
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if      (huart->Instance == USART1) {
        RS485_RxCpltCallback();
    }
    else if (huart->Instance == USART2) {
//        OW_RxCpltCallback();
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if      (huart->Instance == USART1) {
        RS485_TxCpltCallback();
    }
    else if (huart->Instance == USART2) {
//        OW_TxCpltCallback();
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if      (huart->Instance == USART1) {
        RS485_ErrorCallback();
    }
    else if (huart->Instance == USART2) {
//        OW_ErrorCallback();
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc) {
    __HAL_RCC_RTC_ENABLE();
}
/**
  * @brief
  * @param
  * @retval
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc) {
    __HAL_RCC_RTC_DISABLE();
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_IWDG_Init(void) {
#ifdef	USE_WATCHDOG
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256; //(1/(32000/32))*4095 = 4,095s
    hiwdg.Init.Window = 4095;
    hiwdg.Init.Reload = 4095;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        SYSRestart();
    }
#endif
}
/**
  * @brief
  * @param
  * @retval
  */
void TS_Service(void) {
// Definicije za dinamicki treshold osjetljivosti na dodir
#define NORMAL_TOUCH_THRESHOLD      30U // Standardna osjetljivost (manje precizno)
#define HIGH_PRECISION_THRESHOLD    2U  // Visoka osjetljivost (precizno za slajdere)

    uint16_t xDiff, yDiff;
    __IO TS_StateTypeDef  ts;
    static GUI_PID_STATE TS_State = {0};
    static uint32_t ts_update_tmr = 0U;

    if (IsDISPCleaningActiv()) return;
    else if ((HAL_GetTick() - ts_update_tmr)  >= TS_UPDATE_TIME) {
        ts_update_tmr = HAL_GetTick();
        BSP_TS_GetState((TS_StateTypeDef *)&ts);
        if((ts.touchX[0] >= LCD_GetXSize()) ||
                (ts.touchY[0] >= LCD_GetYSize())) {
            ts.touchX[0] = 0U;
            ts.touchY[0] = 0U;
            ts.touchDetected = 0U;
        }
        xDiff = (TS_State.x > ts.touchX[0]) ? (TS_State.x - ts.touchX[0]) : (ts.touchX[0] - TS_State.x);
        yDiff = (TS_State.y > ts.touchY[0]) ? (TS_State.y - ts.touchY[0]) : (ts.touchY[0] - TS_State.y);

        // =======================================================================
        // === ZAMIJENITE `IF` USLOV OVOM LOGIKOM ===

        // 1. Definišemo lokalnu varijablu za treshold.
        uint8_t threshold;

        // 2. Provjeravamo globalni fleg i biramo vrijednost.
        if (g_high_precision_mode) {
            threshold = HIGH_PRECISION_THRESHOLD; // Koristi visoku preciznost
        } else {
            threshold = NORMAL_TOUCH_THRESHOLD;   // Koristi normalnu preciznost
        }

        // 3. Koristimo odabranu vrijednost u `if` uslovu.
        if((TS_State.Pressed != ts.touchDetected) || (xDiff > threshold) || (yDiff > threshold)) {
            // =======================================================================
            TS_State.Pressed = ts.touchDetected;
            TS_State.Layer = TS_LAYER;
            if(ts.touchDetected) {
                TS_State.x = ts.touchX[0];
                TS_State.y = ts.touchY[0];
                GUI_TOUCH_StoreStateEx(&TS_State);
            }
            else {
                GUI_TOUCH_StoreStateEx(&TS_State);
                TS_State.x = 0;
                TS_State.y = 0;
            }
        }
    }
}
/**
  * @brief  Inicijalizuje globalne sistemske varijable iz EEPROM-a.
  * @note   Ova funkcija ucitava samo kljucne sistemske parametre koji nisu
  * dio specificnih modula, kao što su sistemski flegovi (dijeljeni sa
  * bootloaderom) i adresa uredaja na RS485 busu.
  * @param  None
  * @retval None
  */
static void RAM_Init(void) 
{
    // Ucitaj sistemski fleg
    EE_ReadBuffer(&sysfl, EE_SYS_STATE, 1);

    // Ucitaj RS485 adresu
    EE_ReadBuffer(&tfifa, EE_TFIFA, 1);

    // Ucitaj sistemski ID
    uint8_t sysid_buf[2];
    EE_ReadBuffer(sysid_buf, EE_SYSID, 2);
    sysid = ((sysid_buf[0] << 8) | sysid_buf[1]);

    // << NOVO: Ucitavanje sistemskog PIN-a >>
    uint8_t pin_buf[8];
    EE_ReadBuffer(pin_buf, EE_SYSTEM_PIN, 8);

    // Provjera da li je PIN ikada snimljen (ako nije, prvi bajt ce biti 0xFF ili 0x00)
    if (pin_buf[0] < '0' || pin_buf[0] > '9') {
        // PIN nije validan, snimi default "1234" u EEPROM i u RAM
        strcpy(system_pin, DEF_SRVC_PSWRD);
        EE_WriteBuffer((uint8_t*)system_pin, EE_SYSTEM_PIN, 8);
    } else {
        // PIN je validan, ucitaj ga u RAM
        memcpy(system_pin, pin_buf, 8);
    }
}
/**
  * @brief
  * @param
  * @retval
  */
static void SaveResetSrc(void) {
    if      (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))  rstsrc = LOW_POWER_RESET;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))   rstsrc = POWER_ON_RESET;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))   rstsrc = SOFTWARE_RESET;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))  rstsrc = IWDG_RESET;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))   rstsrc = PIN_RESET;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))  rstsrc = WWDG_RESET;
    else                                            rstsrc = 0U;
    __HAL_RCC_CLEAR_RESET_FLAGS();
}
/**
  * @brief
  * @param
  * @retval
  */
static void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct;

    /* Disable the MPU */
    HAL_MPU_Disable();

    /* Configure the MPU attributes as WT for SRAM */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress      = 0x20010000U;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_256KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0U;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes for Quad-SPI area to strongly ordered
     This setting is essentially needed to avoid MCU blockings!
     See also STM Application Note AN4861 */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress      = 0x90000000U;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_256MB;
    MPU_InitStruct.SubRegionDisable = 0U;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes for the QSPI 64MB to normal memory Cacheable, must reflect the real memory size */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER3;
    MPU_InitStruct.BaseAddress      = 0x90000000U;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_16MB; // Set region size according to the QSPI memory size
    MPU_InitStruct.SubRegionDisable = 0U;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes for SDRAM_Banks area to strongly ordered
     This setting is essentially needed to avoid MCU blockings!
     See also STM Application Note AN4861 */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER4;
    MPU_InitStruct.BaseAddress      = 0xC0000000U;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_512MB;
    MPU_InitStruct.SubRegionDisable = 0U;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes for SDRAM 8MB to normal memory Cacheable */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER5;
    MPU_InitStruct.BaseAddress      = 0xC0000000U;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_8MB;
    MPU_InitStruct.SubRegionDisable = 0U;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Enable the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    /* Disable FMC bank1 (0x6000 0000 - 0x6FFF FFFF), since it is not used.
    This setting avoids unnedded speculative access to the first FMC bank.
    See also STM Application Note AN4861 */
    FMC_Bank1->BTCR[0] = 0x000030D2U;
}
/**
  * @brief
  * @param
  * @retval
  */
static void CACHE_Config(void) {
//	SCB_EnableICache();
//	SCB_EnableDCache();
    (*(uint32_t *) 0xE000ED94) &= ~0x5;
    (*(uint32_t *) 0xE000ED98) = 0x0; //MPU->RNR
    (*(uint32_t *) 0xE000ED9C) = 0x20010000 | 1 << 4; //MPU->RBAR
    (*(uint32_t *) 0xE000EDA0) = 0 << 28 | 3 << 24 | 0 << 19 | 0 << 18 | 1 << 17 | 0 << 16 | 0 << 8 | 30 << 1 | 1 << 0; //MPU->RASE  WT
    (*(uint32_t *) 0xE000ED94) = 0x5;

    SCB_InvalidateICache();

    /* Enable branch prediction */
    SCB->CCR |= (1 << 18);
    __DSB();

    SCB_EnableICache();

    SCB_InvalidateDCache();
    SCB_EnableDCache();
}
/**
  * @brief inicijalizacija oscilatora sa provjerom starta LSE i ako otkaže prelazak na LSI
  * @param
  * @retval
  */
static void SystemClock_Config(void) {

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 200;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
        LSE_Failed = true;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {

            ErrorHandler(MAIN_FUNC, SYS_CLOCK);
        }
    }
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        ErrorHandler(MAIN_FUNC, SYS_CLOCK);
    }
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK) {
        ErrorHandler(MAIN_FUNC, SYS_CLOCK);
    }
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_RTC
            |RCC_PERIPHCLK_USART1
            |RCC_PERIPHCLK_I2C3|RCC_PERIPHCLK_I2C4;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 57;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 3;
    PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
    PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV2;
    PeriphClkInitStruct.PLLSAIDivQ = 1;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    if (!LSE_Failed)PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    else PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

    PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInitStruct.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
    PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        ErrorHandler(MAIN_FUNC, SYS_CLOCK);
    }

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000U);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_RTC_Init(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    /**Initialize RTC Only
    */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        ErrorHandler(MAIN_FUNC, RTC_DRV);
    }
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x32F2) {
        sTime.Hours = 0x0U;
        sTime.Minutes = 0x0U;
        sTime.Seconds = 0x0U;
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
            ErrorHandler(MAIN_FUNC, RTC_DRV);
        }
        sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
        sDate.Month = RTC_MONTH_JANUARY;
        sDate.Date = 1;
        sDate.Year = 20;
        RtcTimeValidReset();
    } else {
        sDate.Date      = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
        sDate.Month     = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
        sDate.WeekDay   = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);
        sDate.Year      = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);
        RtcTimeValidSet();
    }
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        ErrorHandler(MAIN_FUNC, RTC_DRV);
    }
    __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
    if (HAL_RTC_WaitForSynchro(&hrtc) != HAL_OK) {
        ErrorHandler(MAIN_FUNC, RTC_DRV);
    }
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_RTC_DeInit(void) {
    HAL_RTC_DeInit(&hrtc);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_TIM9_Init(void) {
    TIM_OC_InitTypeDef sConfigOC;
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_TIM9_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    htim9.Instance = TIM9;
    htim9.Init.Prescaler = 200U;
    htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim9.Init.Period = 1000U;
    htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim9) != HAL_OK)
    {
        ErrorHandler(MAIN_FUNC, TMR_DRV);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 80;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        ErrorHandler(MAIN_FUNC, TMR_DRV);
    }
    /**TIM9 GPIO Configuration
    PE5     ------> TIM9_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_TIM9_DeInit(void) {
    __HAL_RCC_TIM9_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_5);
    HAL_TIM_PWM_DeInit(&htim9);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_UART_Init(void) {
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
//    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
//    __HAL_RCC_GPIOD_CLK_ENABLE();

    /**UART4 GPIO Configuration
    PA12    ------> USART1_DE
    */
//    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
//    GPIO_InitStruct.Pin = GPIO_PIN_12;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10    ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /**USART2 GPIO Configuration
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX
    */
//	GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
//	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
//	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_RS485Ex_Init(&huart1, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK) ErrorHandler (MAIN_FUNC, USART_DRV);
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);


//    huart2.Instance = USART2;
//	huart2.Init.BaudRate = 9600U;
//	huart2.Init.Mode = UART_MODE_TX_RX;
//    huart2.Init.Parity = UART_PARITY_NONE;
//    huart2.Init.StopBits = UART_STOPBITS_1;
//    huart2.Init.WordLength = UART_WORDLENGTH_8B;
//	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
//	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
//	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//    if (HAL_UART_Init(&huart2) != HAL_OK) ErrorHandler (MAIN_FUNC, USART_DRV);
//    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
//    HAL_NVIC_EnableIRQ(USART2_IRQn);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_UART_DeInit(void) {
    __HAL_RCC_USART1_CLK_DISABLE();
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5|GPIO_PIN_6);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    HAL_UART_DeInit(&huart1);
    HAL_UART_DeInit(&huart2);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_CRC_Init(void) {
    hcrc.Instance                       = CRC;
    hcrc.Init.DefaultPolynomialUse      = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse       = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode    = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode   = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat                = CRC_INPUTDATA_FORMAT_BYTES;
    __HAL_RCC_CRC_CLK_ENABLE();

    if (HAL_CRC_Init(&hcrc) != HAL_OK)
    {
        ErrorHandler(MAIN_FUNC, CRC_DRV);
    }
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_CRC_DeInit(void) {
    __HAL_RCC_CRC_CLK_DISABLE();
    HAL_CRC_DeInit(&hcrc);
}
/**
  * @brief  Ocitava temperaturu sa NTC senzora i prosljeduje je termostat modulu.
  * @note   Finalna, refaktorisana verzija. Sva originalna logika za filtriranje je
  * sacuvana, dok je logika za odlucivanje (histereza) premještena u termostat modul.
  * @param  None
  * @retval None
  */
static void ADC3_Read(void) {
    // Dobijamo handle za termostat na pocetku.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    // Ako ovaj uredaj nije master, ne treba ni da mjeri temperaturu.
    if (!Thermostat_IsMaster(pThst)) return;

    // Lokalne staticke varijable za filtriranje ocitanja (Vaša originalna logika).
    static uint32_t adctmr = 0U;
    static uint32_t sample_cnt = 0U;
    static uint16_t sample_value[10] = {0};
    static bool first_run = true;
    static float filtered_temp = 0.0f;

    // Provjera da li je vrijeme za novo ocitavanje.
    if ((HAL_GetTick() - adctmr) >= ADC_READOUT_PERIOD) {
        adctmr = HAL_GetTick();

        // Inicijalizacija filtera pri prvom pokretanju (Vaša originalna logika).
        if (first_run) {
            first_run = false;
            uint32_t tmp_init = 0;
            for (uint8_t i = 0; i < 10; i++) {
                HAL_ADC_Start(&hadc3);
                HAL_ADC_PollForConversion(&hadc3, 10);
                sample_value[i] = HAL_ADC_GetValue(&hadc3);
                tmp_init += sample_value[i];
            }
            tmp_init /= 10;
            filtered_temp = ROOM_GetTemperature(tmp_init);
        }

        // Uzimanje novog uzorka sa ADC-a i ažuriranje bafera za prosjek.
        HAL_ADC_Start(&hadc3);
        HAL_ADC_PollForConversion(&hadc3, 10);
        sample_value[sample_cnt] = HAL_ADC_GetValue(&hadc3);
        if (++sample_cnt > 9) sample_cnt = 0;

        // Racunanje prosjeka zadnjih 10 uzoraka.
        uint32_t tmp_avg = 0;
        for (uint8_t t = 0; t < 10; t++) tmp_avg += sample_value[t];
        tmp_avg /= 10;

        // Provjera da li je NTC senzor (dis)konektovan.
        if ((tmp_avg < 100) || (tmp_avg > 4000)) {
            // NTC je diskonektovan. Obavijesti termostat modul o novom stanju.
            Thermostat_SetNtcStatus(pThst, false, true);
            // Javi termostatu da je temperatura 0. On ce interno obraditi tu informaciju.
            Thermostat_SetMeasuredTemp(pThst, 0);
        } else {
            // NTC je konektovan. Obavijesti termostat modul.
            Thermostat_SetNtcStatus(pThst, true, false);

            // Primjena eksponencijalnog filtera za "peglanje" vrijednosti (Vaša originalna logika).
            float new_temp = ROOM_GetTemperature(tmp_avg);
            filtered_temp = (filtered_temp * 0.9f) + (new_temp * 0.1f);

            // Konverzija u format koji termostat koristi (int16_t, vrijednost x10).
            int16_t ntc_temp = filtered_temp * 10;

            // JEDINI POSAO: Pozovi setter i proslijedi mu novu, filtriranu temperaturu.
            // Termostat modul ce sam odluciti da li je promjena znacajna.
            Thermostat_SetMeasuredTemp(pThst, ntc_temp);
        }
    }
}
/**
  * @brief
        adc_cnt = 0U;
  * @param
  * @retval
  */
static void MX_ADC3_Init(void) {
    ADC_ChannelConfTypeDef sConfig;
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_ADC3_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.NbrOfDiscConversion = 0U;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1U;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if(HAL_ADC_Init(&hadc3) != HAL_OK)
    {
        ErrorHandler(MAIN_FUNC, ADC_DRV);
    }

    sConfig.Channel = ADC_CHANNEL_11;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    sConfig.Offset = 0U;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_ADC3_DeInit(void) {
    __HAL_RCC_ADC3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    HAL_ADC_DeInit(&hadc3);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_I2C3_DeInit(void) {
    __HAL_RCC_I2C3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
    HAL_I2C_DeInit(&hi2c3);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_I2C4_DeInit(void) {
    __HAL_RCC_I2C4_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_12 | GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_2);
    HAL_I2C_DeInit(&hi2c4);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3,  GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_7|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}
/**
  * @brief
  * @param
  * @retval
  */
static void MX_GPIO_DeInit(void) {
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_7|GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_3|GPIO_PIN_13|GPIO_PIN_14);
}
/**
  * @brief
  * @param
  * @retval
  */
static float ROOM_GetTemperature(uint16_t adc_value) {
    float temperature;
    float ntc_resistance;
    ntc_resistance = (float) (AMBIENT_NTC_PULLUP * ((4095.0f / (4095.0f - adc_value)) - 1.0f));
    temperature = ((AMBIENT_NTC_B_VALUE * 298.1f) /  (AMBIENT_NTC_B_VALUE + (298.1f * log(ntc_resistance / AMBIENT_NTC_RREF))) -273.1f);
    return(temperature);
}
/**
  * @brief  periodicna provjera RTC oscilatora i prebacivanje na LSI u slucaju otkazivanja
  * @param
  * @retval
  */
void CheckRTC_Clock(void) {
    static uint32_t lastCheckTime = 0;
    static uint8_t lastSeconds = 60;
    // Provjeri da li je prošlo dovoljno vremena za novu provjeru
    if ((HAL_GetTick() - lastCheckTime) >= LSE_TIMEOUT) {
        lastCheckTime = HAL_GetTick();  // Ažuriraj vrijeme posljednje provjere
        // Provjeri trenutni broj sekundi RTC-a
        RTC_TimeTypeDef sTime;
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

        if (!LSE_Failed) {


            // Ako se broj sekundi nije promjenio od posljednje provere, znaci da LSE nije funkcionalan
            if (sTime.Seconds == lastSeconds) {
                LSE_Failed = true;  // Oznaci da je LSE otkazao

                // Prebaci na LSI
                RCC_OscInitTypeDef RCC_OscInitStruct = {0};
                RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
                RCC_OscInitStruct.LSIState = RCC_LSI_ON;
                if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
                    ErrorHandler(MAIN_FUNC, SYS_CLOCK);
                }

                // Postavi RTC da koristi LSI
                RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
                PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
                PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
                if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
                    ErrorHandler(MAIN_FUNC, SYS_CLOCK);
                }

                // Resetuj RTC nakon prebacivanja
                HAL_RTC_DeInit(&hrtc);
                MX_RTC_Init();  // Ponovno inicijalizuj RTC sa LSI
            }

            // Ažuriraj posljednje sekundno ocitavanje
            lastSeconds = sTime.Seconds;
        }
    }
}
/**
  * @brief  Convert from Binary to 2 digit BCD.
  * @param  Value: Binary value to be converted.
  * @retval Converted word
  */
static uint32_t RTC_GetUnixTimeStamp(RTC_t* data) {
    uint32_t days = 0U, seconds = 0U;
    uint16_t i;
    uint16_t year = (uint16_t) (data->year + 2000U);

    /* Year is below offset year */
    if (year < UNIX_OFFSET_YEAR)
    {
        return 0U;
    }
    /* Days in back years */
    for (i = UNIX_OFFSET_YEAR; i < year; i++)
    {
        days += DAYS_IN_YEAR(i);
    }
    /* Days in current year */
    for (i = 1U; i < data->month; i++)
    {
        days += rtc_months[LEAP_YEAR(year)][i - 1U];
    }
    /* Day starts with 1 */
    days += data->date - 1U;
    seconds = days * SECONDS_PER_DAY;
    seconds += data->hours * SECONDS_PER_HOUR;
    seconds += data->minutes * SECONDS_PER_MINUTE;
    seconds += data->seconds;
    return seconds;
}

/**
  * @brief
  * @param
  * @retval
  */
static void PCA9685_Init(void) { //registrovana 2 PCA965 0x90 I2CPWM0_WRADD  i 0x92  I2CPWM1_WRADD
    uint8_t buf[2];
    if(!pwminit) return;
    buf[0] = 0x00U;
    buf[1]= 0x00U;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
    HAL_Delay(5);
    buf[0] = 0x01U;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
    HAL_Delay(2);
    ZEROFILL(pwm, 32);
    ZEROFILL(pca9685_register, PCA9685_REGISTER_SIZE);
}
/**
  * @brief
  * @param
  * @retval
  */
static void PCA9685_Reset(void) {
    uint8_t cmd = PCA9685_SW_RESET_COMMAND;

    if(HAL_I2C_Master_Transmit(&hi2c4, PCA9685_GENERAL_CALL_ACK, &cmd, 1, I2CPWM_TOUT) != HAL_OK) {
        pwminit = false;
    }
}


/**
  * @brief
  * @param
  * @retval
  */
static void PCA9685_SetOutputFrequency(uint16_t frequency) {
    uint8_t buf[2];
    buf[0] = 0x00U;
    buf[1]= 0x10U;
    if(!pwminit) return;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
    PWM_CalculatePrescale(frequency);
    buf[0] = PCA9685_PRE_SCALE_REG_ADDRESS;
    buf[1]= PCA9685_PRE_SCALE_REGISTER;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
    buf[0] = 0x00U;
    buf[1]= 0xa0U;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
    HAL_Delay(5);
    buf[0] = 0x01U;
    buf[1]= 0x04U;
    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK) {
        SYSRestart();
    }
//    buf[0] = 0x00U;
//	buf[1]= 0x10U;
//	if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM1_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK){
//		SYSRestart();
//	}
//    HAL_Delay(5);
//	PWM_CalculatePrescale(frequency);
//	buf[0] = PCA9685_PRE_SCALE_REG_ADDRESS;
//	buf[1]= PCA9685_PRE_SCALE_REGISTER;
//	if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM1_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK){
//		SYSRestart();
//	}
//	buf[0] = 0x00U;
//	buf[1]= 0xa0U;
//	if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM1_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK){
//		SYSRestart();
//	}
//	HAL_Delay(5);
//	buf[0] = 0x01U;
//	buf[1]= 0x04U;
//	if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM1_WRADD, buf, 2, I2CPWM_TOUT) != HAL_OK){
//		SYSRestart();
//	}
}

/**
  * @brief
  * @param
  * @retval
  */
static void PCA9685_OutputUpdate(void) {

    uint16_t pwm_out;
    uint8_t i,j,buf[70];
    if(!pwminit) return;
    j = 6;
    for(i = 0; i < 16; i++) {
        pca9685_register[j]= 0;
        j += 1;
        pca9685_register[j] = 0;
        j += 3;
    }
    j = 8;
    for(i = 0; i < 16; i++) {
        pwm_out = pwm[i] * 16U;
        if (pwm_out > 4000) pwm_out = 0x0FFFU; // zaokzuzi izlaz na maksimalno 4096 0xfff
        pca9685_register[j]= (pwm_out & 0xffU);
        j += 1;
        pca9685_register[j] = (pwm_out >> 8U);
        j += 3;
    }
    buf[0] = PCA9685_LED_0_ON_L_REG_ADDRESS;
    memcpy(&buf[1],&pca9685_register[6], 64);

    HAL_Delay(300);

    if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM0_WRADD, buf, 65, PWM_UPDATE_TIMEOUT) != HAL_OK) {
        SYSRestart();
    }
//	j = 8;
//    for(i = 16; i < 32; i++){
//		pwm_out= pwm[i] * 16U;
//        pca9685_register[j]= (pwm_out & 0xffU);
//        j += 1;
//        pca9685_register[j] = (pwm_out >> 8U);
//        j += 3;
//	}
//    buf[0] = PCA9685_LED_0_ON_L_REG_ADDRESS;
//    memcpy(&buf[1],&pca9685_register[6], 64);
//	if(HAL_I2C_Master_Transmit(&hi2c4, I2CPWM1_WRADD, buf, 65, PWM_UPDATE_TIMEOUT) != HAL_OK){
//		SYSRestart();
//	}
}

/**
  * @brief
  * @param
  * @retval
  */
void PCA9685_SetOutput(const uint8_t pin, const uint8_t value) {
    if(!pwminit) return;
    pwm[pin - 1] = value;
    PCA9685_OutputUpdate();
}

/**
  * @brief
  * @param
  * @retval
  */
void SetDefault(void) // Not all settings from the settings menu are set to default
{
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Ventilator_Handle* pVen = Ventilator_GetInstance();
    Defroster_Handle* pDef = Defroster_GetInstance();

    Thermostat_SetDefault(pThst);
    THSTAT_Save(pThst);

    LIGHTS_SetDefault();
    LIGHTS_Save();

    Curtains_SetDefault();
    Curtains_Save();

    Ventilator_SetDefault(pVen);
    Ventilator_Save(pVen);

    Defroster_SetDefault(pDef);
    Defroster_Save(pDef);
}

/**
  * @brief
  * @param
  * @retval
  */
void SetPin(uint8_t pin, uint8_t pinVal) {
    if(pin > 0 && pin < 7) { // Dodata provjera da li je pin validan
        switch(pin)
        {
        case 1:
            if(pinVal) Light1On();
            else Light1Off();
            break;

        case 2:
        {
            if(pinVal) Light2On();
            else Light2Off();
            break;
        }

        case 3:
            if(pinVal) Light3On();
            else Light3Off();
            break;

        case 4:
            if(pinVal) Light4On();
            else Light4Off();
            break;

        case 5:
            if(pinVal) Light5On();
            else Light5Off();
            break;

        case 6:
            if(pinVal) Light6On();
            else Light6Off();
            break;

        default:
            break;
        }
    }
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
