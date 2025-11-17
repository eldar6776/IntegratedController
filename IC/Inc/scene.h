/**
 ******************************************************************************
 * @file    scene.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul koji upravlja sistemskim scenama.
 *
 * @note    Ovaj modul enkapsulira kompletnu logiku za memorisanje, aktiviranje i
 * upravljanje korisnički definisanim scenama. Scene omogućavaju korisniku da
 * jednim dodirom postavi stanja više različitih uređaja (svjetla, roletne,
 * kapije, termostati) u unaprijed definisane pozicije.
 * Modul također upravlja globalnim stanjem sistema (npr. "Away Mode").
 ******************************************************************************
 */

#ifndef __SCENE_CTRL_H__
#define __SCENE_CTRL_H__                             FW_BUILD // verzija

#include "main.h"
#include "display.h"
#include "lights.h"
#include "curtain.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I ENUMERATORI                                             */
/*============================================================================*/

/*============================================================================*/
/* DODATNE DEFINICIJE ZA SECURITY SCENE                                       */
/*============================================================================*/

/** @brief Bitmaske za definisanje koje particije alarmnog sistema su naoružane. */
#define SECURITY_PARTITION_1    (1 << 0)    /**< Bit za particiju 1 (npr. Prizemlje) */
#define SECURITY_PARTITION_2    (1 << 1)    /**< Bit za particiju 2 (npr. Sprat) */
#define SECURITY_PARTITION_3    (1 << 2)    /**< Bit za particiju 3 (npr. Garaža) */
#define SECURITY_PARTITION_4    (1 << 3)    /**< Bit za particiju 4 (npr. Perimetar) */
#define SECURITY_ARM_ALL        (SECURITY_PARTITION_1 | SECURITY_PARTITION_2 | SECURITY_PARTITION_3 | SECURITY_PARTITION_4) /**< Maska za sve particije. */
/**
 * @brief Definiše globalna stanja sistema, primarno za "Away" logiku.
 */
typedef enum {
    SYSTEM_STATE_HOME,          /**< Normalno stanje, ukućani su prisutni. */
    SYSTEM_STATE_AWAY_SETTLING, /**< Stanje nakon aktivacije "Leaving" scene, traje kratko vrijeme. */
    SYSTEM_STATE_AWAY_ACTIVE    /**< Sistem je u "Away" modu, aktivna je simulacija prisustva i čekaju se "Homecoming" okidači. */
} SystemState_e;

/**
 ******************************************************************************
 * @brief       Struktura koja definiše vizuelni izgled jedne scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Niz ovih struktura je definisan u `translations.h` kao
 * `scene_appearance_table[]`. Služi kao predefinisana
 * biblioteka izgleda (ikona + naziv) koje korisnik može
 * dodijeliti sceni prilikom kreiranja ili editovanja u "čarobnjaku".
 ******************************************************************************
 */
typedef struct {
    IconID icon_id;             /**< ID ikonice iz `display.h` koja predstavlja scenu. */
    TextID text_id;             /**< ID teksta iz `display.h` koji služi kao naziv scene. */
} SceneAppearance_t;

/**
 * @brief Definiše tip scene, što utiče na logiku koja se izvršava pri aktivaciji.
 */
typedef enum {
    SCENE_TYPE_STANDARD,      /**< Standardna scena, samo postavlja memorisana stanja uređaja. */
    SCENE_TYPE_LEAVING,       /**< Specijalna scena koja aktivira SYSTEM_STATE_AWAY_ACTIVE. */
    SCENE_TYPE_HOMECOMING,    /**< Specijalna scena koja deaktivira "Away" mod. */
    SCENE_TYPE_SLEEP,         /**< Specijalna scena koja može imati povezan tajmer za buđenje. */
} SceneType_e;


/*============================================================================*/
/* JAVNE STRUKTURE (ZA EEPROM)                                                */
/*============================================================================*/

#pragma pack(push, 1)
/**
 ******************************************************************************
 * @brief       Glavna struktura koja čuva kompletnu konfiguraciju i stanje JEDNE scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Niz ovih struktura se čuva unutar `Scene_EepromBlock_t` omotača,
 * što omogućava efikasno snimanje i čitanje iz EEPROM-a.
 * Sadrži maske za uključene uređaje i polja za njihove
 * memorisane vrijednosti.
 ******************************************************************************
 */
typedef struct
{
    /**
     * @brief Indeks u `scene_appearance_table[]` koji definiše ikonicu i naziv scene.
     * @note  Ova vrijednost se bira u "čarobnjaku" za scene i određuje vizuelni
     * prikaz scene na glavnom ekranu.
     */
    uint8_t  appearance_id;

    /**
     * @brief Fleg koji označava da li je scena konfigurisana od strane korisnika.
     * @note  Vrijednost je `false` za defaultnu, praznu scenu; postaje `true`
     * čim je korisnik prvi put snimi. Koristi se u GUI-ju za
     * razlikovanje praznih slotova (koji prikazuju "wizard" ikonicu)
     * od aktivnih scena.
     */
    bool     is_configured;

    /**
     * @brief Bitmaska koja definiše koja svjetla su uključena u scenu.
     * @note  Svaki bit odgovara jednom svjetlu. Ako je Bit 0 postavljen (vrijednost 1),
     * to znači da je svjetlo sa indeksom 0 uključeno u ovu scenu i da će
     * njegovo stanje biti promijenjeno kada se scena aktivira.
     */
    uint8_t  lights_mask;

    /**
     * @brief Bitmaska koja definiše koje roletne su uključene u scenu.
     * @note  Funkcioniše na isti način kao `lights_mask`, ali za 16 roletni.
     * Bit 0 odgovara roletni 0, Bit 1 roletni 1, itd.
     */
    uint16_t curtains_mask;
    
    /**
     * @brief Bitmaska koja definiše koji termostati su uključeni u scenu.
     * @note  Omogućava da scena upravlja stanjem jednog ili više termostata u sistemu.
     */
    uint8_t  thermostat_mask;

    /**
     * @brief Polje koje čuva memorisana stanja (ON/OFF) za svjetla.
     * @note  Vrijednost `light_values[i]` se primjenjuje na svjetlo `i` samo
     * ako je odgovarajući bit u `lights_mask` postavljen.
     */
    uint8_t  light_values[LIGHTS_MODBUS_SIZE];
    
    /**
     * @brief Polje koje čuva memorisane vrijednosti svjetline (0-100) za dimere.
     * @note  Ovo polje se koristi za dimabilna i RGB svjetla.
     */
    uint8_t  light_brightness[LIGHTS_MODBUS_SIZE];
    
    /**
     * @brief Polje koje čuva memorisane RGB boje (u 0x00RRGGBB formatu).
     * @note  Aktiviranjem scene, RGB svjetlima se postavlja memorisana boja.
     */
    uint32_t light_colors[LIGHTS_MODBUS_SIZE];

    /**
     * @brief Polje koje čuva memorisana stanja (STOP, UP, DOWN) za roletne.
     * @note  Aktiviranjem scene, roletnama uključenim u `curtains_mask` se
     * šalje komanda za pomjeranje u memorisanom smjeru.
     */
    uint8_t  curtain_states[CURTAINS_SIZE];

    /**
     * @brief Vrijednost zadate temperature za termostate uključene u scenu.
     * @note  Svim termostatima koji su uključeni u `thermostat_mask` bit će
     * postavljena ova ista zadata temperatura.
     */
    uint8_t  thermostat_setpoint;

    /**
     * @brief Tip scene koji određuje da li se izvršava dodatna logika.
     * @note Vrijednost se postavlja u "čarobnjaku" na osnovu odabrane ikonice.
     * Npr. odabir ikonice "Mjesec" automatski postavlja tip na SCENE_TYPE_SLEEP.
     */
    SceneType_e scene_type;

    /**
     * @brief Sat (0-23) za tajmer buđenja, koristi se samo ako je tip SCENE_TYPE_SLEEP.
     * @note Vrijednost -1 označava da tajmer nije podešen za ovu scenu.
     */
    int8_t   wakeup_hour;

    /**
     * @brief Minuta (0-59) za tajmer buđenja.
     */
    uint8_t  wakeup_minute;

     /**
     * @brief Bitmaska koja određuje koje particije alarma se naoružavaju.
     * @note Koristi se samo ako je tip SCENE_TYPE_SECURITY. Vrijednost 0
     * znači da scena razoružava sistem. Kombinacijom bitova (npr.
     * SECURITY_PARTITION_1 | SECURITY_PARTITION_2) definišu se
     * parcijalna stanja.
     */
    uint8_t  security_partitions_to_arm;
    /**
     * @brief Fleg koji označava da li ova scena aktivira i scenu buđenja.
     * @note Koristi se samo ako je tip SCENE_TYPE_SLEEP.
     */
    bool activate_wakeup_scene;

    /**
     * @brief Indeks scene (0-5) koja će biti aktivirana kao scena buđenja.
     * @note Vrijednost -1 označava da nijedna scena nije odabrana.
     */
    int8_t wakeup_scene_index;

    /**
     * @brief Fleg koji označava da li treba aktivirati i zujalicu (buzzer) pri buđenju.
     */
    bool use_buzzer_alarm;
    
    /**
     * @brief Vrijeme odgode (u intervalima od 10 sekundi) za 'Odlazak' scenu.
     * @note  Vrijednost 6 znači 60 sekundi odgode prije nego se akcije izvrše.
     * Koristi se samo za `SCENE_TYPE_LEAVING`.
     */
    uint8_t  exit_delay_s;

    /**
     * @brief Fleg koji omogućava simulaciju prisustva za 'Odlazak' scenu.
     * @note  Ako je `true`, `Scene_Service` će nasumično upravljati uređajima
     * dok je sistem u `AWAY_ACTIVE` stanju.
     */
    bool     presence_simulation_enabled;
    
    /**
     * @brief Niz adresa (npr. Modbus) koje služe kao okidači za 'Povratak' scenu.
     * @note  Sistem će pratiti `DIN_EVENT` poruke i porediti adresu
     * izvora sa adresama u ovom nizu. Vrijednost 0 označava prazan slot.
     */
    uint16_t homecoming_triggers[SCENE_MAX_TRIGGERS];

} Scene_t;

/**
 ******************************************************************************
 * @brief       "Omot" struktura za snimanje svih scena u EEPROM.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura objedinjuje sve podatke vezane za scene u jedan
 * blok koji se atomarno upisuje i čita iz EEPROM-a.
 * Zahvaljujući `#pragma pack`, raspored u memoriji je kompaktan i
 * pouzdan, a `magic_number` i `crc` osiguravaju integritet
 * podataka nakon restarta ili gubitka napajanja.
 ******************************************************************************
 */
typedef struct 
{
    uint16_t magic_number;              /**< "Potpis" za validaciju podataka u EEPROM-u. Služi za detekciju da li su podaci validni ili je potrebno učitati fabričke postavke. */
    Scene_t  scenes[SCENE_MAX_COUNT];   /**< Niz koji sadrži podatke za sve scene koje sistem podržava. */
    uint16_t crc;                       /**< CRC za provjeru integriteta cijelog bloka. Računa se preko svih članova strukture prije upisa u EEPROM. */
} Scene_EepromBlock_t;

#pragma pack(pop)


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

void Scene_Init(void);
void Scene_Save(void);
void Scene_Service(void);
void Scene_Activate(uint8_t scene_index);
void Scene_Memorize(uint8_t scene_index);
Scene_t* Scene_GetInstance(uint8_t scene_index);
uint8_t Scene_GetCount(void);
void Scene_SetSystemState(SystemState_e state);
SystemState_e Scene_GetSystemState(void);

#endif // __SCENE_CTRL_H__
