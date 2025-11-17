/**
 ******************************************************************************
 * @file    thermostat.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Header fajl za potpuno enkapsulirani modul termostata.
 *
 * @note
 * Ovaj modul koristi "Opaque Pointer" tehniku da sakrije svoju internu
 * strukturu podataka od ostatka aplikacije. Sva interakcija sa termostatom
 * se mora odvijati iskljucivo preko funkcija definisanih u ovom API-ju.
 * Instanci modula se pristupa preko Thermostat_GetInstance() funkcije.
 ******************************************************************************
 */
 
#ifndef __TEMP_CTRL_H__
#define __TEMP_CTRL_H__                             FW_BUILD // version

#include "main.h"
#include "stm32f7xx.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I KONSTANTE                                               */
/*============================================================================*/
#define EEPROM_THERMOSTAT_CONFIG_SIZE               32      // Rucno definisane velicine na osnovu pravila "velicina * 2 zaokruženo na najbliži višekratnik 16"
#define FANC_FAN_MIN_ON_TIME                        560U    // Minimalno vrijeme (u ms) izmedu promjena brzine ventilatora.
#define THST_SP_MIN                                 5       // Minimalna dozvoljena zadata temperatura (°C).
#define THST_SP_MAX                                 40      // Maksimalna dozvoljena zadata temperatura (°C).
#define THST_OFF                                    0       //  (0=OFF, 1=Hladenje, 2=Grijanje).
#define THST_COOLING                                1    
#define THST_HEATING                                2
#define THST_MASTER                                 0       // MASTER:SLAVE grupa termostata
#define THST_SLAVE                                  1

/*============================================================================*/
/* JAVNA STRUKTURA (SAMO ZA EEPROM MAPU)                                      */
/*============================================================================*/

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži iskljucivo podatke za termostat koji se
 * trajno cuvaju u EEPROM.
 * @note Ova definicija je javna kako bi EEPROM mapa mogla dinamicki
 * izracunati svoju velicinu koristeci sizeof().
 */
typedef struct {
    uint16_t magic_number;
    uint8_t group;
    bool    master;
    uint8_t  th_ctrl;
    uint8_t  sp_temp;
    uint8_t  sp_max;
    uint8_t  sp_min;
    uint8_t  sp_diff;
    int8_t   mv_offset;
    uint8_t  fan_ctrl;
    uint8_t  fan_diff;
    uint8_t  fan_loband;
    uint8_t  fan_hiband;
    uint16_t crc;
} THERMOSTAT_EepromConfig_t;
#pragma pack(pop)


/*============================================================================*/
/* OPAQUE TIP PODATAKA                                                        */
/*============================================================================*/

/**
 * @brief  "Opaque" (sakriveni) tip za termostat.
 * @note   Stvarna definicija ove strukture je sakrivena unutar thermostat.c
 * i nije dostupna ostatku programa. Pristup je moguc samo preko pointera (handle-a).
 */
typedef struct THERMOSTAT_TypeDef_s THERMOSTAT_TypeDef;


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

// --- Grupa 1: Upravljanje Instancom i Osnovne Funkcije ---

/**
 * @brief  Vraca pointer na jedinu (singleton) instancu termostata.
 * @note   Ovo je jedini ispravan nacin da se dobije "handle" za rad sa termostatom.
 * @param  None
 * @retval THERMOSTAT_TypeDef* Pointer (handle) na instancu termostata.
 */
THERMOSTAT_TypeDef* Thermostat_GetInstance(void);

/**
 * @brief  Inicijalizuje modul termostata.
 * @note   Ucitava postavke iz EEPROM-a, vrši validaciju i postavlja pocetno stanje.
 * Poziva se jednom pri startu sistema.
 * @param  handle Pointer na instancu termostata dobijen sa Thermostat_GetInstance().
 * @retval None
 */
void THSTAT_Init(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Cuva trenutnu konfiguraciju termostata u EEPROM.
 * @note   Funkcija automatski racuna i upisuje CRC radi integriteta podataka.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void THSTAT_Save(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Glavna servisna petlja za termostat.
 * @note   Ova funkcija sadrži logiku za kontrolu grijanja/hladenja i ventilatora.
 * Treba je pozivati periodicno iz glavne `while(1)` petlje.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void THSTAT_Service(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja sve postavke termostata na sigurne fabricke vrijednosti.
 * @note   Ova funkcija se interno poziva ako podaci u EEPROM-u nisu validni.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void Thermostat_SetDefault(THERMOSTAT_TypeDef* const handle);


// --- Grupa 2: Kontrola Zadate Temperature (Setpoint) ---

/**
 * @brief  Dobija trenutnu zadanu temperaturu (setpoint).
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Zadata temperatura u °C.
 */
uint8_t Thermostat_GetSetpoint(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja novu zadanu temperaturu, uz validaciju.
 * @param  handle Pointer na instancu termostata.
 * @param  setpoint Nova željena temperatura u °C.
 * @retval None
 */
void Thermostat_SP_Temp_Set(THERMOSTAT_TypeDef* const handle, const uint8_t setpoint);

/**
 * @brief  Povecava zadanu temperaturu za 1°C.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void Thermostat_SP_Temp_Increment(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Smanjuje zadanu temperaturu za 1°C.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void Thermostat_SP_Temp_Decrement(THERMOSTAT_TypeDef* const handle);


// --- Grupa 3: Glavna Kontrola i Konfiguracija ---

/**
 * @brief  Dobija trenutni mod rada termostata.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Mod (0=OFF, 1=Hladenje, 2=Grijanje).
 */
uint8_t Thermostat_GetControlMode(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja mod rada termostata.
 * @param  handle Pointer na instancu termostata.
 * @param  mode Mod koji treba postaviti (0=OFF, 1=Hladenje, 2=Grijanje).
 * @retval None
 */
void Thermostat_SetControlMode(THERMOSTAT_TypeDef* const handle, uint8_t mode);

/**
 * @brief  Dobija maksimalnu dozvoljenu zadanu temperaturu.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Temperatura u °C.
 */
uint8_t Thermostat_Get_SP_Max(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja maksimalnu dozvoljenu zadanu temperaturu.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost za postavljanje.
 * @retval None
 */
void Thermostat_Set_SP_Max(THERMOSTAT_TypeDef* const handle, const uint8_t value);

/**
 * @brief  Dobija minimalnu dozvoljenu zadanu temperaturu.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Temperatura u °C.
 */
uint8_t Thermostat_Get_SP_Min(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja minimalnu dozvoljenu zadanu temperaturu.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost za postavljanje.
 * @retval None
 */
void Thermostat_Set_SP_Min(THERMOSTAT_TypeDef* const handle, const uint8_t value);

/**
 * @brief  Provjerava da li je termostat konfigurisan kao master.
 * @param  handle Pointer na instancu termostata.
 * @retval bool True ako je master, inace false.
 */
bool Thermostat_IsMaster(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja master status termostata.
 * @param  handle Pointer na instancu termostata.
 * @param  is_master True da postane master, false za slave.
 * @retval None
 */
void Thermostat_SetMaster(THERMOSTAT_TypeDef* const handle, bool is_master);


// --- Grupa 4: Kontrola Ventilatora ---

/**
 * @brief  Dobija prag za prelazak ventilatora na NISKU brzinu.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Vrijednost praga (°C x0.1 od zadate temperature).
 */
uint8_t Thermostat_GetFanLowBand(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja prag za prelazak ventilatora na NISKU brzinu.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost praga za postavljanje.
 * @retval None
 */
void Thermostat_SetFanLowBand(THERMOSTAT_TypeDef* const handle, uint8_t value);

/**
 * @brief  Dobija prag za prelazak ventilatora na VISOKU brzinu.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Vrijednost praga (°C x0.1 od zadate temperature).
 */
uint8_t Thermostat_GetFanHighBand(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja prag za prelazak ventilatora na VISOKU brzinu.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost praga za postavljanje.
 * @retval None
 */
void Thermostat_SetFanHighBand(THERMOSTAT_TypeDef* const handle, uint8_t value);
/**
 * @brief  Dobija diferencijal za promjenu brzine ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Vrijednost diferencijala (°C x0.1).
 */
uint8_t Thermostat_GetFanDifference(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja diferencijal za promjenu brzine ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost za postavljanje.
 * @retval None
 */
void Thermostat_SetFanDifference(THERMOSTAT_TypeDef* const handle, uint8_t value);

/**
 * @brief  Dobija trenutni mod rada ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Mod (0=ON/OFF, 1=3 Brzine).
 */
uint8_t Thermostat_GetFanControlMode(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja mod rada ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @param  mode Mod koji treba postaviti (0=ON/OFF, 1=3 Brzine).
 * @retval None
 */
void Thermostat_SetFanControlMode(THERMOSTAT_TypeDef* const handle, uint8_t mode);


// --- Grupa 5: Ocitavanje Stanja (Runtime Getters) ---

/**
 * @brief  Dobija trenutnu izmjerenu temperaturu.
 * @param  handle Pointer na instancu termostata.
 * @retval int16_t Izmjerena temperatura (vrijednost x10, npr. 235 = 23.5°C).
 */
int16_t Thermostat_GetMeasuredTemp(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Dobija trenutnu brzinu ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Brzina (0-3).
 */
uint8_t Thermostat_GetFanSpeed(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Provjerava da li je termostat trenutno aktivan (grijanje ili hladenje).
 * @param  handle Pointer na instancu termostata.
 * @retval bool True ako je aktivan, inace false.
 */
bool Thermostat_IsActive(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Provjerava da li je NTC senzor temperature konektovan.
 * @param  handle Pointer na instancu termostata.
 * @retval bool True ako je senzor konektovan, inace false.
 */
bool Thermostat_IsNtcConnected(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Provjerava da li postoji greška na NTC senzoru.
 * @param  handle Pointer na instancu termostata.
 * @retval bool True ako postoji greška, inace false.
 */
bool Thermostat_IsNtcError(THERMOSTAT_TypeDef* const handle);


// --- Grupa 6: Postavljanje Eksternih Podataka (External Setters) ---

/**
 * @brief  Postavlja vrijednost izmjerene temperature (poziva se iz ADC logike).
 * @param  handle Pointer na instancu termostata.
 * @param  temp Vrijednost temperature (x10, npr. 235 za 23.5°C).
 * @retval None
 */
void Thermostat_SetMeasuredTemp(THERMOSTAT_TypeDef* const handle, int16_t temp);

/**
 * @brief  Ažurira status NTC senzora (poziva se iz ADC logike).
 * @param  handle Pointer na instancu termostata.
 * @param  is_connected Da li je senzor konektovan.
 * @param  has_error Da li postoji greška.
 * @retval None
 */
void Thermostat_SetNtcStatus(THERMOSTAT_TypeDef* const handle, bool is_connected, bool has_error);

/**
 * @brief  Potpuno iskljucuje termostat i sve povezane funkcije.
 * @note   Ova funkcija postavlja mod rada na OFF i resetuje stanje ventilatora.
 * @param  handle Pointer na instancu termostata.
 * @retval None
 */
void Thermostat_TurnOff(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Dobija ID grupe kojoj termostat pripada.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t ID grupe (0 = samostalan rad).
 */
uint8_t Thermostat_GetGroup(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja ID grupe kojoj termostat pripada.
 * @param  handle Pointer na instancu termostata.
 * @param  value ID grupe za postavljanje.
 * @retval None
 */
void Thermostat_SetGroup(THERMOSTAT_TypeDef* const handle, uint8_t value);
/**
 * @brief  Dobija trenutno stanje regulatora (da li je aktivan).
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t 1 ako je aktivan, 0 ako nije.
 */
uint8_t Thermostat_GetState(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Dobija diferencijal (histerezu) za aktivaciju.
 * @param  handle Pointer na instancu termostata.
 * @retval uint8_t Vrijednost diferencijala (°C x0.1).
 */
uint8_t Thermostat_GetSetpointDifference(THERMOSTAT_TypeDef* const handle);

/**
 * @brief  Postavlja diferencijal (histerezu) za aktivaciju.
 * @param  handle Pointer na instancu termostata.
 * @param  value Vrijednost za postavljanje.
 * @retval None
 */
void Thermostat_SetSetpointDifference(THERMOSTAT_TypeDef* const handle, uint8_t value);

/**
 * @brief  setovanje flaga hasInfoChanged
 * @param  handle Pointer na instancu termostata.
 * @param  state Vrijednost za postavljanje.
 * @retval None
 */
void Thermostat_SetInfoChanged(THERMOSTAT_TypeDef* const handle, bool state);

#endif /* __TEMP_CTRL_H__ */