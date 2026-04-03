/**
 ******************************************************************************
 * @file    timer.c
 * @author  Gemini & [VaÅ¡e Ime]
 * @brief   Implementacija logike za modul "Pametni Alarm".
 *
 * @note    Ovaj fajl sadrÅ¾i kompletnu pozadinsku (backend) logiku.
 * Odgovoran je za Äuvanje i Äitanje konfiguracije alarma iz EEPROM-a,
 * te za provjeru vremena i aktivaciju definisanih akcija (zujalica, scene).
 * Modul je potpuno enkapsuliran i autonoman, koristeÄ‡i jednu statiÄku
 * instancu za sve operacije.
 ******************************************************************************
 */

#if (__TIMER_H__ != FW_BUILD)
#error "timer header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "timer.h"
#include "scene.h"
#include "buzzer.h"
#include "stm32746g_eeprom.h"

/*============================================================================*/
/* PRIVATNA DEFINICIJA "RUNTIME" STRUKTURE                                    */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief Puna definicija "runtime" strukture za Pametni Alarm.
 * @note  Ova struktura objedinjuje konfiguracione podatke koji se Äuvaju
 * trajno i runtime flegove koji se koriste tokom izvrÅ¡avanja.
 ******************************************************************************
 */
typedef struct
{
    /**
     * @brief Struktura sa konfiguracionim podacima koji se Äuvaju u EEPROM-u.
     */
    Timer_EepromConfig_t config;

    /**
     * @brief Interni "latch" fleg koji sprjeÄava viÅ¡estruko aktiviranje alarma unutar jedne minute.
     * @note  Resetuje se na `false` svaki put kada se promijeni minuta na RTC-u.
     */
    bool hasTriggeredThisMinute;
} Timer_Runtime_t;

/*============================================================================*/
/* PRIVATNA (STATIÄŒKA) INSTANCA                                               */
/*============================================================================*/

/**
 * @brief Jedina instanca tajmera u cijelom sistemu (Singleton pattern).
 * @note  Sve funkcije u ovom modulu implicitno rade sa ovom instancom.
 */
static Timer_Runtime_t timer;

/**
 * @brief Fleg koji privremeno zaustavlja izvrÅ¡avanje alarmne logike.
 * @note  Postavlja se na `true` kada korisnik uÄ‘e u meni za podeÅ¡avanje.
 */
static bool is_suppressed = false;
/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

// --- Grupa 1: Inicijalizacija, Servis i ÄŒuvanje ---

/**
 ******************************************************************************
 * @brief       Inicijalizuje modul tajmera iz EEPROM-a.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se poziva jednom pri startu sistema. UÄitava
 * konfiguracioni blok i provjerava njegov integritet. Ako podaci
 * nisu validni, poziva `Timer_SetDefault` i snima fabriÄke vrijednosti.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Timer_Init(void)
{
    EE_ReadBuffer((uint8_t*)&timer.config, EE_TIMER, sizeof(Timer_EepromConfig_t));

    if (timer.config.magic_number != EEPROM_MAGIC_NUMBER) {
        Timer_SetDefault();
        Timer_Save();
    } else {
        uint16_t received_crc = timer.config.crc;
        timer.config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&timer.config, sizeof(Timer_EepromConfig_t));

        if (received_crc != calculated_crc) {
            Timer_SetDefault();
            Timer_Save();
        }
    }
    // Runtime varijable se uvijek resetuju na poÄetku.
    timer.hasTriggeredThisMinute = false;
}

/**
 ******************************************************************************
 * @brief       ÄŒuva trenutnu konfiguraciju tajmera u EEPROM.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Prije upisa, izraÄunava novi CRC32 nad konfiguracijom.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Timer_Save(void)
{
    timer.config.magic_number = EEPROM_MAGIC_NUMBER;
    timer.config.crc = 0;
    timer.config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&timer.config, sizeof(Timer_EepromConfig_t));
    EE_WriteBuffer((uint8_t*)&timer.config, EE_TIMER, sizeof(Timer_EepromConfig_t));
}

/**
 ******************************************************************************
 * @brief       Postavlja sve postavke tajmera na sigurne fabriÄke vrijednosti.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        DefiniÅ¡e poÄetno stanje alarma (07:30, radnim danima, zujalica).
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Timer_SetDefault(void)
{
    memset(&timer.config, 0, sizeof(Timer_EepromConfig_t));
    timer.config.isActive = false;
    timer.config.hour = 7;
    timer.config.minute = 30;
    timer.config.repeatMask = TIMER_WEEKDAYS;
    timer.config.actionBuzzer = true;
    timer.config.sceneIndexToTrigger = -1;
}

/**
 ******************************************************************************
 * @brief       Glavna servisna petlja za tajmer.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Poziva se iz `main()` petlje. Provjerava vrijeme i aktivira alarm.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Timer_Service(void)
{
    static uint8_t last_checked_minute = 61;

    if (is_suppressed) {
        return; // Prekini izvrÅ¡avanje ako je alarm pauziran
    }
    
    if (!timer.config.isActive || !IsRtcTimeValid()) {
        return;
    }

    RTC_TimeTypeDef currentTime;
    RTC_DateTypeDef currentDate;

    HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);

    if (currentTime.Minutes != last_checked_minute) {
        timer.hasTriggeredThisMinute = false;
        last_checked_minute = currentTime.Minutes;
    }

    if (!timer.hasTriggeredThisMinute) {
        if ((timer.config.hour == currentTime.Hours) && (timer.config.minute == currentTime.Minutes)) {
            HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);
            uint8_t day_of_week_mask = (1 << (currentDate.WeekDay - 1));

            if (timer.config.repeatMask & day_of_week_mask) {
                timer.hasTriggeredThisMinute = true;

                if (timer.config.actionBuzzer) {
                    Buzzer_StartAlarm();
                    screen = SCREEN_ALARM_ACTIVE;
                    shouldDrawScreen = 1;
                }

                if (timer.config.sceneIndexToTrigger != -1) {
                    Scene_Activate(timer.config.sceneIndexToTrigger);
                }
            }
        }
    }
}

// --- Grupa 2: Getteri i Setteri za Konfiguraciju ---

/**
 ******************************************************************************
 * @brief Postavlja glavno stanje alarma (ON/OFF).
 ******************************************************************************
 */
void Timer_SetState(bool isActive) {
    timer.config.isActive = isActive;
}

/**
 ******************************************************************************
 * @brief Provjerava da li je alarm aktivan.
 ******************************************************************************
 */
bool Timer_IsActive(void) {
    return timer.config.isActive;
}

/**
 ******************************************************************************
 * @brief Postavlja sat za aktivaciju alarma.
 ******************************************************************************
 */
void Timer_SetHour(uint8_t hour) {
    if (hour < 24) {
        timer.config.hour = hour;
    }
}

/**
 ******************************************************************************
 * @brief VraÄ‡a podeÅ¡eni sat aktivacije.
 ******************************************************************************
 */
uint8_t Timer_GetHour(void) {
    return timer.config.hour;
}

/**
 ******************************************************************************
 * @brief Postavlja minutu za aktivaciju alarma.
 ******************************************************************************
 */
void Timer_SetMinute(uint8_t minute) {
    if (minute < 60) {
        timer.config.minute = minute;
    }
}

/**
 ******************************************************************************
 * @brief VraÄ‡a podeÅ¡enu minutu aktivacije.
 ******************************************************************************
 */
uint8_t Timer_GetMinute(void) {
    return timer.config.minute;
}

/**
 ******************************************************************************
 * @brief Postavlja masku za dane ponavljanja.
 ******************************************************************************
 */
void Timer_SetRepeatMask(uint8_t mask) {
    timer.config.repeatMask = mask & 0x7F;
}

/**
 ******************************************************************************
 * @brief VraÄ‡a masku za dane ponavljanja.
 ******************************************************************************
 */
uint8_t Timer_GetRepeatMask(void) {
    return timer.config.repeatMask;
}

/**
 ******************************************************************************
 * @brief Postavlja da li alarm aktivira zujalicu.
 ******************************************************************************
 */
void Timer_SetActionBuzzer(bool enable) {
    timer.config.actionBuzzer = enable;
}

/**
 ******************************************************************************
 * @brief Provjerava da li alarm aktivira zujalicu.
 ******************************************************************************
 */
bool Timer_GetActionBuzzer(void) {
    return timer.config.actionBuzzer;
}

/**
 ******************************************************************************
 * @brief Postavlja indeks scene koja se pokreÄ‡e.
 ******************************************************************************
 */
void Timer_SetSceneIndex(int8_t index) {
    if (index >= -1 && index < SCENE_MAX_COUNT) {
        timer.config.sceneIndexToTrigger = index;
    }
}

/**
 ******************************************************************************
 * @brief VraÄ‡a indeks scene koja se pokreÄ‡e.
 ******************************************************************************
 */
int8_t Timer_GetSceneIndex(void) {
    return timer.config.sceneIndexToTrigger;
}

/**
 * @brief Pauzira izvrÅ¡avanje alarmne logike u Timer_Service.
 */
void Timer_Suppress(void) {
    is_suppressed = true;
}

/**
 * @brief Nastavlja izvrÅ¡avanje alarmne logike u Timer_Service.
 */
void Timer_Unsuppress(void) {
    is_suppressed = false;
}

