/**
 ******************************************************************************
 * @file    gate.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul koji upravlja kapijama, bravama i rampama.
 *
 * @note    Verzija 2.0: Potpuno redizajniran API za podršku dinamičkoj,
 * metapodacima vođenoj arhitekturi. Modul koristi Univerzalnu State
 * Mašinu (USM) čije se ponašanje definiše kroz "Profile Kontrole".
 * Ovaj header fajl je isključivo odgovoran za definisanje logike
 * ponašanja i struktura podataka. Sve vizuelne reprezentacije
 * (ikonice) su definisane u display.h.
 ******************************************************************************
 */

#ifndef __GATE_CTRL_H__
#define __GATE_CTRL_H__                              FW_BUILD // Verzija

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I ENUMERATORI                                             */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief Definiše LOGIKU PONAŠANJA (Profil Kontrole) za Univerzalnu State Mašinu.
 * @note  Ovo je ključni parametar koji određuje kako će se tumačiti komande
 * i signali sa senzora. Svaki član enuma odgovara jednom unosu u
 * `g_ControlProfileLibrary` nizu u `gate.c`.
 ******************************************************************************
 */
typedef enum {
    CONTROL_TYPE_NONE,                  /**< Profil nije odabran; uređaj je neaktivan. */
    CONTROL_TYPE_BFT_STEP_BY_STEP,      /**< Profil za BFT motore sa "Step-by-Step" logikom. */
    CONTROL_TYPE_NICE_SLIDING_PULSE,    /**< Profil za NICE klizne motore sa odvojenim pulsnim komandama. */
    CONTROL_TYPE_SIMPLE_LOCK,           /**< Profil za jednostavne pametne brave. */
    CONTROL_TYPE_GENERIC_MAINTAINED,    /**< Profil za motore sa kontinuiranim signalom (ne-pulsni). */
    CONTROL_TYPE_RAMP_PULSE,            /**< Profil za rampe sa odvojenim pulsnim komandama GORE/DOLE. */
    CONTROL_TYPE_SIMPLE_STEP_BY_STEP,   /**< Profil za motore sa samo jednom S-S komandom (bez pješaka). */
} GateControlType_e;


/**
 ******************************************************************************
 * @brief Definiše moguće komande koje korisnik može pokrenuti iz UI.
 * @note  Služi kao indeks za `command_map` niz unutar `ProfilDeskriptor_t`
 * strukture, prevodeći akciju korisnika u fizičku komandu.
 ******************************************************************************
 */
typedef enum {
    UI_COMMAND_NONE,
    UI_COMMAND_SMART_STEP,          /**< Kratki klik na ikonicu (toggle: otvori/stop/zatvori/stop). */
    UI_COMMAND_OPEN_CYCLE,          /**< Eksplicitna komanda "Otvori". */
    UI_COMMAND_CLOSE_CYCLE,         /**< Eksplicitna komanda "Zatvori". */
    UI_COMMAND_PEDESTRIAN,          /**< Eksplicitna komanda "Pješak". */
    UI_COMMAND_STOP,                /**< Eksplicitna komanda "Stop". */
    UI_COMMAND_UNLOCK               /**< Eksplicitna komanda "Otključaj" za brave. */
} UI_Command_e;


/**
 ******************************************************************************
 * @brief Definiše moguća stanja u kojima se uređaj može naći.
 * @note  Koristi ih Univerzalna State Mašina (USM) za praćenje ciklusa.
 * Stanje se primarno ažurira na osnovu signala sa senzora.
 ******************************************************************************
 */
typedef enum {
    GATE_STATE_UNDEFINED,       /**< Stanje nepoznato (npr. nakon restarta bez senzora). */
    GATE_STATE_CLOSED,          /**< Potvrđeno zatvoreno (senzor aktivan). */
    GATE_STATE_OPENING,         /**< U procesu otvaranja. */
    GATE_STATE_OPEN,            /**< Potvrđeno otvoreno (senzor aktivan). */
    GATE_STATE_CLOSING,         /**< U procesu zatvaranja. */
    GATE_STATE_PARTIALLY_OPEN,  /**< Zaustavljeno u međupoziciji. */
    GATE_STATE_FAULT,           /**< Greška (npr. timeout ciklusa). */
} GateState_e;


/**
 ******************************************************************************
 * @brief Definiše tip aktivnog tajmera unutar gate modula.
 * @note  Koristi se za upravljanje logikom u `Gate_Service` funkciji.
 ******************************************************************************
 */
typedef enum {
    GATE_TIMER_NONE,            /**< Nijedan tajmer nije aktivan. */
    GATE_TIMER_CYCLE,           /**< Aktivan je tajmer za puni ciklus (za detekciju greške). */
    GATE_TIMER_PEDESTRIAN,      /**< Aktivan je tajmer za pješački mod. */
    GATE_TIMER_PULSE            /**< Aktivan je kratki tajmer za impuls na releju. */
} GateTimerType_e;


/**
 ******************************************************************************
 * @brief Bitmaske koje definišu vidljivost pojedinih postavki u UI meniju.
 * @note  Koristi se u `ProfilDeskriptor_t` da bi se korisniku prikazale samo
 * relevantne opcije za odabrani tip motora.
 ******************************************************************************
 */
#define SETTING_VISIBLE_RELAY_CMD1      (1 << 0)  /**< Pokaži postavku za generičku komandu 1 (npr. Otvori, S-S). */
#define SETTING_VISIBLE_RELAY_CMD2      (1 << 1)  /**< Pokaži postavku za generičku komandu 2 (npr. Zatvori, Pješak). */
#define SETTING_VISIBLE_RELAY_CMD3      (1 << 2)  /**< Pokaži postavku za generičku komandu 3 (npr. Stop). */
#define SETTING_VISIBLE_RELAY_CMD4      (1 << 3)  /**< Pokaži postavku za generičku komandu 4. */
#define SETTING_VISIBLE_FEEDBACK_1      (1 << 4)  /**< Pokaži postavku za generički senzor 1 (npr. Otvoreno). */
#define SETTING_VISIBLE_FEEDBACK_2      (1 << 5)  /**< Pokaži postavku za generički senzor 2 (npr. Zatvoreno). */
#define SETTING_VISIBLE_FEEDBACK_3      (1 << 6)  /**< Pokaži postavku za generički senzor 3. */
#define SETTING_VISIBLE_CYCLE_TIMER     (1 << 7)  /**< Pokaži postavku za tajmer punog ciklusa. */
#define SETTING_VISIBLE_PED_TIMER       (1 << 8)  /**< Pokaži postavku za tajmer pješačkog moda. */
#define SETTING_VISIBLE_PULSE_TIMER     (1 << 9)  /**< Pokaži postavku za trajanje impulsa. */


/**
 ******************************************************************************
 * @brief Definicija fizičke akcije koja se izvršava na releju.
 * @note  Koristi se u `command_map` nizu unutar Profil Deskriptora.
 ******************************************************************************
 */
typedef struct {
    uint8_t target_relay_index;     /**< Koji generički relej se aktivira (1-4 za cmd1-cmd4). 0 znači da nema akcije. */
    bool is_pulse;                  /**< Da li je signal pulsni ili kontinuirani. */
} GateAction_t;


/**
 ******************************************************************************
 * @brief Definicija "Profil Deskriptora" - šablona za ponašanje uređaja.
 * @note  Ovo je srž nove arhitekture. Svaki element u `g_ControlProfileLibrary`
 * nizu je ovog tipa i u potpunosti definiše jedan tip motora/brave.
 ******************************************************************************
 */
typedef struct {
    GateControlType_e profile_id;       /**< Jedinstveni ID profila (iz enuma `GateControlType_e`). */
    const char* profile_name;           /**< Naziv za prikaz u UI (npr. "BFT S-S"). */
    uint32_t visible_settings_mask;     /**< Maska bitova (`SETTING_VISIBLE_*`) za prikaz postavki u meniju. */
    GateAction_t command_map[8];        /**< Mapa koja prevodi `UI_Command_e` u fizičku akciju `GateAction_t`. */
} ProfilDeskriptor_t;


#pragma pack(push, 1)
/**
 ******************************************************************************
 * @brief       Struktura koja sadrži kompletnu konfiguraciju za jedan uređaj.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je struktura koja se čuva u EEPROM-u. Sva polja su
 * generička, a njihovo značenje tumači Univerzalna State Mašina
 * na osnovu odabranog `control_type`. Struktura je kompaktna
 * zahvaljujući `#pragma pack`.
 ******************************************************************************
 */
typedef struct
{
    /** @brief "Magični broj" za validaciju podataka u EEPROM-u. Mora biti `EEPROM_MAGIC_NUMBER`. */
    uint16_t magic_number;

    /** @brief Korisnički definisan naziv uređaja (npr. "GARAŽA DONJA"). Prazan string znači da se koristi podrazumijevani naziv iz tabele izgleda. */
    char custom_label[21];
    
    /** @brief ID logike ponašanja (`GateControlType_e`) koju uređaj koristi. Ovo je indeks u `g_ControlProfileLibrary`. */
    GateControlType_e control_type;

    /** @brief ID vizuelnog prikaza. Ovo je indeks u `gate_appearance_mapping_table` u `translations.h`. */
    uint8_t appearance_id;

    /** @brief Generička adresa za prvu komandu (npr. OTVORI, STEP-BY-STEP). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } relay_cmd1;

    /** @brief Generička adresa za drugu komandu (npr. ZATVORI, PJEŠAK). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } relay_cmd2;

    /** @brief Generička adresa za treću komandu (npr. PJEŠAK, STOP). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } relay_cmd3;
    
    /** @brief Generička adresa za četvrtu komandu (npr. STOP). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } relay_cmd4;

    /** @brief Generička adresa za prvi senzor (npr. senzor OTVORENO). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } feedback_input1;
    
    /** @brief Generička adresa za drugi senzor (npr. senzor ZATVORENO). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } feedback_input2;
    
    /** @brief Generička adresa za treći senzor (npr. senzor kretanja - lampa). */
    union { uint16_t tf; struct { uint16_t mod; uint8_t pin; } mb; } feedback_input3;
    
    /** @brief Vrijeme (u sekundama) za puni ciklus. Koristi se za detekciju greške (timeout). 0 = isključeno. */
    uint8_t  cycle_timer_s;
    
    /** @brief Vrijeme (u sekundama) za softverski pješački mod. 0 = nije konfigurisano. */
    uint8_t  pedestrian_timer_s;
    
    /** @brief Trajanje impulsa (u milisekundama) za sve pulsne komande. */
    uint16_t pulse_timer_ms;

    /** @brief CRC (Cyclic Redundancy Check) za provjeru integriteta podataka. */
    uint16_t crc;
} Gate_EepromConfig_t;
#pragma pack(pop)


/** @brief  "Opaque" (sakriveni) tip za jedan uređaj (kapiju, bravu, rampu). Služi kao handle za sve javne funkcije. */
typedef struct Gate_s Gate_Handle;


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

// --- Glavne funkcije ---
void Gate_Init(void);
void Gate_Service(void);
void Gate_Save(void);
Gate_Handle* Gate_GetInstance(uint8_t index);
uint8_t Gate_GetCount(void);

// --- Komandne funkcije ---
void Gate_TriggerSmartStep(Gate_Handle* handle);
void Gate_TriggerFullCycleOpen(Gate_Handle* handle);
void Gate_TriggerFullCycleClose(Gate_Handle* handle);
void Gate_TriggerPedestrian(Gate_Handle* handle);
void Gate_TriggerStop(Gate_Handle* handle);
void Gate_TriggerUnlock(Gate_Handle* handle);

// --- Obrada događaja ---
void GATE_BusEvent(uint16_t address, uint8_t command, uint8_t* data, uint8_t len);
void Gate_AcknowledgeFault(Gate_Handle* handle);

/*============================================================================*/
/* JAVNI API - GETTERI I SETTERI ZA KONFIGURACIJU                             */
/*============================================================================*/

// --- Getteri za konfiguraciju ---
GateControlType_e Gate_GetControlType(const Gate_Handle* handle);
uint8_t           Gate_GetAppearanceId(const Gate_Handle* handle);
const char* Gate_GetCustomLabel(const Gate_Handle* handle);
uint16_t          Gate_GetRelayAddr(const Gate_Handle* handle, uint8_t relay_index);
uint16_t          Gate_GetFeedbackAddr(const Gate_Handle* handle, uint8_t sensor_index);
uint8_t           Gate_GetCycleTimer(const Gate_Handle* handle);
uint8_t           Gate_GetPedestrianTimer(const Gate_Handle* handle);
uint16_t          Gate_GetPulseTimer(const Gate_Handle* handle);

// --- Getteri za runtime stanje ---
GateState_e Gate_GetState(const Gate_Handle* handle);
/**
 ******************************************************************************
 * @brief       Direktno postavlja interno stanje mašine stanja za dati handle.
 * @author      Gemini
 * @note        Ova funkcija je namijenjena za "optimistic UI update". Omogućava
 * frontendu da trenutno promijeni stanje radi vizuelnog feedbacka,
 * bez pokretanja backend logike.
 * @param       handle Pokazivač na instancu uređaja.
 * @param       new_state Novo stanje iz `GateState_e` enuma.
 ******************************************************************************
 */
void Gate_SetState(Gate_Handle* handle, GateState_e new_state);

// --- Getteri za pristup Biblioteci Profila (za UI) ---
const ProfilDeskriptor_t* Gate_GetProfilDeskriptor(const Gate_Handle* handle);
const char* Gate_GetProfileNameByIndex(uint8_t index);
uint8_t                   Gate_GetProfileCount(void);

// --- Setteri za konfiguraciju ---
void Gate_SetControlType(Gate_Handle* handle, GateControlType_e type);
void Gate_SetAppearanceId(Gate_Handle* handle, uint8_t id);
void Gate_SetCustomLabel(Gate_Handle* handle, const char* label);
void Gate_SetRelayAddr(Gate_Handle* handle, uint8_t relay_index, uint16_t addr);
void Gate_SetFeedbackAddr(Gate_Handle* handle, uint8_t sensor_index, uint16_t addr);
void Gate_SetCycleTimer(Gate_Handle* handle, uint8_t seconds);
void Gate_SetPedestrianTimer(Gate_Handle* handle, uint8_t seconds);
void Gate_SetPulseTimer(Gate_Handle* handle, uint16_t ms);

#endif // __GATE_CTRL_H__
