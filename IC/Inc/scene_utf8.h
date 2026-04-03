/**
 ******************************************************************************
 * @file    scene.h
 * @author  Gemini & [VaÅ¡e Ime]
 * @brief   Javni API za modul koji upravlja sistemskim scenama.
 *
 * @note    Ovaj modul enkapsulira kompletnu logiku za memorisanje, aktiviranje i
 * upravljanje korisniÄki definisanim scenama. Scene omoguÄ‡avaju korisniku da
 * jednim dodirom postavi stanja viÅ¡e razliÄitih ureÄ‘aja (svjetla, roletne,
 * kapije, termostati) u unaprijed definisane pozicije.
 * Modul takoÄ‘er upravlja globalnim stanjem sistema (npr. "Away Mode").
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

/** @brief Bitmaske za definisanje koje particije alarmnog sistema su naoruÅ¾ane. */
#define SECURITY_PARTITION_1    (1 << 0)    /**< Bit za particiju 1 (npr. Prizemlje) */
#define SECURITY_PARTITION_2    (1 << 1)    /**< Bit za particiju 2 (npr. Sprat) */
#define SECURITY_PARTITION_3    (1 << 2)    /**< Bit za particiju 3 (npr. GaraÅ¾a) */
#define SECURITY_PARTITION_4    (1 << 3)    /**< Bit za particiju 4 (npr. Perimetar) */
#define SECURITY_ARM_ALL        (SECURITY_PARTITION_1 | SECURITY_PARTITION_2 | SECURITY_PARTITION_3 | SECURITY_PARTITION_4) /**< Maska za sve particije. */
/**
 * @brief DefiniÅ¡e globalna stanja sistema, primarno za "Away" logiku.
 */
typedef enum {
    SYSTEM_STATE_HOME,          /**< Normalno stanje, ukuÄ‡ani su prisutni. */
    SYSTEM_STATE_AWAY_SETTLING, /**< Stanje nakon aktivacije "Leaving" scene, traje kratko vrijeme. */
    SYSTEM_STATE_AWAY_ACTIVE    /**< Sistem je u "Away" modu, aktivna je simulacija prisustva i Äekaju se "Homecoming" okidaÄi. */
} SystemState_e;

/**
 ******************************************************************************
 * @brief       Struktura koja definiÅ¡e vizuelni izgled jedne scene.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Niz ovih struktura je definisan u `translations.h` kao
 * `scene_appearance_table[]`. SluÅ¾i kao predefinisana
 * biblioteka izgleda (ikona + naziv) koje korisnik moÅ¾e
 * dodijeliti sceni prilikom kreiranja ili editovanja u "Äarobnjaku".
 ******************************************************************************
 */
typedef struct {
    IconID icon_id;             /**< ID ikonice iz `display.h` koja predstavlja scenu. */
    TextID text_id;             /**< ID teksta iz `display.h` koji sluÅ¾i kao naziv scene. */
} SceneAppearance_t;

/**
 * @brief DefiniÅ¡e tip scene, Å¡to utiÄe na logiku koja se izvrÅ¡ava pri aktivaciji.
 */
typedef enum {
    SCENE_TYPE_STANDARD,      /**< Standardna scena, samo postavlja memorisana stanja ureÄ‘aja. */
    SCENE_TYPE_LEAVING,       /**< Specijalna scena koja aktivira SYSTEM_STATE_AWAY_ACTIVE. */
    SCENE_TYPE_HOMECOMING,    /**< Specijalna scena koja deaktivira "Away" mod. */
    SCENE_TYPE_SLEEP,         /**< Specijalna scena koja moÅ¾e imati povezan tajmer za buÄ‘enje. */
} SceneType_e;


/*============================================================================*/
/* JAVNE STRUKTURE (ZA EEPROM)                                                */
/*============================================================================*/

#pragma pack(push, 1)
/**
 ******************************************************************************
 * @brief       Glavna struktura koja Äuva kompletnu konfiguraciju i stanje JEDNE scene.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Niz ovih struktura se Äuva unutar `Scene_EepromBlock_t` omotaÄa,
 * Å¡to omoguÄ‡ava efikasno snimanje i Äitanje iz EEPROM-a.
 * SadrÅ¾i maske za ukljuÄene ureÄ‘aje i polja za njihove
 * memorisane vrijednosti.
 ******************************************************************************
 */
typedef struct
{
    /**
     * @brief Indeks u `scene_appearance_table[]` koji definiÅ¡e ikonicu i naziv scene.
     * @note  Ova vrijednost se bira u "Äarobnjaku" za scene i odreÄ‘uje vizuelni
     * prikaz scene na glavnom ekranu.
     */
    uint8_t  appearance_id;

    /**
     * @brief Fleg koji oznaÄava da li je scena konfigurisana od strane korisnika.
     * @note  Vrijednost je `false` za defaultnu, praznu scenu; postaje `true`
     * Äim je korisnik prvi put snimi. Koristi se u GUI-ju za
     * razlikovanje praznih slotova (koji prikazuju "wizard" ikonicu)
     * od aktivnih scena.
     */
    bool     is_configured;

    /**
     * @brief Bitmaska koja definiÅ¡e koja svjetla su ukljuÄena u scenu.
     * @note  Svaki bit odgovara jednom svjetlu. Ako je Bit 0 postavljen (vrijednost 1),
     * to znaÄi da je svjetlo sa indeksom 0 ukljuÄeno u ovu scenu i da Ä‡e
     * njegovo stanje biti promijenjeno kada se scena aktivira.
     */
    uint8_t  lights_mask;

    /**
     * @brief Bitmaska koja definiÅ¡e koje roletne su ukljuÄene u scenu.
     * @note  FunkcioniÅ¡e na isti naÄin kao `lights_mask`, ali za 16 roletni.
     * Bit 0 odgovara roletni 0, Bit 1 roletni 1, itd.
     */
    uint16_t curtains_mask;
    
    /**
     * @brief Bitmaska koja definiÅ¡e koji termostati su ukljuÄeni u scenu.
     * @note  OmoguÄ‡ava da scena upravlja stanjem jednog ili viÅ¡e termostata u sistemu.
     */
    uint8_t  thermostat_mask;

    /**
     * @brief Polje koje Äuva memorisana stanja (ON/OFF) za svjetla.
     * @note  Vrijednost `light_values[i]` se primjenjuje na svjetlo `i` samo
     * ako je odgovarajuÄ‡i bit u `lights_mask` postavljen.
     */
    uint8_t  light_values[LIGHTS_MODBUS_SIZE];
    
    /**
     * @brief Polje koje Äuva memorisane vrijednosti svjetline (0-100) za dimere.
     * @note  Ovo polje se koristi za dimabilna i RGB svjetla.
     */
    uint8_t  light_brightness[LIGHTS_MODBUS_SIZE];
    
    /**
     * @brief Polje koje Äuva memorisane RGB boje (u 0x00RRGGBB formatu).
     * @note  Aktiviranjem scene, RGB svjetlima se postavlja memorisana boja.
     */
    uint32_t light_colors[LIGHTS_MODBUS_SIZE];

    /**
     * @brief Polje koje Äuva memorisana stanja (STOP, UP, DOWN) za roletne.
     * @note  Aktiviranjem scene, roletnama ukljuÄenim u `curtains_mask` se
     * Å¡alje komanda za pomjeranje u memorisanom smjeru.
     */
    uint8_t  curtain_states[CURTAINS_SIZE];

    /**
     * @brief Vrijednost zadate temperature za termostate ukljuÄene u scenu.
     * @note  Svim termostatima koji su ukljuÄeni u `thermostat_mask` bit Ä‡e
     * postavljena ova ista zadata temperatura.
     */
    uint8_t  thermostat_setpoint;

    /**
     * @brief Tip scene koji odreÄ‘uje da li se izvrÅ¡ava dodatna logika.
     * @note Vrijednost se postavlja u "Äarobnjaku" na osnovu odabrane ikonice.
     * Npr. odabir ikonice "Mjesec" automatski postavlja tip na SCENE_TYPE_SLEEP.
     */
    SceneType_e scene_type;

    /**
     * @brief Sat (0-23) za tajmer buÄ‘enja, koristi se samo ako je tip SCENE_TYPE_SLEEP.
     * @note Vrijednost -1 oznaÄava da tajmer nije podeÅ¡en za ovu scenu.
     */
    int8_t   wakeup_hour;

    /**
     * @brief Minuta (0-59) za tajmer buÄ‘enja.
     */
    uint8_t  wakeup_minute;

     /**
     * @brief Bitmaska koja odreÄ‘uje koje particije alarma se naoruÅ¾avaju.
     * @note Koristi se samo ako je tip SCENE_TYPE_SECURITY. Vrijednost 0
     * znaÄi da scena razoruÅ¾ava sistem. Kombinacijom bitova (npr.
     * SECURITY_PARTITION_1 | SECURITY_PARTITION_2) definiÅ¡u se
     * parcijalna stanja.
     */
    uint8_t  security_partitions_to_arm;
    /**
     * @brief Fleg koji oznaÄava da li ova scena aktivira i scenu buÄ‘enja.
     * @note Koristi se samo ako je tip SCENE_TYPE_SLEEP.
     */
    bool activate_wakeup_scene;

    /**
     * @brief Indeks scene (0-5) koja Ä‡e biti aktivirana kao scena buÄ‘enja.
     * @note Vrijednost -1 oznaÄava da nijedna scena nije odabrana.
     */
    int8_t wakeup_scene_index;

    /**
     * @brief Fleg koji oznaÄava da li treba aktivirati i zujalicu (buzzer) pri buÄ‘enju.
     */
    bool use_buzzer_alarm;
    
    /**
     * @brief Vrijeme odgode (u intervalima od 10 sekundi) za 'Odlazak' scenu.
     * @note  Vrijednost 6 znaÄi 60 sekundi odgode prije nego se akcije izvrÅ¡e.
     * Koristi se samo za `SCENE_TYPE_LEAVING`.
     */
    uint8_t  exit_delay_s;

    /**
     * @brief Fleg koji omoguÄ‡ava simulaciju prisustva za 'Odlazak' scenu.
     * @note  Ako je `true`, `Scene_Service` Ä‡e nasumiÄno upravljati ureÄ‘ajima
     * dok je sistem u `AWAY_ACTIVE` stanju.
     */
    bool     presence_simulation_enabled;
    
    /**
     * @brief Niz adresa (npr. Modbus) koje sluÅ¾e kao okidaÄi za 'Povratak' scenu.
     * @note  Sistem Ä‡e pratiti `DIN_EVENT` poruke i porediti adresu
     * izvora sa adresama u ovom nizu. Vrijednost 0 oznaÄava prazan slot.
     */
    uint16_t homecoming_triggers[SCENE_MAX_TRIGGERS];

} Scene_t;

/**
 ******************************************************************************
 * @brief       "Omot" struktura za snimanje svih scena u EEPROM.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura objedinjuje sve podatke vezane za scene u jedan
 * blok koji se atomarno upisuje i Äita iz EEPROM-a.
 * ZahvaljujuÄ‡i `#pragma pack`, raspored u memoriji je kompaktan i
 * pouzdan, a `magic_number` i `crc` osiguravaju integritet
 * podataka nakon restarta ili gubitka napajanja.
 ******************************************************************************
 */
typedef struct 
{
    uint16_t magic_number;              /**< "Potpis" za validaciju podataka u EEPROM-u. SluÅ¾i za detekciju da li su podaci validni ili je potrebno uÄitati fabriÄke postavke. */
    Scene_t  scenes[SCENE_MAX_COUNT];   /**< Niz koji sadrÅ¾i podatke za sve scene koje sistem podrÅ¾ava. */
    uint16_t crc;                       /**< CRC za provjeru integriteta cijelog bloka. RaÄuna se preko svih Älanova strukture prije upisa u EEPROM. */
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

