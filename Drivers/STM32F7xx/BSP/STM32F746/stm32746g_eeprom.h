/**
 ******************************************************************************
 * @file    stm32746g_eeprom.h
 * @author  Gemini
 * @brief   Ovaj fajl definiše memorijsku mapu za EEPROM.
 * @note    Mapa je dizajnirana da bude fleksibilna i otporna na promjene.
 * Koristi sizeof() za automatsko izracunavanje adresa blokova podataka,
 * cime se eliminiše potreba za rucnim ažuriranjem adresa.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H__
#define __EEPROM_H__                        FW_BUILD // version

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32746g.h"

// NOVO: Ukljucujemo headere modula da bismo dobili pristup definicijama
// njihovih EEPROM struktura.
#include "display.h"
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "security.h"
#include "curtain.h"
#include "lights.h"
#include "scene.h"
#include "timer.h"
#include "gate.h"
     
/* EEPROM hardware address and page size */
#define EE_PGSIZE                           64
#define EE_MAXSIZE                          0x4000 /* 64Kbit */
#define EE_ADDR                             0xA0
#define EEPROM_MAGIC_NUMBER                 0xABCD // Definišemo jedinstven "magicni broj" za cijeli projekat

// =================================================================================
// === FLEKSIBILNA EEPROM MAPA ===
// =================================================================================

/**
 * @brief  Sekcija 1: Nezavisne Sistemske Varijable
 * @note   Ove varijable imaju fiksne adrese i ne podliježu CRC provjeri unutar
 * vecih blokova. Koriste se za osnovne sistemske funkcije i komunikaciju
 * sa drugim sistemima (npr. bootloaderom).
 */
#define EE_INIT_ADDR                        0x00    // 1 bajt:  Marker za provjeru prve inicijalizacije.
#define EE_SYS_STATE                        0x02    // 1 bajt:  Sistemski flegovi (dijeli se sa bootloaderom!).
#define EE_TFIFA			                0x04	// 1 bajt:  Adresa uredaja na RS485 busu (TinyFrame).
#define EE_SYSID			                0x05	// 2 bajta: Jedinstveni ID sistema.
#define EE_SYSTEM_PIN                       0x08    // 5 bajtova: Sistemski PIN kod (npr. "1234\0")
/**
 * @brief  Sekcija 2: Struktuirani Blokovi Podataka
 * @note   Ovo je glavni dio konfiguracije. Svaki modul ima svoj blok podataka
 * koji se cita/piše odjednom i zašticen je CRC-om. Adrese se
 * automatski izracunavaju na osnovu velicine prethodnog bloka.
 */
// Pocetak prvog bloka na sigurnoj adresi, ostavljajuci prostor za buduce sistemske varijable.
#define EE_DISPLAY_SETTINGS                 0x20

// PROMJENA: Umjesto hardkodovanih brojeva, sada koristimo sizeof()
#define EE_THERMOSTAT                       (EE_DISPLAY_SETTINGS + sizeof(Display_EepromSettings_t))
#define EE_VENTILATOR                       (EE_THERMOSTAT + sizeof(THERMOSTAT_EepromConfig_t))
#define EE_DEFROSTER                        (EE_VENTILATOR + sizeof(Ventilator_EepromConfig_t))
#define EE_CURTAINS                         (EE_DEFROSTER + sizeof(Defroster_EepromConfig_t))
#define EE_LIGHTS_MODBUS                    (EE_CURTAINS + sizeof(Curtains_EepromData_t))
#define EE_SCENES                           (EE_LIGHTS_MODBUS + (sizeof(LIGHT_EepromConfig_t) * LIGHTS_MODBUS_SIZE))
#define EE_GATES                            (EE_SCENES + sizeof(Scene_EepromBlock_t))
#define EE_TIMER                            (EE_GATES + (sizeof(Gate_EepromConfig_t) * GATE_MAX_COUNT))
#define EE_SECURITY                         (EE_TIMER + sizeof(Timer_EepromConfig_t))


/**
 * @brief  Sekcija 3: Blokovi za Specijalne Namjene
 * @note   Ovi blokovi su na visokim, fiksnim adresama kako bi bili izolovani od
 * dinamicke konfiguracione mape.
 */
#define EE_QR_CODE1                         0x400       // Rezervisano 64 bajta za WiFi QR kod.
#define EE_QR_CODE2                         0x440       // Rezervisano 64 bajta za App QR kod.


/* Link function for I2C EEPROM peripheral */
void     EE_Init         (void);
uint32_t EE_ReadBuffer   (uint8_t *pBuffer, uint16_t ReadAddr,  uint16_t NumByteToRead);
uint32_t EE_WriteBuffer  (uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);


#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_H__ */