/**
 ******************************************************************************
 * @file    scene.c
 * @author  Gemini & [VaÅ¡e Ime]
 * @brief   Implementacija modula za upravljanje sistemskim scenama.
 *
 * @note
 * Ovaj fajl sadrÅ¾i kompletnu pozadinsku (backend) logiku za sistem scena.
 * Odgovoran je za Äuvanje i Äitanje konfiguracija scena iz EEPROM-a,
 * aktiviranje scena (slanje komandi drugim modulima), memorisanje trenutnog
 * stanja sistema u scenu, kao i za upravljanje globalnim stanjem sistema
 * (npr. "Away Mode").
 ******************************************************************************
 */

#if (__SCENE_CTRL_H__ != FW_BUILD)
#error "scene header version mismatch"
#endif

/*============================================================================*/
/* UKLJUÄŒENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "scene.h"
#include "display.h"
#include "lights.h"
#include "curtain.h"
#include "thermostat.h"
#include "gate.h"
#include "rs485.h"
#include "stm32746g_eeprom.h"

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI (INTERNI)                                     */
/*============================================================================*/

/**
 * @brief Puna veliÄina bloka podataka za scene koji se Äuva u EEPROM-u.
 * @note  UkljuÄuje magiÄni broj, niz scena i CRC.
 */
#define EE_SCENES_BLOCK_SIZE (sizeof(uint16_t) + sizeof(scenes) + sizeof(uint16_t))


/*============================================================================*/
/* PRIVATNE (STATIÄŒKE) VARIJABLE                                              */
/*============================================================================*/

/**
 * @brief StatiÄki niz koji u RAM-u Äuva konfiguraciju i stanje za sve scene.
 * @note  Ovo je "jedinstveni izvor istine" za stanje scena tokom rada ureÄ‘aja.
 * Inicijalizuje se iz EEPROM-a prilikom pokretanja sistema.
 */
static Scene_t scenes[SCENE_MAX_COUNT];

/**
 * @brief StatiÄka varijabla koja Äuva trenutno globalno stanje sistema.
 * @note  Koristi se za implementaciju "Away" i "Homecoming" logike.
 */
static SystemState_e system_state = SYSTEM_STATE_HOME;

/**
 * @brief Definicija runtime podataka za jednu scenu.
 * @note  Ovi podaci se ne Äuvaju u EEPROM-u i koriste se za praÄ‡enje
 * dinamiÄkih stanja, kao Å¡to su aktivni tajmeri.
 */
typedef struct
{
    uint8_t  runtime_state; // Npr. STATE_IDLE, STATE_LEAVING_DELAY
    uint32_t timer_start;   // Vrijeme poÄetka tajmera (HAL_GetTick())
} Scene_Runtime_t;

/**
 * @brief Stanja za runtime maÅ¡inu stanja jedne scene.
 */
enum {
    SCENE_RUNTIME_STATE_IDLE,
    SCENE_RUNTIME_STATE_LEAVING_DELAY
};

/**
 * @brief StatiÄki niz koji Äuva runtime podatke za sve scene, paralelno sa `scenes` nizom.
 */
static Scene_Runtime_t scene_runtime_data[SCENE_MAX_COUNT];
/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOCNIH FUNKCIJA                                    */
/*============================================================================*/
static void Scene_SetDefault(void);
static void Scene_ExecuteComfortActions(uint8_t scene_index);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/
/**
 ******************************************************************************
 * @brief       Glavna servisna petlja za modul scena.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se poziva periodiÄno iz `main.c`. Njena uloga je da
 * izvrÅ¡ava dugotrajne ili periodiÄne zadatke. Upravlja tajmerom za
 * odgodu "Odlazak" scene i logikom za simulaciju prisustva kada je
 * sistem u "Away" modu.
 ******************************************************************************
 */
void Scene_Service(void)
{
    // Prolazimo kroz sve scene da provjerimo da li neka ima aktivan tajmer
    for (uint8_t i = 0; i < SCENE_MAX_COUNT; i++)
    {
        // Provjeravamo runtime stanje svake scene
        switch (scene_runtime_data[i].runtime_state)
        {
            case SCENE_RUNTIME_STATE_LEAVING_DELAY:
            {
                Scene_t* target_scene = &scenes[i];
                uint32_t delay_ms = (uint32_t)target_scene->exit_delay_s * 10000UL; // Vrijednost iz menija (npr. 6) * 10s

                // Provjera da li je vrijeme odgode isteklo
                if ((HAL_GetTick() - scene_runtime_data[i].timer_start) >= delay_ms)
                {
                    // Vrijeme je isteklo, izvrÅ¡i "comfort" akcije (ugasi svjetla, itd.)
                    Scene_ExecuteComfortActions(i);
                    
                    // PoÅ¡alji broadcast poruku da je pokrenut "Odlazak" dogaÄ‘aj
                    // TODO: Pozvati AddCommand za slanje SCENE_CONTROL poruke tipa SCENE_TYPE_LEAVING

                    // Postavi globalno stanje sistema na "Away"
                    Scene_SetSystemState(SYSTEM_STATE_AWAY_ACTIVE);
                    
                    // Vrati runtime stanje scene na IDLE
                    scene_runtime_data[i].runtime_state = SCENE_RUNTIME_STATE_IDLE;
                    scene_runtime_data[i].timer_start = 0;
                }
                break;
            }

            case SCENE_RUNTIME_STATE_IDLE:
            default:
                // Scena je neaktivna, ne radi niÅ¡ta.
                break;
        }
    }

    // --- Logika za Simulaciju Prisustva ---
    if (Scene_GetSystemState() == SYSTEM_STATE_AWAY_ACTIVE)
    {
        // TODO: Implementirati logiku za simulaciju prisustva.
        // Ovdje Ä‡e se nalaziti tajmeri i logika koja Ä‡e periodiÄno pozivati
        // LIGHT_Flip() ili Curtain_Move() za nasumiÄno odabrane ureÄ‘aje
        // iz maske aktivne "Odlazak" scene.
    }
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje modul za scene pri pokretanju sistema.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        UÄitava kompletan blok podataka za sve scene iz EEPROM-a. VrÅ¡i
 * provjeru validnosti podataka pomoÄ‡u magiÄnog broja i CRC-a. Ako
 * podaci nisu validni (npr. prvo pokretanje ili oÅ¡teÄ‡eni podaci),
 * poziva `Scene_SetDefault()` da kreira jednu, defaultnu,
 * nekonfigurisanu scenu i odmah je snima u EEPROM.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Init(void)
{
    // KORISTIMO DIREKTNO DEFINISANI TIP IZ scene.h
    Scene_EepromBlock_t eeprom_block;

    // UÄitaj cijeli blok iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&eeprom_block, EE_SCENES, sizeof(Scene_EepromBlock_t));

    // Provjeri validnost podataka
    if (eeprom_block.magic_number != EEPROM_MAGIC_NUMBER)
    {
        // Podaci nisu validni (npr. prvo pokretanje), postavi defaultne vrijednosti
        Scene_SetDefault();
        Scene_Save(); // Odmah snimi ispravne defaultne vrijednosti u EEPROM
    }
    else
    {
        uint16_t received_crc = eeprom_block.crc;
        eeprom_block.crc = 0;
        // Koristimo sizeof(Scene_EepromBlock_t) za taÄan proraÄun
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&eeprom_block, sizeof(Scene_EepromBlock_t));

        if (received_crc != calculated_crc)
        {
            // Podaci su oÅ¡teÄ‡eni (CRC se ne poklapa), postavi defaultne vrijednosti
            Scene_SetDefault();
            Scene_Save();
        }
        else
        {
            // Podaci su validni, prekopiraj ih u radnu memoriju (RAM)
            memcpy(scenes, eeprom_block.scenes, sizeof(scenes));
        }
    }
}

/**
 ******************************************************************************
 * @brief       Snima trenutno stanje svih scena iz RAM-a u EEPROM.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se poziva nakon svake promjene u konfiguraciji
 * scena (npr. nakon poziva `Scene_Memorize`). Ona priprema kompletan
 * memorijski blok tako Å¡to u lokalnu `Scene_EepromBlock_t` strukturu
 * upiÅ¡e magiÄni broj, prekopira podatke iz `scenes` niza, izraÄuna
 * CRC32 nad cijelim blokom i na kraju ga atomarno upiÅ¡e u EEPROM.
 * Ovaj proces osigurava integritet i validnost podataka.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Save(void)
{
    // Kreiraj lokalnu instancu strukture koja odgovara EEPROM bloku.
    Scene_EepromBlock_t block_to_save;

    // KORAK 1: Postavi "potpis" (magiÄni broj) za validaciju.
    block_to_save.magic_number = EEPROM_MAGIC_NUMBER;

    // KORAK 2: Prekopiraj trenutno stanje svih scena iz RAM-a u strukturu za snimanje.
    memcpy(block_to_save.scenes, scenes, sizeof(scenes));

    // KORAK 3: Pripremi za izraÄunavanje CRC-a tako Å¡to se CRC polje postavi na 0.
    block_to_save.crc = 0;

    // KORAK 4: IzraÄunaj CRC nad cijelim blokom (magiÄni broj + podaci o scenama).
    block_to_save.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&block_to_save, sizeof(Scene_EepromBlock_t));

    // KORAK 5: Snimi kompletan, pripremljen blok u EEPROM u jednoj operaciji.
    EE_WriteBuffer((uint8_t*)&block_to_save, EE_SCENES, sizeof(Scene_EepromBlock_t));
}
/**
 ******************************************************************************
 * @brief       Aktivira odabranu scenu primjenjujuÄ‡i memorisana stanja na ureÄ‘aje.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Verzija 2.1: ProÅ¡ireno da podrÅ¾ava asinhrono izvrÅ¡avanje za scene
 * sa odgodom (npr. "Odlazak"). Za takve scene, ova funkcija samo
 * pokreÄ‡e maÅ¡inu stanja, a `Scene_Service` izvrÅ¡ava stvarne akcije
 * nakon isteka tajmera. Za ostale scene, akcija je trenutna.
 ******************************************************************************
 */
void Scene_Activate(uint8_t scene_index)
{
    // Sigurnosna provjera da se ne Äita izvan granica niza
    if (scene_index >= SCENE_MAX_COUNT)
    {
        return;
    }

    // Dobijamo "handle" na scenu koju Å¾elimo aktivirati
    Scene_t* target_scene = &scenes[scene_index];

    // Ne radimo niÅ¡ta ako scena nije prethodno konfigurisana
    if (!target_scene->is_configured)
    {
        return;
    }

    // --- IzvrÅ¡avanje specijalne logike na osnovu tipa scene ---
    switch (target_scene->scene_type)
    {
        case SCENE_TYPE_LEAVING:
            // Za scenu "Odlazak", ne izvrÅ¡avamo akcije odmah.
            // Samo pokreÄ‡emo maÅ¡inu stanja i tajmer za odgodu.
            scene_runtime_data[scene_index].runtime_state = SCENE_RUNTIME_STATE_LEAVING_DELAY;
            scene_runtime_data[scene_index].timer_start = HAL_GetTick();
            // Stvarne akcije Ä‡e izvrÅ¡iti `Scene_Service` nakon isteka vremena.
            return; // VaÅ¾no: Prekidamo izvrÅ¡avanje ovdje!

        case SCENE_TYPE_HOMECOMING:
            // Prvo poÅ¡alji broadcast poruku da se sistem vraÄ‡a u HOME mod.
            // TODO: Pozvati AddCommand za slanje SCENE_CONTROL poruke tipa SCENE_TYPE_HOMECOMING
            Scene_SetSystemState(SYSTEM_STATE_HOME);
            break;

        case SCENE_TYPE_SLEEP:
            if (target_scene->wakeup_hour != -1)
            {
                // TODO: Pozvati buduÄ‡u funkciju iz Timer modula za postavljanje alarma
            }
            // Integracija sa alarmom:
            if (target_scene->security_partitions_to_arm > 0)
            {
                // TODO: Pozvati buduÄ‡u funkciju iz Security modula za naoruÅ¾avanje particija
                // Npr. Security_ArmPartitions(target_scene->security_partitions_to_arm);
            }
            break;

        case SCENE_TYPE_STANDARD:
        default:
            // Za standardne scene, akcije se izvrÅ¡avaju odmah.
            break;
    }

    // IzvrÅ¡avanje "comfort" akcija za sve scene koje nisu prekinute (sve osim LEAVING)
    Scene_ExecuteComfortActions(scene_index);
}
/**
 ******************************************************************************
 * @brief       IzvrÅ¡ava "comfort" akcije za datu scenu.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova pomoÄ‡na funkcija sadrÅ¾i logiku za postavljanje stanja
 * svjetala, roletni i termostata na osnovu memorisanih vrijednosti
 * u strukturi scene. Kreirana je da bi se izbjeglo dupliranje koda
 * izmeÄ‘u `Scene_Activate` i `Scene_Service` funkcija.
 * @param       scene_index Indeks scene (0-5) Äije akcije treba izvrÅ¡iti.
 ******************************************************************************
 */
static void Scene_ExecuteComfortActions(uint8_t scene_index)
{
    if (scene_index >= SCENE_MAX_COUNT) return;
    Scene_t* target_scene = &scenes[scene_index];

    // Postavljanje stanja za SVJETLA
    for (uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        if (target_scene->lights_mask & (1 << i))
        {
            LIGHT_Handle* light_handle = LIGHTS_GetInstance(i);
            if (light_handle)
            {
                LIGHT_SetState(light_handle, target_scene->light_values[i]);
                LIGHT_SetBrightness(light_handle, target_scene->light_brightness[i]);
                LIGHT_SetColor(light_handle, target_scene->light_colors[i]);
            }
        }
    }

    // Postavljanje stanja za ROLETNE
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if (target_scene->curtains_mask & (1 << i))
        {
            Curtain_Handle* curtain_handle = Curtain_GetInstanceByIndex(i);
            if (curtain_handle)
            {
                Curtain_Move(curtain_handle, target_scene->curtain_states[i]);
            }
        }
    }

    // Postavljanje stanja za TERMOSTAT
    if (target_scene->thermostat_mask)
    {
        THERMOSTAT_TypeDef* thst_handle = Thermostat_GetInstance();
        if (thst_handle)
        {
            Thermostat_SP_Temp_Set(thst_handle, target_scene->thermostat_setpoint);
        }
    }
}
/**
 ******************************************************************************
 * @brief       MemoriÅ¡e trenutno stanje svih relevantnih ureÄ‘aja u odabranu scenu.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija je srce "Äarobnjaka" za kreiranje scena. Ona iterira
 * kroz sve konfigurisane ureÄ‘aje (svjetla, roletne, termostat),
 * poziva njihove javne API funkcije (gettere) da bi prikupila
 * njihovo trenutno stanje, i te vrijednosti upisuje u strukturu
 * odabrane scene u RAM-u. TakoÄ‘er postavlja 'is_configured' fleg
 * na 'true', signalizirajuÄ‡i da scena viÅ¡e nije prazna.
 * @param       scene_index Indeks scene (0-5) u koju treba memorisati stanje.
 * @retval      None
 ******************************************************************************
 */
void Scene_Memorize(uint8_t scene_index)
{
    // Sigurnosna provjera da se ne piÅ¡e izvan granica niza
    if (scene_index >= SCENE_MAX_COUNT)
    {
        return;
    }

    // Dobijamo "handle" na scenu koju Å¾elimo modifikovati
    Scene_t* target_scene = &scenes[scene_index];

    // Resetujemo maske prije popunjavanja da osiguramo Äisto stanje
    target_scene->lights_mask = 0;
    target_scene->curtains_mask = 0;
    target_scene->thermostat_mask = 0;

    // --- Memorisanje Stanja Svjetala ---
    for (uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        LIGHT_Handle* light_handle = LIGHTS_GetInstance(i);
        if (light_handle && LIGHT_GetRelay(light_handle) != 0) // Provjera da li je svjetlo konfigurisano
        {
            // Postavi odgovarajuÄ‡i bit u maski
            target_scene->lights_mask |= (1 << i);
            
            // SaÄuvaj trenutne vrijednosti koristeÄ‡i API funkcije
            target_scene->light_values[i] = LIGHT_isActive(light_handle);
            target_scene->light_brightness[i] = LIGHT_GetBrightness(light_handle);
            target_scene->light_colors[i] = LIGHT_GetColor(light_handle);
        }
    }

    // --- Memorisanje Stanja Roletni ---
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain_Handle* curtain_handle = Curtain_GetInstanceByIndex(i);
        if (curtain_handle && Curtain_hasRelays(curtain_handle)) // Provjera da li je roletna konfigurisana
        {
            // Postavi odgovarajuÄ‡i bit u maski
            target_scene->curtains_mask |= (1 << i);
            
            // SaÄuvaj trenutno stanje (STOP, UP, ili DOWN)
            target_scene->curtain_states[i] = Curtain_getNewDirection(curtain_handle);
        }
    }

    // --- Memorisanje Stanja Termostata ---
    THERMOSTAT_TypeDef* thst_handle = Thermostat_GetInstance();
    if (thst_handle)
    {
        // Za sada, pretpostavljamo da scena uvijek utiÄe na termostat ako je prisutan
        target_scene->thermostat_mask = 1; 
        target_scene->thermostat_setpoint = Thermostat_GetSetpoint(thst_handle);
    }
    
    // KljuÄni korak: OznaÄi scenu kao konfigurisanu
    target_scene->is_configured = true;
}

/**
 ******************************************************************************
 * @brief       VraÄ‡a pokazivaÄ na instancu odabrane scene u RAM-u.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        OmoguÄ‡ava sigurnan, "read-only" pristup podacima scene iz
 * drugih modula, npr. iz `display.c` za potrebe iscrtavanja.
 * @param       scene_index Indeks scene (0-5).
 * @retval      Scene_t* PokazivaÄ na traÅ¾enu scenu, ili NULL ako je
 * indeks neispravan.
 ******************************************************************************
 */
Scene_t* Scene_GetInstance(uint8_t scene_index)
{
    if (scene_index < SCENE_MAX_COUNT)
    {
        return &scenes[scene_index];
    }
    return NULL;
}

/**
 ******************************************************************************
 * @brief       VraÄ‡a broj trenutno konfigurisanih scena.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija prolazi kroz niz svih moguÄ‡ih scena i broji koliko
 * njih ima postavljen `is_configured` fleg na `true`. Scene koje
 * korisnik joÅ¡ nije memorisao (prazni slotovi) se ne ubrajaju u
 * rezultat. KljuÄna je za dinamiÄko iscrtavanje GUI-ja.
 * @param       None
 * @retval      uint8_t Broj aktivnih, korisniÄki konfigurisanih scena (0 do 6).
 ******************************************************************************
 */
uint8_t Scene_GetCount(void)
{
    // Inicijalizuj brojaÄ na nulu
    uint8_t configured_count = 0;

    // ProÄ‘i kroz sve slotove za scene
    for (uint8_t i = 0; i < SCENE_MAX_COUNT; i++)
    {
        // Ako je fleg 'is_configured' postavljen na true, uveÄ‡aj brojaÄ
        if (scenes[i].is_configured)
        {
            configured_count++;
        }
    }

    // Vrati ukupan broj pronaÄ‘enih konfigurisanih scena
    return configured_count;
}

/**
 ******************************************************************************
 * @brief       Postavlja novo globalno stanje sistema.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se koristi za upravljanje "Away" modom i drugim
 * globalnim stanjima.
 * @param       state Novo stanje iz `SystemState_e` enumeracije.
 * @retval      None
 ******************************************************************************
 */
void Scene_SetSystemState(SystemState_e state)
{
    system_state = state;
    // TODO: Dodati logiku koja se izvrÅ¡ava pri promjeni stanja
}

/**
 ******************************************************************************
 * @brief       VraÄ‡a trenutno globalno stanje sistema.
 * @author      Gemini & [VaÅ¡e Ime]
 * @param       None
 * @retval      SystemState_e Trenutno stanje sistema.
 ******************************************************************************
 */
SystemState_e Scene_GetSystemState(void)
{
    return system_state;
}


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 * @brief  Postavlja scene na poÄetno, fabriÄko stanje.
 * @note   Poziva se iz `Scene_Init()` kada podaci u EEPROM-u nisu validni.
 * BriÅ¡e sve postojeÄ‡e podatke i kreira jednu, defaultnu scenu koja
 * je oznaÄena kao nekonfigurisana (`is_configured = false`).
 * @param  None
 * @retval None
 */
static void Scene_SetDefault(void)
{
    // Prvo obriÅ¡i kompletan niz scena u RAM-u
    memset(scenes, 0, sizeof(scenes));

    // KonfiguriÅ¡i prvu scenu kao defaultnu
    scenes[0].appearance_id = 0; // Pretpostavka da je ID=0 za "Wizzard" ili "Add" ikonicu
    scenes[0].is_configured = false; // KljuÄni fleg za UI logiku
}

