/**
 ******************************************************************************
 * @file    defroster.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija kompletne logike za upravljanje odmrzivacem.
 *
 * @note
 * Ovaj fajl sadrži privatnu, staticku `defroster` instancu, cineci je
 * nevidljivom za ostatak programa. Sva interakcija sa podacima se vrši
 * preko `handle`-a koji se prosljeduje funkcijama. `Defroster_Service()`
 * periodicno poziva pomocne `Handle...` funkcije za obradu tajmera.
 ******************************************************************************
 */

#if (__DEFROSTER_CTRL_H__ != FW_BUILD)
#error "defroster header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "defroster.h"
#include "display.h"
#include "stm32746g_eeprom.h"

/*============================================================================*/
/* PRIVATNA DEFINICIJA STRUKTURE                                              */
/*============================================================================*/

/**
 * @brief Puna definicija glavne strukture, sakrivena od ostatka programa.
 * Ovo je konkretna implementacija `Defroster_Handle` tipa.
 */
struct Defroster_s
{
    // Konfiguracioni dio strukture koji se cuva u EEPROM
    Defroster_EepromConfig_t config;

    // Runtime varijable koje se ne cuvaju
    uint32_t cycleTime_TimerStart;
    uint32_t activeTime_TimerStart;
};

/*============================================================================*/
/* PRIVATNA (STATICKA) INSTANCA                                               */
/*============================================================================*/

/**
 * @brief Jedina instanca odmrzivaca u cijelom sistemu (Singleton pattern).
 * @note  Kljucna rijec 'static' je cini vidljivom samo unutar ovog .c fajla.
 */
static struct Defroster_s defroster;


/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOCNIH FUNKCIJA                                    */
/*============================================================================*/
static void HandleDefrosterCycle(Defroster_Handle* const handle);
static void HandleDefrosterActiveTime(Defroster_Handle* const handle);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

Defroster_Handle* Defroster_GetInstance(void)
{
    return &defroster;
}

void Defroster_Init(Defroster_Handle* const handle)
{
    EE_ReadBuffer((uint8_t*)&handle->config, EE_DEFROSTER, sizeof(Defroster_EepromConfig_t));

    if (handle->config.magic_number != EEPROM_MAGIC_NUMBER) {
        Defroster_SetDefault(handle);
        Defroster_Save(handle);
    } else {
        uint16_t received_crc = handle->config.crc;
        handle->config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Defroster_EepromConfig_t));
        if (received_crc != calculated_crc) {
            Defroster_SetDefault(handle);
            Defroster_Save(handle);
        }
    }
    // Runtime varijable se uvijek resetuju na pocetku
    handle->cycleTime_TimerStart = 0;
    handle->activeTime_TimerStart = 0;
}

void Defroster_Save(Defroster_Handle* const handle)
{
    handle->config.magic_number = EEPROM_MAGIC_NUMBER;
    handle->config.crc = 0;
    handle->config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Defroster_EepromConfig_t));
    EE_WriteBuffer((uint8_t*)&handle->config, EE_DEFROSTER, sizeof(Defroster_EepromConfig_t));
}

void Defroster_SetDefault(Defroster_Handle* const handle)
{
    // Sigurnosno nuliranje cijele strukture
    memset(handle, 0, sizeof(struct Defroster_s));
    // Vrijednosti su vec 0, ali ih eksplicitno postavljamo radi citljivosti
    handle->config.cycleTime = 0;
    handle->config.activeTime = 0;
    handle->config.pin = 0;
}

void Defroster_Service(Defroster_Handle* const handle)
{
    // Servis radi samo ako je u postavkama displeja odabran mod za odmrzivac
    if (g_display_settings.selected_control_mode != MODE_DEFROSTER) {
        return;
    }

    // I samo ako je odmrzivac globalno aktivan (ima pokrenut ciklicni tajmer)
    if(Defroster_isActive(handle))
    {
        HandleDefrosterCycle(handle);
        HandleDefrosterActiveTime(handle);
    }
}

bool Defroster_isActive(const Defroster_Handle* const handle)
{
    // Odmrzivac se smatra aktivnim ako mu je pokrenut ciklicni tajmer.
    return (handle->cycleTime_TimerStart != 0);
}

void Defroster_On(Defroster_Handle* const handle)
{
    if (handle->config.pin == 0) return; // Ne radi ništa ako pin nije konfigurisan

    uint32_t tick = HAL_GetTick();
    handle->cycleTime_TimerStart = tick ? tick : 1;
    handle->activeTime_TimerStart = tick ? tick : 1;
    SetPin(handle->config.pin, 1); // Postavlja pin na HIGH
}

void Defroster_Off(Defroster_Handle* const handle)
{
    if (handle->config.pin == 0) return;

    handle->cycleTime_TimerStart = 0;
    handle->activeTime_TimerStart = 0;
    SetPin(handle->config.pin, 0); // Postavlja pin na LOW
}

void Defroster_setCycleTime(Defroster_Handle* const handle, uint8_t time)
{
    handle->config.cycleTime = time;
    // Osigurava da aktivno vrijeme ne bude vece od vremena ciklusa
    if(handle->config.activeTime > time)
    {
        handle->config.activeTime = time;
    }
}

uint8_t Defroster_getCycleTime(const Defroster_Handle* const handle)
{
    return handle->config.cycleTime;
}

void Defroster_setActiveTime(Defroster_Handle* const handle, uint8_t time)
{
    // Osigurava da aktivno vrijeme ne bude vece od vremena ciklusa
    if(time > handle->config.cycleTime)
    {
        handle->config.activeTime = handle->config.cycleTime;
    }
    else
    {
        handle->config.activeTime = time;
    }
}

uint8_t Defroster_getActiveTime(const Defroster_Handle* const handle)
{
    return handle->config.activeTime;
}

void Defroster_setPin(Defroster_Handle* const handle, uint8_t pin)
{
    handle->config.pin = pin;
}

uint8_t Defroster_getPin(const Defroster_Handle* const handle)
{
    return handle->config.pin;
}

/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH POMOCNIH FUNKCIJA                                 */
/*============================================================================*/

/**
 * @brief Upravlja ciklusom rada odmrzivaca.
 * @note  Ako je ciklicni tajmer istekao, ponovo ga pokrece, pokrece i tajmer
 * aktivnog vremena i ukljucuje grijac (podiže pin na HIGH).
 * @param handle Pointer na instancu modula.
 */
static void HandleDefrosterCycle(Defroster_Handle* const handle)
{
    // Ako ciklus nije podešen (vrijednost 0), ne radi ništa.
    if (handle->config.cycleTime == 0) return;

    const uint32_t tick = HAL_GetTick();
    if((tick - handle->cycleTime_TimerStart) >= (handle->config.cycleTime * 60000UL)) // Minute u ms
    {
        handle->cycleTime_TimerStart = tick ? tick : 1;
        handle->activeTime_TimerStart = tick ? tick : 1;
        SetPin(handle->config.pin, 1);
    }
}

/**
 * @brief Upravlja aktivnim vremenom odmrzivaca.
 * @note  Ako je tajmer za aktivno vrijeme istekao, zaustavlja tajmer i
 * iskljucuje grijac (spušta pin na LOW).
 * @param handle Pointer na instancu modula.
 */
static void HandleDefrosterActiveTime(Defroster_Handle* const handle)
{
    // Ako aktivno vrijeme nije podešeno (vrijednost 0), ne radi ništa.
    if (handle->config.activeTime == 0) return;

    // Ako tajmer nije ni pokrenut, ne radi ništa.
    if (handle->activeTime_TimerStart == 0) return;

    const uint32_t tick = HAL_GetTick();
    if((tick - handle->activeTime_TimerStart) >= (handle->config.activeTime * 60000UL)) // Minute u ms
    {
        handle->activeTime_TimerStart = 0; // Zaustavi tajmer
        SetPin(handle->config.pin, 0);
    }
}
