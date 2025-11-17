/**
 ******************************************************************************
 * File Name          : main.c
 * Date               : 10.3.2018.
 * Description        : Room Thermostat Main Program Header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__                              FW_BUILD 	// header version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
/* Exported constants --------------------------------------------------------*/
extern CRC_HandleTypeDef hcrc;
extern QSPI_HandleTypeDef hqspi;
extern IWDG_HandleTypeDef hiwdg;
void Restart(void);
#endif /* __MAIN_H */
/***************** (C) COPYRIGHT JUBERA D.O.O. SARAJEVO ***********************/
