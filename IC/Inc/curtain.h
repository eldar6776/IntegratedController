/**
 ******************************************************************************
 * @file    curtain.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za potpuno enkapsulirani modul za kontrolu roletni.
 *
 * @note
 * Verzija 2.0: Kompletiran API koji odgovara originalnoj funkcionalnosti.
 * Modul koristi "Opaque Pointer" tehniku (Curtain_Handle) za potpunu
 * enkapsulaciju. Stvarna struktura `Curtain` i globalni niz `curtains`
 * su sakriveni unutar `curtain.c`.
 ******************************************************************************
 */

#ifndef __CURTAIN_CTRL_H__
#define __CURTAIN_CTRL_H__                             FW_BUILD // version

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I KONSTANTE                                               */
/*============================================================================*/

// --- Definicije za smjer kretanja roletne ---
#define CURTAIN_STOP     0
#define CURTAIN_UP       1
#define CURTAIN_DOWN     2

/*============================================================================*/
/* JAVNE STRUKTURE (ZA EEPROM)                                                */
/*============================================================================*/

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži KONFIGURACIJU jedne roletne za EEPROM.
 * @note  Ova struktura je javna kako bi `stm32746g_eeprom.h` (mapa EEPROM-a)
 * mogao da koristi `sizeof()` za automatsko izracunavanje adresa.
 * `#pragma pack` osigurava da nema "rupa" u memoriji, cineci snimanje
 * i citanje 100% pouzdanim.
 */
typedef struct {
    /** @brief Adresa releja za podizanje. Format zavisi od globalne postavke protokola. */
    union {
        uint16_t tf;            /**< Apsolutna adresa za TinyFrame. */
        struct { uint16_t mod; uint8_t pin; } mb; /**< Par adresa za Modbus. */
    } relayUp;

    /** @brief Adresa releja za spuštanje. Format zavisi od globalne postavke protokola. */
    union {
        uint16_t tf;            /**< Apsolutna adresa za TinyFrame. */
        struct { uint16_t mod; uint8_t pin; } mb; /**< Par adresa za Modbus. */
    } relayDown;
} Curtain_EepromConfig_t;

/**
 * @brief Struktura koja objedinjuje SVE podatke o roletnama za EEPROM.
 * @note  Cuva se i cita kao jedan blok, zašticen magicnim brojem i CRC-om.
 */
typedef struct {
    uint16_t magic_number;          /**< "Potpis" za validaciju podataka. */
    uint8_t  upDownDurationSeconds; /**< Globalno vrijeme kretanja za sve roletne. */
    Curtain_EepromConfig_t curtains[CURTAINS_SIZE]; /**< Niz konfiguracija za sve roletne. */
    uint16_t crc;                   /**< CRC za provjeru integriteta cijelog bloka. */
} Curtains_EepromData_t;
#pragma pack(pop)

/*============================================================================*/
/* OPAQUE (SAKRIVENI) TIP PODATAKA                                            */
/*============================================================================*/
/**
 * @brief  "Opaque" (sakriveni) tip za jednu roletnu.
 * @note   Stvarna definicija ove strukture je sakrivena unutar `curtain.c`.
 * Pristup je moguc iskljucivo preko pointera (handle-a) koji vracaju
 * funkcije `Curtain_GetInstanceByIndex` ili `Curtain_GetByLogicalIndex`.
 */
typedef struct Curtain_s Curtain_Handle;

/*============================================================================*/
/* EKSTERNE VARIJABLE (IZ DRUGIH MODULA)                                      */
/*============================================================================*/
/**
 * @brief Indeks trenutno odabrane roletne u GUI-ju.
 * @note  Ova varijabla pripada `display` modulu i definiše se u `display.c`.
 * Zadržana je radi kompatibilnosti sa postojecom logikom korisnickog
 * interfejsa. Vrijednost jednaka `Curtains_getCount()` oznacava
 * da su odabrane "SVE" roletne.
 */
extern uint8_t curtain_selected;

/*============================================================================*/
/* PROTOTIPOVI FUNKCIJA (KOMPLETAN JAVNI API)                                 */
/*============================================================================*/

// --- Grupa 1: Inicijalizacija i Servis ---

/**
 * @brief  Inicijalizuje `curtain` modul.
 * @note   Ucitava kompletnu konfiguraciju svih roletni iz EEPROM-a,
 * vrši validaciju podataka pomocu magicnog broja i CRC-a,
 * i postavlja pocetno stanje za sve roletne.
 * Poziva se jednom pri startu sistema iz `main()`.
 */
void Curtains_Init(void);

/**
 * @brief  Glavna servisna petlja za `curtain` modul.
 * @note   Ova funkcija prolazi kroz sve konfigurisane roletne i upravlja
 * njihovim stanjem, tajmerima i slanjem komandi na RS485 bus.
 * Mora se pozivati periodicno iz glavne `while(1)` petlje.
 */
void Curtain_Service(void);


// --- Grupa 2: Konfiguracija i Cuvanje ---

/**
 * @brief  Snima trenutnu konfiguraciju svih roletni u EEPROM.
 * @note   Automatski izracunava i upisuje CRC za cijeli blok podataka.
 */
void Curtains_Save(void);

/**
 * @brief  Postavlja sve konfiguracione parametre roletni na sigurne fabricke vrijednosti.
 * @note   Ova funkcija ne snima promjene u EEPROM; nakon njenog poziva
 * potrebno je pozvati `Curtains_Save()` da bi se promjene sacuvale.
 */
void Curtains_SetDefault(void);

/**
 * @brief  Postavlja globalno vrijeme kretanja roletni (u sekundama).
 * @param  seconds Vrijeme kretanja (0-255 sekundi).
 */
void Curtain_SetMoveTime(const uint8_t seconds);

/**
 * @brief  Dobija globalno vrijeme kretanja roletni.
 * @retval uint8_t Vrijeme kretanja u sekundama.
 */
uint8_t Curtain_GetMoveTime(void);

/**
 * @brief  Postavlja Modbus adresu za relej "GORE" za odredenu roletnu.
 * @param  handle Pointer na roletnu (dobijen od `GetInstanceByIndex`).
 * @param  relay Modbus adresa releja.
 */
void Curtain_setRelayUp(Curtain_Handle* const handle, uint16_t relay);

/**
 * @brief  Dobija Modbus adresu za relej "GORE" za odredenu roletnu.
 * @param  handle Pointer na roletnu.
 * @retval uint16_t Modbus adresa releja.
 */
uint16_t Curtain_getRelayUp(const Curtain_Handle* const handle);

/**
 * @brief  Postavlja Modbus adresu za relej "DOLE" za odredenu roletnu.
 * @param  handle Pointer na roletnu (dobijen od `GetInstanceByIndex`).
 * @param  relay Modbus adresa releja.
 */
void Curtain_setRelayDown(Curtain_Handle* const handle, uint16_t relay);

/**
 * @brief  Dobija Modbus adresu za relej "DOLE" za odredenu roletnu.
 * @param  handle Pointer na roletnu.
 * @retval uint16_t Modbus adresa releja.
 */
uint16_t Curtain_getRelayDown(const Curtain_Handle* const handle);


// --- Grupa 3: Pristup i Brojaci ---

/**
 * @brief  Vraca pointer (handle) na instancu roletne na osnovu fizickog indeksa.
 * @param  index Fizicki indeks u nizu (0 do CURTAINS_SIZE - 1).
 * @retval Curtain_Handle* Pointer na instancu, ili `NULL` ako je indeks neispravan.
 */
Curtain_Handle* Curtain_GetInstanceByIndex(uint8_t index);

/**
 * @brief  Pronalazi handle za roletnu na osnovu logickog indeksa.
 * @note   Logicki indeks predstavlja redni broj konfigurisane roletne.
 * Npr. ako su konfigurisane roletne na fizickim indeksima 2, 5 i 7,
 * logicki indeks 0 ce vratiti roletnu sa indeksa 2,
 * logicki indeks 1 ce vratiti roletnu sa indeksa 5, itd.
 * @param  logical_index Redni broj roletne (0 = prva konfigurisana, 1 = druga...).
 * @return Curtain_Handle* Pokazivac na instancu ili `NULL` ako nije pronadena.
 */
Curtain_Handle* Curtain_GetByLogicalIndex(const uint8_t logical_index);

/**
 * @brief  Vraca ukupan broj STVARNO konfigurisanih roletni.
 * @retval uint8_t Broj roletni koje imaju definisane releje.
 */
uint8_t Curtains_getCount(void);


// --- Grupa 4: Selekcija (za GUI) ---

/**
 * @brief  Postavlja koja je roletna trenutno odabrana u GUI-ju.
 * @param  curtain_index Logicki indeks odabrane roletne.
 */
void Curtain_Select(const uint8_t curtain_index);

/**
 * @brief  Vraca logicki indeks trenutno odabrane roletne.
 * @retval uint8_t Logicki indeks.
 */
uint8_t Curtain_getSelected(void);

/**
 * @brief  Provjerava da li je u GUI-ju odabrana opcija "SVE" roletne.
 * @retval bool `true` ako su sve roletne odabrane, inace `false`.
 */
bool Curtain_areAllSelected(void);

/**
 * @brief  Resetuje selekciju u GUI-ju na opciju "SVE".
 */
void Curtain_ResetSelection(void);


// --- Grupa 5: Kontrola i Upravljanje ---

/**
 * @brief  Zaustavlja kretanje jedne, specificne roletne.
 * @param  handle Pointer na roletnu koju treba zaustaviti.
 */
void Curtain_Stop(Curtain_Handle* const handle);

/**
 * @brief  Zaustavlja kretanje svih konfigurisanih roletni.
 */
void Curtains_StopAll(void);

/**
 * @brief  Pokrece kretanje jedne roletne u specificiranom smjeru.
 * @param  handle Pointer na roletnu.
 * @param  direction Smjer kretanja (`CURTAIN_UP` ili `CURTAIN_DOWN`).
 */
void Curtain_Move(Curtain_Handle* const handle, const uint8_t direction);

/**
 * @brief  Pokrece kretanje svih konfigurisanih roletni u istom smjeru.
 * @param  direction Smjer kretanja (`CURTAIN_UP` ili `CURTAIN_DOWN`).
 */
void Curtains_MoveAll(const uint8_t direction);

/**
 * @brief  "Pametno" pokrece/zaustavlja jednu roletnu (toggle).
 * @note   Ako roletna vec ide u željenom smjeru, zaustavice je.
 * Ako miruje ili ide u suprotnom smjeru, pokrenuce je.
 * @param  handle Pointer na roletnu.
 * @param  direction Željeni smjer.
 */
void Curtain_MoveSignal(Curtain_Handle* const handle, const uint8_t direction);

/**
 * @brief  "Pametno" pokrece/zaustavlja sve roletne (toggle).
 * @note   Ako se ijedna roletna krece u željenom smjeru, zaustavice sve.
 * Inace, pokrenuce sve u željenom smjeru.
 * @param  direction Željeni smjer.
 */
void Curtains_MoveSignalAll(const uint8_t direction);

/**
 * @brief  Centralna logika za obradu dodira na ekranu za roletne.
 * @note   Poziva se iz `display.c` i odlucuje da li treba pokrenuti
 * jednu ili sve roletne na osnovu `curtain_selected` varijable.
 * @param  direction Smjer dobijen sa GUI-ja (`CURTAIN_UP` ili `CURTAIN_DOWN`).
 */
void Curtain_HandleTouchLogic(const uint8_t direction);

/**
 * @brief  Ažurira stanje roletne na osnovu eksterne komande (npr. sa RS485).
 * @param  relay Adresa releja koji je promijenio stanje.
 * @param  state Novo stanje (`CURTAIN_UP`, `CURTAIN_DOWN`, `CURTAIN_STOP`).
 */
void Curtain_Update_External(uint16_t relay, uint8_t state);


// --- Grupa 6: Provjera Stanja (Runtime Getters) ---

/**
 * @brief  Provjerava da li roletna ima konfigurisane releje.
 * @param  handle Pointer na roletnu.
 * @retval bool `true` ako ima, inace `false`.
 */
bool Curtain_hasRelays(const Curtain_Handle* const handle);

/**
 * @brief  Provjerava da li se roletna trenutno krece.
 * @param  handle Pointer na roletnu.
 * @retval bool `true` ako se krece, inace `false`.
 */
bool Curtain_isMoving(const Curtain_Handle* const handle);

/**
 * @brief  Provjerava da li se roletna trenutno krece GORE.
 * @param  handle Pointer na roletnu.
 * @retval bool `true` ako se krece gore, inace `false`.
 */
bool Curtain_isMovingUp(const Curtain_Handle* const handle);

/**
 * @brief  Provjerava da li se roletna trenutno krece DOLE.
 * @param  handle Pointer na roletnu.
 * @retval bool `true` ako se krece dole, inace `false`.
 */
bool Curtain_isMovingDown(const Curtain_Handle* const handle);

/**
 * @brief  Provjerava da li se ijedna od svih roletni trenutno krece.
 * @retval bool `true` ako se bar jedna krece, inace `false`.
 */
bool Curtains_isAnyCurtainMoving(void);

/**
 * @brief  Provjerava da li se ijedna roletna krece GORE.
 * @retval bool `true` ako se bar jedna krece gore, inace `false`.
 */
bool Curtains_isAnyCurtainMovingUp(void);

/**
 * @brief  Provjerava da li se ijedna roletna krece DOLE.
 * @retval bool `true` ako se bar jedna krece dole, inace `false`.
 */
bool Curtains_isAnyCurtainMovingDown(void);

/**
 * @brief  Provjerava da li se sve roletne krecu u istom smjeru.
 * @param  direction Smjer koji se provjerava (`CURTAIN_UP` ili `CURTAIN_DOWN`).
 * @retval bool `true` ako se sve krecu u datom smjeru, inace `false`.
 */
bool Curtains_areAllMovinginSameDirection(const uint8_t direction);

/**
 * @brief  Dobija novo željeno stanje (smjer) roletne.
 * @param  handle Pointer na roletnu.
 * @retval uint8_t Stanje (`CURTAIN_STOP`, `CURTAIN_UP`, `CURTAIN_DOWN`).
 */
uint8_t Curtain_getNewDirection(const Curtain_Handle* const handle);

#endif // __CURTAIN_CTRL_H__
