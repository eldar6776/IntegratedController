/**
 ******************************************************************************
 * @file    timer.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul "Pametni Alarm".
 *
 * @note    Ovaj modul implementira jedan, centralizovan i autonoman sistemski
 * servis za alarm. Pošto postoji samo jedna instanca tajmera u sistemu
 * (singleton), funkcije u API-ju ne zahtijevaju prosljeđivanje handle-a.
 ******************************************************************************
 */

#ifndef __TIMER_H__
#define __TIMER_H__                             FW_BUILD // verzija

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE, STRUKTURE I MAKROI                                       */
/*============================================================================*/

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži sve EEPROM postavke za "Pametni Alarm".
 * @note  Ova struktura se čuva i čita kao jedan blok iz EEPROM-a i zaštićena
 * je magičnim brojem i CRC-om radi integriteta podataka.
 */
typedef struct
{
    uint16_t magic_number;          /**< "Potpis" za validaciju podataka. */
    bool     isActive;              /**< Glavni ON/OFF prekidač za alarm. */
    uint8_t  hour;                  /**< Sat aktivacije (0-23). */
    uint8_t  minute;                /**< Minuta aktivacije (0-59). */
    uint8_t  repeatMask;            /**< Bitmaska za dane ponavljanja (Pon=bit0... Ned=bit6). */
    bool     actionBuzzer;          /**< Fleg koji određuje da li alarm aktivira zujalicu. */
    int8_t   sceneIndexToTrigger;   /**< Indeks scene (0-5) koja se pokreće. Vrijednost -1 znači "nijedna". */
    uint16_t crc;                   /**< CRC za provjeru integriteta podataka. */
} Timer_EepromConfig_t;
#pragma pack(pop)

/**
 * @brief Bitmaske za dane u sedmici radi lakšeg i čitljivijeg rada sa `repeatMask`.
 */
#define TIMER_MONDAY    (1 << 0)
#define TIMER_TUESDAY   (1 << 1)
#define TIMER_WEDNESDAY (1 << 2)
#define TIMER_THURSDAY  (1 << 3)
#define TIMER_FRIDAY    (1 << 4)
#define TIMER_SATURDAY  (1 << 5)
#define TIMER_SUNDAY    (1 << 6)
#define TIMER_WEEKDAYS  (TIMER_MONDAY | TIMER_TUESDAY | TIMER_WEDNESDAY | TIMER_THURSDAY | TIMER_FRIDAY)
#define TIMER_WEEKEND   (TIMER_SATURDAY | TIMER_SUNDAY)
#define TIMER_EVERY_DAY (0x7F)

/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

// --- Grupa 1: Inicijalizacija, Servis i Čuvanje ---
void Timer_Init(void);
void Timer_Save(void);
void Timer_SetDefault(void);
void Timer_Service(void);

// --- Grupa 2: Getteri i Setteri za Konfiguraciju ---
void Timer_SetState(bool isActive);
bool Timer_IsActive(void);

void Timer_SetHour(uint8_t hour);
uint8_t Timer_GetHour(void);

void Timer_SetMinute(uint8_t minute);
uint8_t Timer_GetMinute(void);

void Timer_SetRepeatMask(uint8_t mask);
uint8_t Timer_GetRepeatMask(void);

void Timer_SetActionBuzzer(bool enable);
bool Timer_GetActionBuzzer(void);

void Timer_SetSceneIndex(int8_t index);
int8_t Timer_GetSceneIndex(void);

void Timer_Suppress(void);
void Timer_Unsuppress(void);

#endif // __TIMER_H__
