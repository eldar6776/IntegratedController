/**
 ******************************************************************************
 * @file    defroster.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za potpuno enkapsulirani modul za kontrolu odmrzivaca.
 *
 * @note
 * Ovaj modul koristi "Opaque Pointer" tehniku (Defroster_Handle) za potpunu
 * enkapsulaciju. Stvarna struktura i globalna instanca su sakrivene
 * unutar `defroster.c`, cineci modul samostalnim i lakšim za održavanje.
 *
 * Za interakciju sa modulom, prvo se mora dobiti "handle" pozivom
 * funkcije `Defroster_GetInstance()`, koji se zatim prosljeduje
 * svim ostalim funkcijama u ovom API-ju.
 ******************************************************************************
 */

#ifndef __DEFROSTER_CTRL_H__
#define __DEFROSTER_CTRL_H__                             FW_BUILD // verzija

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I STRUKTURE                                               */
/*============================================================================*/

/**
 * @brief Struktura koja sadrži iskljucivo podatke koji se trajno cuvaju u EEPROM.
 * @note  Definicija je javna kako bi `stm32746g_eeprom.h` mogao koristiti `sizeof()`.
 * `#pragma pack` osigurava kompaktan i pouzdan memorijski raspored.
 */
#pragma pack(push, 1)
typedef struct
{
    uint16_t magic_number;  /**< "Potpis" za validaciju podataka. */
    uint8_t cycleTime;      /**< Vrijeme ciklusa u minutama. */
    uint8_t activeTime;     /**< Aktivno vrijeme unutar ciklusa, u minutama. */
    uint8_t pin;            /**< GPIO pin na koji je povezan grijac odmrzivaca. */
    uint16_t crc;           /**< CRC za provjeru integriteta podataka. */
} Defroster_EepromConfig_t;
#pragma pack(pop)


/*============================================================================*/
/* OPAQUE (SAKRIVENI) TIP PODATAKA                                            */
/*============================================================================*/

/**
 * @brief  "Opaque" (sakriveni) tip za odmrzivac.
 * @note   Stvarna definicija ove strukture je sakrivena unutar `defroster.c`.
 * Pristup je moguc samo preko pointera (handle-a).
 */
typedef struct Defroster_s Defroster_Handle;


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

/**
 * @brief  Vraca pointer (handle) na jedinu (singleton) instancu odmrzivaca.
 * @retval Defroster_Handle* Pointer (handle) na instancu modula.
 */
Defroster_Handle* Defroster_GetInstance(void);

/**
 * @brief  Inicijalizuje modul odmrzivaca.
 * @note   Ucitava postavke iz EEPROM-a, vrši validaciju i postavlja pocetno stanje.
 * @param  handle Pointer na instancu dobijen od `Defroster_GetInstance()`.
 */
void Defroster_Init(Defroster_Handle* const handle);

/**
 * @brief  Cuva trenutnu konfiguraciju odmrzivaca u EEPROM.
 * @note   Automatski izracunava i upisuje CRC radi integriteta.
 * @param  handle Pointer na instancu modula.
 */
void Defroster_Save(Defroster_Handle* const handle);

/**
 * @brief  Postavlja sve parametre odmrzivaca na sigurne fabricke vrijednosti.
 * @param  handle Pointer na instancu modula.
 */
void Defroster_SetDefault(Defroster_Handle* const handle);

/**
 * @brief  Glavna servisna petlja za odmrzivac.
 * @note   Upravlja ciklusima i stanjem. Poziva se periodicno iz `main()`.
 * @param  handle Pointer na instancu modula.
 */
void Defroster_Service(Defroster_Handle* const handle);

/**
 * @brief  Provjerava da li je odmrzivac trenutno aktivan.
 * @param  handle Pointer na instancu modula.
 * @retval bool `true` ako je aktivan, inace `false`.
 */
bool Defroster_isActive(const Defroster_Handle* const handle);

/**
 * @brief  Ukljucuje odmrzivac.
 * @param  handle Pointer na instancu modula.
 */
void Defroster_On(Defroster_Handle* const handle);

/**
 * @brief  Iskljucuje odmrzivac.
 * @param  handle Pointer na instancu modula.
 */
void Defroster_Off(Defroster_Handle* const handle);

/**
 * @brief  Postavlja vrijeme ciklusa odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @param  time Vrijeme ciklusa u minutama.
 */
void Defroster_setCycleTime(Defroster_Handle* const handle, uint8_t time);

/**
 * @brief  Dobija vrijeme ciklusa odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @retval uint8_t Vrijeme ciklusa u minutama.
 */
uint8_t Defroster_getCycleTime(const Defroster_Handle* const handle);

/**
 * @brief  Postavlja aktivno vrijeme odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @param  time Aktivno vrijeme u minutama.
 */
void Defroster_setActiveTime(Defroster_Handle* const handle, uint8_t time);

/**
 * @brief  Dobija aktivno vrijeme odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @retval uint8_t Aktivno vrijeme u minutama.
 */
uint8_t Defroster_getActiveTime(const Defroster_Handle* const handle);

/**
 * @brief  Postavlja GPIO pin za kontrolu odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @param  pin Broj pina.
 */
void Defroster_setPin(Defroster_Handle* const handle, uint8_t pin);

/**
 * @brief  Dobija GPIO pin za kontrolu odmrzivaca.
 * @param  handle Pointer na instancu modula.
 * @retval uint8_t Broj pina.
 */
uint8_t Defroster_getPin(const Defroster_Handle* const handle);


#endif // __DEFROSTER_CTRL_H__
