/**
 ******************************************************************************
 * @file    buzzer.h
 * @author      Gemini & [Vaše Ime]
 * @brief       Javni API za ne-blokirajući drajver za zujalicu.
 ******************************************************************************
 */
#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "main.h"

void Buzzer_Init(void);
void Buzzer_Service(void);
void Buzzer_StartAlarm(void);
void Buzzer_Stop(void);
void Buzzer_SingleClick(void);

#endif // __BUZZER_H__
