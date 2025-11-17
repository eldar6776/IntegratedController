/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "stm32f7xx_it.h"
#include "GUI.h"
#include "main.h"
#include "rs485.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern volatile GUI_TIMER_TIME OS_TimeMS;
extern LTDC_HandleTypeDef hltdc;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart2;
extern QSPI_HandleTypeDef hqspi;
extern DMA2D_HandleTypeDef hdma2d;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void NMI_Handler(void){	
}

void HardFault_Handler(void){
	SYSRestart();
}

void MemManage_Handler(void){
	SYSRestart();
}

void BusFault_Handler(void){
	SYSRestart();
}

void UsageFault_Handler(void){
	SYSRestart();
}	

void DebugMon_Handler(void){	
}

void SysTick_Handler(void){
	HAL_IncTick();
	OS_TimeMS++;
    RS485_Tick();
}

void DMA2D_IRQHandler(void){
	HAL_DMA2D_IRQHandler(&hdma2d);
	DMA2D->IFCR = (U32)DMA2D_IFSR_CTCIF;
}

void USART1_IRQHandler(void){
	HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void){
	HAL_UART_IRQHandler(&huart2);
}

void LTDC_IRQHandler(void){
	HAL_LTDC_IRQHandler(&hltdc);
}

void QUADSPI_IRQHandler(void){
	HAL_QSPI_IRQHandler(&hqspi);
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
