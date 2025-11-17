/**
 ******************************************************************************
 * @file    lights.h
 * @author  Edin & Gemini
 * @version V2.0.0
 * @date    15-08-2025
 * @brief   Javni API za potpuno enkapsulirani modul za kontrolu svjetala.
 *
 * @note
 * Ovaj modul koristi "Opaque Pointer" tehniku za potpunu enkapsulaciju
 * runtime podataka. Stvarna 'runtime' struktura (LIGHT_Modbus_CmdTypeDef)
 * i globalni niz svjetala su sakriveni unutar `lights.c`.
 *
 * Da biste radili sa nekim svjetlom, morate prvo dobiti "handle" (pointer)
 * na njega pozivanjem funkcije `LIGHTS_GetInstance(index)`. Taj handle se
 * zatim prosljeduje svim ostalim API funkcijama (getterima i setterima).
 *
 * Definicija `LIGHT_EepromConfig_t` strukture je ostala javna kako bi
 * drugi moduli (poput EEPROM mape) mogli da koriste `sizeof()` na njoj.
 *
 ******************************************************************************
 * @attention
 *
 * (C) COPYRIGHT 2025 JUBERA d.o.o. Sarajevo
 *
 * Sva prava zadržana.
 *
 ******************************************************************************
 */

#ifndef __LIGHTS_H__
#define __LIGHTS_H__                             FW_BUILD // version


// UKLJUCEN SAMO NEOPHODAN HEADER
#include "main.h"

/*============================================================================*/
/* DEFINICIJE I KONSTANTE                                                     */
/*============================================================================*/


/**
 * @brief Trajanje tajmera (u sekundama) za automatsko gašenje nocnog svjetla.
 */
#define LIGHT_NIGHT_TIMER_DURATION          15
/*============================================================================*/
/* STRUKTURE PODATAKA                                                         */
/*============================================================================*/
// === NOVO: STRUKTURA SAMO ZA EEPROM PODATKE SA GARANTOVANIM PAKOVANJEM ===
// ULOGA: Ova struktura sadrži iskljucivo podatke koji se trajno cuvaju.
// `#pragma pack` osigurava da nema "rupa" u memoriji, cineci snimanje
// i citanje 100% pouzdanim.
/*============================================================================*/

#pragma pack(push, 1) // Pocni memorijsko pakovanje
/**
 * @brief Struktura koja sadrži iskljucivo podatke koji se trajno cuvaju u EEPROM.
 * @note  Zahvaljujuci `#pragma pack`, ova struktura je kompaktna i njen raspored u
 * memoriji je garantovano isti, što je kljucno za pouzdan rad sa EEPROM-om.
 */
typedef struct
{
    /**
     * @brief "Magicni broj" koji služi kao potpis firmvera.
     * Koristi se u `LIGHT_Init` da bi se provjerilo da li su podaci u EEPROM-u validni
     * i da li poticu od ove verzije softvera. Glavna zaštita od praznog ili oštecenog EEPROM-a.
     */
    uint16_t magic_number;

    /**
     * @brief Adresa releja ili dimera. Format zavisi od globalne postavke protokola.
     * @note  Ovo je kljucna informacija za slanje komandi preko RS485 bus-a.
     * Koristi se u `HandleLightStatusChanges`.
     */
    union {
        uint16_t tf;            /**< Apsolutna adresa za TinyFrame protokol. */
        struct { uint16_t mod; uint8_t pin; } mb; /**< Par adresa (modul+pin) za Modbus protokol. */
    } address;

    /**
     * @brief Fleg (0 ili 1) koji oznacava da li je svjetlo povezano sa glavnim prekidacem.
     * Koristi se u `LIGHTS_isAnyLightOn` i `HandleRelease_MainScreenLogic` da bi se
     * znalo koja svjetla treba upaliti/ugasiti pritiskom na glavni prekidac na ekranu.
     */
    uint8_t  tiedToMainLight;

    /**
     * @brief Vrijeme u minutama nakon kojeg ce se svjetlo automatski ugasiti.
     * Ako je 0, funkcija je iskljucena. Koristi se u `HandleOffTimeTimers`.
     */
    uint8_t  off_time;

    /**
     * @brief ID ikonice koja ce se prikazivati na GUI-ju.
     * Koristi se kao indeks u `icon_mapping_table` nizu da bi se odabrala
     * odgovarajuca slicica i tekstualni opisi.
     */
    uint8_t  iconID;

    /**
     * @brief Adresa DRUGOG uredaja (npr. ventilatora) koji ce se aktivirati
     * zajedno sa ovim svjetlom.
     */
    union {
        uint16_t tf;            /**< Apsolutna adresa za TinyFrame protokol. */
        struct { uint16_t mod; uint8_t pin; } mb; /**< Par adresa (modul+pin) za Modbus protokol. */
    } controllerID_on;

    /**
     * @brief Vrijeme odgode u minutama za "delayed-on" tajmer.
     * Koristi se u `HandleOnDelayTimers` da odloži paljenje svjetla nakon
     * što je primljena eksterna komanda.
     */
    uint8_t  controllerID_on_delay;

    /**
     * @brief Sat (0-23) kada se svjetlo treba automatski upaliti.
     * Vrijednost -1 oznacava da je tajmer iskljucen.
     * Koristi se u `Handle_PeriodicEvents` za dnevni raspored paljenja.
     */
    int8_t   on_hour;

    /**
     * @brief Minuta (0-59) kada se svjetlo treba automatski upaliti.
     * Koristi se zajedno sa `on_hour`.
     */
    uint8_t  on_minute;

    /**
     * @brief Tip svjetla (1=BIN, 2=DIM, 3=COLOR).
     * Kljucna varijabla u `HandleLightStatusChanges` koja odlucuje kakav tip
     * komande treba poslati.
     */
    uint8_t  communication_type;

    /**
     * @brief ID lokalnog pina (GPIO ili PWM) na ovom uredaju.
     * Ako je vrijednost razlicita od nule, `SetPin` ili `PCA9685_SetOutput`
     * ce biti pozvani da direktno kontrolišu hardverski izlaz.
     */
    uint8_t  local_pin;

    /**
     * @brief Vrijeme mirovanja.
     * Trenutno se može podesiti u meniju, ali se nigdje u kodu ne koristi.
     */
    uint8_t  sleep_time;

    /**
     * @brief Definiše ponašanje svjetla na pritisak eksternog tastera (1=ON, 2=OFF, 3=FLIP).
     * Koristi se u `HandleExternalButtonActivity`.
     */
    uint8_t  button_external;

    /**
     * @brief Fleg (0 ili 1) koji odreduje da li dimer pamti posljednju svjetlinu.
     */
    uint8_t  rememberBrightness;

    /**
     * @brief Vrijednost svjetline od 0 do 100.
     */
    uint8_t  brightness;

    /**
     * @brief Korisnicki definisan naziv za svjetlo (npr. "Luster Dnevna").
     * @note  Maksimalno 20 karaktera + NUL terminator. Ako je ovo polje prazno,
     * sistem koristi defaultni, prevedeni naziv iz `icon_mapping_table`.
     */
    char     custom_label[21];

    /**
     * @brief CRC (Cyclic Redundancy Check) "digitalni otisak prsta".
     * Izracunava se i snima u `LIGHTS_Save`. Provjerava se u `LIGHTS_Init`
     * da bi se potvrdio integritet svih ostalih podataka u ovoj strukturi.
     */
    uint16_t crc;

} LIGHT_EepromConfig_t;
#pragma pack(pop) // Završi memorijsko pakovanje


/*============================================================================*/
/* OPAQUE (SAKRIVENI) TIP PODATAKA                                            */
/*============================================================================*/

/**
 * @brief  "Opaque" (sakriveni) tip za jedno svjetlo.
 * @note   Stvarna definicija ove strukture je sakrivena unutar lights.c
 * i nije dostupna ostatku programa. Pristup je moguc samo preko pointera (handle-a).
 */
typedef struct LIGHT_s LIGHT_Handle;

/*============================================================================*/
/* PROTOTIPOVI FUNKCIJA - NOVI JAVNI API                                      */
/*============================================================================*/

// --- Grupa 1: Inicijalizacija, Servis i Upravljanje Instancama ---
void LIGHTS_Init(void);
void LIGHT_Service(void);

/**
 * @brief  Vraca pointer (handle) na instancu svjetla na osnovu indeksa.
 * @param  index Logicki indeks svjetla (0 do LIGHTS_MODBUS_SIZE - 1).
 * @retval LIGHT_Handle* Pointer na instancu, ili NULL ako je indeks neispravan.
 */
LIGHT_Handle* LIGHTS_GetInstance(uint8_t index);

uint8_t LIGHTS_getCount(void);
uint8_t LIGHTS_Rows_getCount(void);

// --- Grupa 2: Konfiguracija i Cuvanje ---
void LIGHTS_Save(void);
void LIGHTS_SetDefault(void);

// --- Grupa 3: Getteri i Setteri za Konfiguraciju ---
uint16_t  LIGHT_GetRelay(const LIGHT_Handle* const handle);
void      LIGHT_SetRelay(LIGHT_Handle* const handle, const uint16_t val);
bool      LIGHT_isTiedToMainLight(const LIGHT_Handle* const handle);
void      LIGHT_SetTiedToMainLight(LIGHT_Handle* const handle, bool isTied);
uint8_t   LIGHT_GetOffTime(const LIGHT_Handle* const handle);
void      LIGHT_SetOffTime(LIGHT_Handle* const handle, const uint8_t val);
uint8_t   LIGHT_GetIconID(const LIGHT_Handle* const handle);
void      LIGHT_SetIconID(LIGHT_Handle* const handle, const uint8_t id);
uint16_t  LIGHT_GetControllerID(const LIGHT_Handle* const handle);
void      LIGHT_SetControllerID(LIGHT_Handle* const handle, uint16_t val);
uint8_t   LIGHT_GetOnDelayTime(const LIGHT_Handle* const handle);
void      LIGHT_SetOnDelayTime(LIGHT_Handle* const handle, uint8_t val);
int8_t    LIGHT_GetOnHour(const LIGHT_Handle* const handle);
void      LIGHT_SetOnHour(LIGHT_Handle* const handle, int8_t hour);
uint8_t   LIGHT_GetOnMinute(const LIGHT_Handle* const handle);
void      LIGHT_SetOnMinute(LIGHT_Handle* const handle, uint8_t minute);
uint8_t   LIGHT_GetCommunicationType(const LIGHT_Handle* const handle);
void      LIGHT_SetCommunicationType(LIGHT_Handle* const handle, uint8_t type);
uint8_t   LIGHT_GetLocalPin(const LIGHT_Handle* const handle);
void      LIGHT_SetLocalPin(LIGHT_Handle* const handle, uint8_t pin);
uint8_t   LIGHT_GetSleepTime(const LIGHT_Handle* const handle);
void      LIGHT_SetSleepTime(LIGHT_Handle* const handle, uint8_t time);
uint8_t   LIGHT_GetButtonExternal(const LIGHT_Handle* const handle);
void      LIGHT_SetButtonExternal(LIGHT_Handle* const handle, uint8_t mode);
bool      LIGHT_isBrightnessRemembered(const LIGHT_Handle* const handle);
void      LIGHT_SetRememberBrightness(LIGHT_Handle* const handle, const bool remember);
uint8_t   LIGHT_GetBrightness(const LIGHT_Handle* const handle);
void      LIGHT_SetBrightness(LIGHT_Handle* const handle, uint8_t brightness);
uint32_t  LIGHT_GetColor(const LIGHT_Handle* const handle);
void      LIGHT_SetColor(LIGHT_Handle* const handle, uint32_t color);
const char* LIGHT_GetCustomLabel(const LIGHT_Handle* const handle);
void        LIGHT_SetCustomLabel(LIGHT_Handle* const handle, const char* label);

// --- Grupa 4: Kontrola Stanja ---
void LIGHT_Flip(LIGHT_Handle* const handle);
void LIGHT_SetState(LIGHT_Handle* const handle, const bool state);
bool LIGHT_isActive(const LIGHT_Handle* const handle);

// --- Grupa 5: Provjera Tipova ---
bool LIGHT_isBinary(const LIGHT_Handle* const handle);
bool LIGHT_isDimmer(const LIGHT_Handle* const handle);
bool LIGHT_isRGB(const LIGHT_Handle* const handle);

// --- Grupa 6: Funkcije koje ne zavise od instance ---
bool LIGHTS_isAnyLightOn(void);

// --- Grupa 7: Funkcije za RS485 i eksterne module ---
void LIGHTS_UpdateExternalState(uint16_t relay_address, uint8_t state);
void LIGHTS_UpdateExternalBrightness(uint16_t relay_address, uint8_t brightness);

// --- Grupa 8: API za Nocni Tajmer ---
void    LIGHTS_StartNightTimer(void);
void    LIGHTS_StopNightTimer(void);
bool    LIGHTS_IsNightTimerActive(void);
uint8_t LIGHTS_GetNightTimerCountdown(void);

#endif // __LIGHTS_H__
