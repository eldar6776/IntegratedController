/**
 ******************************************************************************
 * @file    gate.c
 * @author  Gemini & [VaÅ¡e Ime]
 * @brief   Implementacija modula za upravljanje kapijama, bravama i rampama.
 *
 * @note    Ovaj fajl sadrÅ¾i kompletnu pozadinsku (backend) logiku.
 * Odgovoran je za Äuvanje i Äitanje konfiguracija iz EEPROM-a,
 * upravljanje stanjima kapija, izvrÅ¡avanje komandi i obradu signala
 * sa eksternih senzora, sve voÄ‘eno metapodacima iz Biblioteke
 * Profila Kontrole.
 ******************************************************************************
 */

#if (__GATE_CTRL_H__ != FW_BUILD)
#error "gate header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "gate.h"
#include "display.h"
#include "rs485.h"
#include "stm32746g_eeprom.h"

/*============================================================================*/
/* DEFINICIJA INTERNE "RUNTIME" STRUKTURE                                     */
/*============================================================================*/
/**
 ******************************************************************************
 * @brief  Puna definicija "runtime" strukture za jednu kapiju.
 * @note   Ovo je konkretna implementacija `Gate_Handle` opaque tipa. SadrÅ¾i
 * konfiguraciju koja se Äuva u EEPROM-u (`config`) i dodatne
 * runtime varijable koje se koriste za praÄ‡enje stanja i tajmera.
 ******************************************************************************
 */
struct Gate_s
{
    /** @brief Konfiguracioni podaci koji se Äuvaju u EEPROM-u. */
    Gate_EepromConfig_t config;

    // --- Runtime podaci ---
    /** @brief Trenutno stanje kapije (Otvorena, Zatvorena, KreÄ‡e se...). */
    GateState_e     current_state;
    /** @brief Tip tajmera koji je trenutno aktivan. */
    GateTimerType_e active_timer_type;
    /** @brief Vrijeme (HAL_GetTick()) kada je posljednji tajmer pokrenut. */
    uint32_t        timer_start_tick;
    /** @brief Index releja (1-4) za koji je vezan aktivni pulsni tajmer. */
    uint8_t         pulse_relay_index;
};


/*============================================================================*/
/* PRIVATNE (STATIÄŒKE) VARIJABLE                                              */
/*============================================================================*/

/**
 * @brief Glavni niz koji u RAM-u Äuva kompletnu konfiguraciju i runtime stanje za sve kapije.
 * @note  Ovaj niz je statiÄan i vidljiv samo unutar ovog fajla.
 */
static struct Gate_s gates[GATE_MAX_COUNT];

/**
 ******************************************************************************
 * @brief       Biblioteka Profila Kontrole.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ovo je centralna, konstantna baza podataka koja definiÅ¡e ponaÅ¡anje
 * za svaki podrÅ¾ani tip ureÄ‘aja. Univerzalna State MaÅ¡ina koristi
 * ove metapodatke za donoÅ¡enje odluka. Dodavanje novih tipova motora
 * zahtijeva samo dodavanje novog unosa u ovaj niz.
 ******************************************************************************
 */
static const ProfilDeskriptor_t g_ControlProfileLibrary[] = {
    [CONTROL_TYPE_NONE] = {
        .profile_id = CONTROL_TYPE_NONE,
        .profile_name = "Nije KoriÅ¡teno",
        .visible_settings_mask = 0,
        .command_map = {0},
    },
    
    [CONTROL_TYPE_BFT_STEP_BY_STEP] = {
        .profile_id = CONTROL_TYPE_BFT_STEP_BY_STEP,
        .profile_name = "BFT S-S",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 | SETTING_VISIBLE_RELAY_CMD2 |
                                 SETTING_VISIBLE_FEEDBACK_1 | SETTING_VISIBLE_FEEDBACK_2 |
                                 SETTING_VISIBLE_CYCLE_TIMER | SETTING_VISIBLE_PULSE_TIMER,
        .command_map = {
            [UI_COMMAND_SMART_STEP] = { .target_relay_index = 1, .is_pulse = true },
            [UI_COMMAND_PEDESTRIAN] = { .target_relay_index = 2, .is_pulse = true },
        }
    },
    
    [CONTROL_TYPE_NICE_SLIDING_PULSE] = {
        .profile_id = CONTROL_TYPE_NICE_SLIDING_PULSE,
        .profile_name = "NICE Klizna-Puls",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 | SETTING_VISIBLE_RELAY_CMD2 |
                                 SETTING_VISIBLE_RELAY_CMD3 | SETTING_VISIBLE_RELAY_CMD4 |
                                 SETTING_VISIBLE_FEEDBACK_1 | SETTING_VISIBLE_FEEDBACK_2 |
                                 SETTING_VISIBLE_CYCLE_TIMER | SETTING_VISIBLE_PULSE_TIMER,
        .command_map = {
            [UI_COMMAND_OPEN_CYCLE]  = { .target_relay_index = 1, .is_pulse = true },
            [UI_COMMAND_CLOSE_CYCLE] = { .target_relay_index = 2, .is_pulse = true },
            [UI_COMMAND_PEDESTRIAN]  = { .target_relay_index = 3, .is_pulse = true },
            [UI_COMMAND_STOP]        = { .target_relay_index = 4, .is_pulse = true },
        }
    },
    
    [CONTROL_TYPE_SIMPLE_LOCK] = {
        .profile_id = CONTROL_TYPE_SIMPLE_LOCK,
        .profile_name = "Pametna Brava",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 | SETTING_VISIBLE_FEEDBACK_1 |
                                 SETTING_VISIBLE_CYCLE_TIMER | SETTING_VISIBLE_PULSE_TIMER,
        .command_map = {
            [UI_COMMAND_UNLOCK] = { .target_relay_index = 1, .is_pulse = true },
        }
    },

    [CONTROL_TYPE_GENERIC_MAINTAINED] = {
        .profile_id = CONTROL_TYPE_GENERIC_MAINTAINED,
        .profile_name = "Kontinuirani Signal",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 | SETTING_VISIBLE_RELAY_CMD2 |
                                 SETTING_VISIBLE_FEEDBACK_1 | SETTING_VISIBLE_FEEDBACK_2 |
                                 SETTING_VISIBLE_CYCLE_TIMER, // Pulsni tajmer nije potreban
        .command_map = {
            [UI_COMMAND_OPEN_CYCLE]  = { .target_relay_index = 1, .is_pulse = false }, // Nije puls
            [UI_COMMAND_CLOSE_CYCLE] = { .target_relay_index = 2, .is_pulse = false }, // Nije puls
            [UI_COMMAND_STOP]        = { .target_relay_index = 0, .is_pulse = false }, // Stop se rjeÅ¡ava gaÅ¡enjem oba releja
        }
    },
    
    [CONTROL_TYPE_RAMP_PULSE] = {
        .profile_id = CONTROL_TYPE_RAMP_PULSE,
        .profile_name = "Rampa (Puls)",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 | SETTING_VISIBLE_RELAY_CMD2 |
                                 SETTING_VISIBLE_FEEDBACK_1 | SETTING_VISIBLE_FEEDBACK_2 |
                                 SETTING_VISIBLE_CYCLE_TIMER | SETTING_VISIBLE_PULSE_TIMER,
        .command_map = {
            [UI_COMMAND_OPEN_CYCLE]  = { .target_relay_index = 1, .is_pulse = true },
            [UI_COMMAND_CLOSE_CYCLE] = { .target_relay_index = 2, .is_pulse = true },
        }
    },
    
    [CONTROL_TYPE_SIMPLE_STEP_BY_STEP] = {
        .profile_id = CONTROL_TYPE_SIMPLE_STEP_BY_STEP,
        .profile_name = "Jednostavni S-S",
        .visible_settings_mask = SETTING_VISIBLE_RELAY_CMD1 |
                                 SETTING_VISIBLE_FEEDBACK_1 | SETTING_VISIBLE_FEEDBACK_2 |
                                 SETTING_VISIBLE_CYCLE_TIMER | SETTING_VISIBLE_PULSE_TIMER,
        .command_map = {
            [UI_COMMAND_SMART_STEP] = { .target_relay_index = 1, .is_pulse = true },
        }
    },
};


/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOÄ†NIH FUNKCIJA                                    */
/*============================================================================*/
static void Gate_SendAction(Gate_Handle* handle, UI_Command_e command);
static void Gate_SendRawCommand(Gate_Handle* handle, uint8_t relay_index, bool is_pulse);
static void Gate_StopAllRelays(Gate_Handle* const handle);
static Gate_Handle* Gate_FindByFeedbackSensor(uint16_t sensor_addr);
static void Gate_SetDefault(Gate_Handle* const handle);
static void Gate_Init_Single(uint8_t index);
static void Gate_Save_Single(uint8_t index);
static uint16_t Gate_GetRelayAddressByIndex(const Gate_Handle* handle, uint8_t index);
static void HandleSensorEvent(Gate_Handle* handle, uint16_t sensor_addr, uint8_t state);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Inicijalizuje kompletan modul za kapije.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se poziva jednom pri startu sistema iz `main()`.
 * Iterira kroz sve podrÅ¾ane kapije (definisane sa `GATE_MAX_COUNT`)
 * i za svaku poziva internu `Gate_Init_Single()` funkciju koja
 * uÄitava i validira konfiguraciju iz EEPROM-a.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Init(void)
{
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        Gate_Init_Single(i);
    }
}

/**
 ******************************************************************************
 * @brief       Snima konfiguraciju svih kapija u EEPROM.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija se poziva iz UI-ja nakon Å¡to korisnik potvrdi
 * izmjene u meniju za podeÅ¡avanja. Iterira kroz sve instance
 * i poziva `Gate_Save_Single` za svaku.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Save(void)
{
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        Gate_Save_Single(i);
    }
}

/**
 ******************************************************************************
 * @brief       Glavna servisna petlja za modul kapija (Univerzalna State MaÅ¡ina).
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Poziva se periodiÄno iz `main()`. Prolazi kroz sve
 * konfigurisane ureÄ‘aje i upravlja njihovim stanjem na osnovu
 * aktivnih tajmera. Odgovorna je za gaÅ¡enje pulsnih releja,
 * detekciju timeout-a ciklusa i upravljanje pjeÅ¡aÄkim modom.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Service(void)
{
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        Gate_Handle* handle = &gates[i];
        if (handle->config.control_type == CONTROL_TYPE_NONE || handle->active_timer_type == GATE_TIMER_NONE) {
            continue;
        }
        uint32_t now = HAL_GetTick();
        uint32_t elapsed_ms = now - handle->timer_start_tick;
        switch (handle->active_timer_type)
        {
            case GATE_TIMER_PULSE:
            {
                uint16_t pulse_duration_ms = handle->config.pulse_timer_ms;
                if (pulse_duration_ms == 0) pulse_duration_ms = 500;

                if (elapsed_ms >= pulse_duration_ms) {
                    // 1. UVIJEK ugasi relej nakon pulsa
                    uint16_t relay_addr = Gate_GetRelayAddressByIndex(handle, handle->pulse_relay_index);
                    uint8_t buff[3] = { (relay_addr >> 8) & 0xFF, relay_addr & 0xFF, BINARY_OFF };
                    AddCommand(&binaryQueue, BINARY_SET, buff, 3);
                    handle->pulse_relay_index = 0;

                    // 2. KONAÄŒNA LOGIKA ZA BRAVU: VRAÄ†ANJE NA ZATVORENO NAKON PULSA
                    //    Ovaj blok ima najviÅ¡i prioritet, reÅ¡avajuÄ‡i vizuelni povratak.
                    if (handle->config.control_type == CONTROL_TYPE_SIMPLE_LOCK)
                    {
                        handle->current_state = GATE_STATE_CLOSED;
                        handle->active_timer_type = GATE_TIMER_NONE;
                        handle->timer_start_tick = 0;
                        shouldDrawScreen = 1;
                    }
                    // 3. LOGIKA ZA RAMPU/KAPIJE (nastavi sa CYCLE tajmerom)
                    else if (handle->current_state == GATE_STATE_OPENING || handle->current_state == GATE_STATE_CLOSING) 
                    {
                        handle->active_timer_type = GATE_TIMER_CYCLE; 
                    }
                    // 4. ZA SVE OSTALE SLUÄŒAJEVE, ugasi tajmer
                    else
                    {
                        handle->active_timer_type = GATE_TIMER_NONE;
                        handle->timer_start_tick = 0;
                    }
                }
                break;
            }
            case GATE_TIMER_PEDESTRIAN:
            {
                uint32_t ped_duration_ms = (uint32_t)handle->config.pedestrian_timer_s * 1000UL;
                if (elapsed_ms >= ped_duration_ms) {
                    Gate_TriggerStop(handle);
                }
                break;
            }
            case GATE_TIMER_CYCLE:
            {
                uint32_t cycle_duration_ms = (uint32_t)handle->config.cycle_timer_s * 1000UL;
                if (cycle_duration_ms > 0 && elapsed_ms >= cycle_duration_ms) {
                    
                    // NOVA LOGIKA: Provjera timeout-a za bravu sa senzorom
                    if (handle->current_state == GATE_STATE_OPEN && handle->config.control_type == CONTROL_TYPE_SIMPLE_LOCK) {
                        // Vrijeme je isteklo, a senzor nije javio da je zatvoreno. Proglasi greÅ¡ku.
                        handle->current_state = GATE_STATE_FAULT;
                        handle->active_timer_type = GATE_TIMER_NONE;
                        handle->timer_start_tick = 0;
                        shouldDrawScreen = 1; 
                        break;
                    }

                    // PostojeÄ‡a logika za kapije u pokretu
                    if (handle->current_state == GATE_STATE_OPENING) {
                        handle->current_state = GATE_STATE_OPEN;
                    } else if (handle->current_state == GATE_STATE_CLOSING) {
                        handle->current_state = GATE_STATE_CLOSED;
                    }
                    
                    handle->active_timer_type = GATE_TIMER_NONE;
                    handle->timer_start_tick = 0;
                    shouldDrawScreen = 1;
                }
                break;
            }
            case GATE_TIMER_NONE:
            default:
                break;
        }
    }
}
/**
 ******************************************************************************
 * @brief       VraÄ‡a "handle" (pokazivaÄ) na instancu ureÄ‘aja.
 * @author      Gemini & [VaÅ¡e Ime]
 * @param       index Indeks ureÄ‘aja (0-5).
 * @retval      Gate_Handle* PokazivaÄ na instancu, ili NULL ako je indeks neispravan.
 ******************************************************************************
 */
Gate_Handle* Gate_GetInstance(uint8_t index)
{
    if (index < GATE_MAX_COUNT)
    {
        return &gates[index];
    }
    return NULL;
}

/**
 ******************************************************************************
 * @brief       VraÄ‡a broj trenutno konfigurisanih ureÄ‘aja.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Koristi se u UI-ju za dinamiÄko iscrtavanje ikonica.
 * @param       None
 * @retval      uint8_t Broj ureÄ‘aja Äiji `control_type` nije `CONTROL_TYPE_NONE`.
 ******************************************************************************
 */
uint8_t Gate_GetCount(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++) {
        if (gates[i].config.control_type != CONTROL_TYPE_NONE) {
            count++;
        }
    }
    return count;
}

/**
 ******************************************************************************
 * @brief       Aktivira "pametnu" step-by-step (toggle) komandu.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerSmartStep(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_SMART_STEP);
}

/**
 ******************************************************************************
 * @brief       Eksplicitno pokreÄ‡e puni ciklus otvaranja.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerFullCycleOpen(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_OPEN_CYCLE);
}

/**
 ******************************************************************************
 * @brief       Eksplicitno pokreÄ‡e puni ciklus zatvaranja.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerFullCycleClose(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_CLOSE_CYCLE);
}

/**
 ******************************************************************************
 * @brief       Aktivira pjeÅ¡aÄki mod.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerPedestrian(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_PEDESTRIAN);
}

/**
 ******************************************************************************
 * @brief       Aktivira komandu za zaustavljanje.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerStop(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_STOP);
}

/**
 ******************************************************************************
 * @brief       Aktivira komandu za otkljuÄavanje brave.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_TriggerUnlock(Gate_Handle* handle)
{
    Gate_SendAction(handle, UI_COMMAND_UNLOCK);
}

/**
 ******************************************************************************
 * @brief       [NOVA IMPLEMENTACIJA] Procesira dogaÄ‘aj sa magistrale.
 * @author      Gemini
 * @note        Ova funkcija je srce novog mehanizma. Njen jedini zadatak je da
 * utvrdi da li je pristigli dogaÄ‘aj relevantan za ovaj modul. Ako jeste,
 * prosljeÄ‘uje ga internom handleru na obradu.
 ******************************************************************************
 */
void GATE_BusEvent(uint16_t address, uint8_t command, uint8_t* data, uint8_t len)
{
    // PronaÄ‘i da li bilo koja kapija koristi ovu adresu kao senzor
    Gate_Handle* handle = Gate_FindByFeedbackSensor(address);

    if (handle != NULL)
    {
        // Jeste, ova adresa je relevantna.
        // Pretpostavljamo da je stanje (ON/OFF) u prvom bajtu payload-a.
        uint8_t state = (len > 0) ? data[0] : 0;
        
        // Proslijedi dogaÄ‘aj na dalju obradu unutar modula
        HandleSensorEvent(handle, address, state);
    }
    // Ako handle == NULL, dogaÄ‘aj se ignoriÅ¡e i funkcija se odmah zavrÅ¡ava.
}
/*============================================================================*/
/* IMPLEMENTACIJA GETTERA I SETTERA                                           */
/*============================================================================*/
/**
 ******************************************************************************
 * @brief       PotvrÄ‘uje greÅ¡ku i prebacuje stanje u parcijalno otvoreno.
 * @param[in]   handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
void Gate_AcknowledgeFault(Gate_Handle* handle)
{
    if (handle && handle->current_state == GATE_STATE_FAULT)
    {
        handle->current_state = GATE_STATE_PARTIALLY_OPEN;
    }
}
/**
 ******************************************************************************
 * @brief       Dohvata odabrani Profil Kontrole za ureÄ‘aj.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @retval      GateControlType_e ID odabranog profila.
 ******************************************************************************
 */
GateControlType_e Gate_GetControlType(const Gate_Handle* handle)
{
    return handle ? handle->config.control_type : CONTROL_TYPE_NONE;
}

/**
 ******************************************************************************
 * @brief       Dohvata ID odabranog izgleda za ureÄ‘aj.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @retval      uint8_t Indeks u `gate_appearance_mapping_table`.
 ******************************************************************************
 */
uint8_t Gate_GetAppearanceId(const Gate_Handle* handle)
{
    return handle ? handle->config.appearance_id : 0;
}

/**
 ******************************************************************************
 * @brief       Dohvata korisniÄki definisan naziv za ureÄ‘aj.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @retval      const char* PokazivaÄ na string koji sadrÅ¾i naziv.
 ******************************************************************************
 */
const char* Gate_GetCustomLabel(const Gate_Handle* handle)
{
    return handle ? handle->config.custom_label : "";
}

/**
 ******************************************************************************
 * @brief       Dohvata trenutno stanje ureÄ‘aja.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @retval      GateState_e Trenutno stanje iz maÅ¡ine stanja.
 ******************************************************************************
 */
GateState_e Gate_GetState(const Gate_Handle* handle)
{
    return handle ? handle->current_state : GATE_STATE_UNDEFINED;
}

/**
 ******************************************************************************
 * @brief       Postavlja novi korisniÄki definisan naziv za ureÄ‘aj.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @param       label  PokazivaÄ na string sa novim nazivom.
 ******************************************************************************
 */
void Gate_SetCustomLabel(Gate_Handle* handle, const char* label)
{
    if (handle && label) {
        strncpy(handle->config.custom_label, label, sizeof(handle->config.custom_label) - 1);
        handle->config.custom_label[sizeof(handle->config.custom_label) - 1] = '\0';
    }
}

const char* Gate_GetProfileNameByIndex(uint8_t index) 
{
    if (index < (sizeof(g_ControlProfileLibrary) / sizeof(ProfilDeskriptor_t))) {
        return g_ControlProfileLibrary[index].profile_name;
    }
    return "Nepoznat";
}

uint8_t Gate_GetProfileCount(void) 
{
    return (sizeof(g_ControlProfileLibrary) / sizeof(ProfilDeskriptor_t));
}

const ProfilDeskriptor_t* Gate_GetProfilDeskriptor(const Gate_Handle* handle) 
{
    if (handle && handle->config.control_type < Gate_GetProfileCount()) {
        return &g_ControlProfileLibrary[handle->config.control_type];
    }
    return &g_ControlProfileLibrary[CONTROL_TYPE_NONE];
}

uint16_t Gate_GetRelayAddr(const Gate_Handle* handle, uint8_t relay_index) 
{
    if (!handle) return 0;
    switch(relay_index) {
        case 1: return handle->config.relay_cmd1.tf;
        case 2: return handle->config.relay_cmd2.tf;
        case 3: return handle->config.relay_cmd3.tf;
        case 4: return handle->config.relay_cmd4.tf;
        default: return 0;
    }
}

uint16_t Gate_GetFeedbackAddr(const Gate_Handle* handle, uint8_t sensor_index) 
{
    if (!handle) return 0;
    switch(sensor_index) {
        case 1: return handle->config.feedback_input1.tf;
        case 2: return handle->config.feedback_input2.tf;
        case 3: return handle->config.feedback_input3.tf;
        default: return 0;
    }
}

uint8_t Gate_GetCycleTimer(const Gate_Handle* handle) 
{ 
    return handle ? handle->config.cycle_timer_s : 0; 
}
uint8_t Gate_GetPedestrianTimer(const Gate_Handle* handle) 
{ 
    return handle ? handle->config.pedestrian_timer_s : 0; 
}
uint16_t Gate_GetPulseTimer(const Gate_Handle* handle) 
{ 
    return handle ? handle->config.pulse_timer_ms : 0; 
}

void Gate_SetControlType(Gate_Handle* handle, GateControlType_e type) 
{ 
    if (handle) handle->config.control_type = type; 
}
void Gate_SetAppearanceId(Gate_Handle* handle, uint8_t id) 
{ 
    if (handle) handle->config.appearance_id = id; 
}

void Gate_SetRelayAddr(Gate_Handle* handle, uint8_t relay_index, uint16_t addr) 
{
    if (!handle) return;
    switch(relay_index) {
        case 1: handle->config.relay_cmd1.tf = addr; break;
        case 2: handle->config.relay_cmd2.tf = addr; break;
        case 3: handle->config.relay_cmd3.tf = addr; break;
        case 4: handle->config.relay_cmd4.tf = addr; break;
    }
}

void Gate_SetFeedbackAddr(Gate_Handle* handle, uint8_t sensor_index, uint16_t addr) 
{
    if (!handle) return;
    switch(sensor_index) {
        case 1: handle->config.feedback_input1.tf = addr; break;
        case 2: handle->config.feedback_input2.tf = addr; break;
        case 3: handle->config.feedback_input3.tf = addr; break;
    }
}

void Gate_SetCycleTimer(Gate_Handle* handle, uint8_t seconds) 
{ 
    if (handle) handle->config.cycle_timer_s = seconds; 
}
void Gate_SetPedestrianTimer(Gate_Handle* handle, uint8_t seconds) 
{ 
    if (handle) handle->config.pedestrian_timer_s = seconds; 
}
void Gate_SetPulseTimer(Gate_Handle* handle, uint16_t ms) 
{ 
    if (handle) handle->config.pulse_timer_ms = ms; 
}

void Gate_SetState(Gate_Handle* handle, GateState_e new_state)
{
    if (handle) {
        handle->current_state = new_state;
    }
}
/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Privatna funkcija koja prevodi UI komandu u fiziÄku akciju.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ovo je centralno mjesto gdje se Äita "Profil Kontrole". Na osnovu
 * ID-ja profila pohranjenog u `handle->config.control_type`,
 * pronalazi se odgovarajuÄ‡i deskriptor u `g_ControlProfileLibrary`.
 * Zatim se iz `command_map` niza tog deskriptora Äita koja
 * konkretna akcija (`GateAction_t`) odgovara proslijeÄ‘enoj
 * UI komandi.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @param       command UI komanda koju treba izvrÅ¡iti.
 ******************************************************************************
 */
static void Gate_SendAction(Gate_Handle* handle, UI_Command_e command)
{
    if (!handle || handle->config.control_type == CONTROL_TYPE_NONE) return;

    const ProfilDeskriptor_t* profil = &g_ControlProfileLibrary[handle->config.control_type];
    const GateAction_t* akcija = &profil->command_map[command];

    // Resetujemo prethodne tajmere prije pokretanja nove akcije
    // Za kontinuirane komande, prvo zaustavljamo sve motore
    if (akcija->is_pulse == false || command == UI_COMMAND_STOP) {
         Gate_StopAllRelays(handle);
    }
    
    handle->active_timer_type = GATE_TIMER_NONE;
    handle->timer_start_tick = 0;

     // ====================================================================
    // === KLJUÄŒNA ISPRAVKA: Logika Brave je prva i nezavisna (BINARNA) ===
    // ====================================================================

    if (handle->config.control_type == CONTROL_TYPE_SIMPLE_LOCK)
    {
       
            // STANJE BRAVE: OtkljuÄano (samo dok traje puls)
            handle->current_state = GATE_STATE_OPEN;
            handle->active_timer_type = GATE_TIMER_PULSE; // Koristi samo puls tajmer
            handle->timer_start_tick = HAL_GetTick() ? HAL_GetTick() : 1;
            shouldDrawScreen = 1;

            Gate_SendRawCommand(handle, akcija->target_relay_index, akcija->is_pulse);
            return; // Brava zavrÅ¡ava ovde, ne ide dalje u Smart Step ili Ciklus logiku!
      
       
    }

    // ====================================================================
    // === Logika za KAPIJE/RAMPE (ukljuÄuje OPENING/CLOSING) ===
    // ====================================================================
    
    // Postavljanje poÄetnog stanja i pokretanje tajmera ciklusa
    switch(command)
    {
        case UI_COMMAND_OPEN_CYCLE:
        case UI_COMMAND_PEDESTRIAN:
            Gate_SetState(handle, GATE_STATE_OPENING);
            // ISPRAVKA: Pokretanje tajmera koji je nedostajao
            handle->active_timer_type = GATE_TIMER_CYCLE;
            handle->timer_start_tick = HAL_GetTick() ? HAL_GetTick() : 1;
            break;

        case UI_COMMAND_CLOSE_CYCLE:
            Gate_SetState(handle, GATE_STATE_CLOSING);
            // ISPRAVKA: Pokretanje tajmera koji je nedostajao
            handle->active_timer_type = GATE_TIMER_CYCLE;
            handle->timer_start_tick = HAL_GetTick() ? HAL_GetTick() : 1;
            break;

        case UI_COMMAND_SMART_STEP:
            // ISPRAVKA: Kompletno ispravljena toggle logika
            // Ako je kapija zatvorena -> pokreni otvaranje
            if (handle->current_state == GATE_STATE_CLOSED) {
                handle->current_state = GATE_STATE_OPENING;
            } 
            // Ako je kapija otvorena -> pokreni zatvaranje
            else if (handle->current_state == GATE_STATE_OPEN) {
                handle->current_state = GATE_STATE_CLOSING;
            }
            // Ako se kretala (bilo otvarala ili zatvarala) -> zaustavi je (slanjem iste komande)
            else {
                handle->current_state = GATE_STATE_PARTIALLY_OPEN;
            }
            // Za sve sluÄajeve S-S komande, pokreni tajmer ciklusa
            handle->active_timer_type = GATE_TIMER_CYCLE;
            handle->timer_start_tick = HAL_GetTick() ? HAL_GetTick() : 1;
            break;
            
        case UI_COMMAND_STOP:
            Gate_SetState(handle, GATE_STATE_PARTIALLY_OPEN);
            shouldDrawScreen = 1;
            return; // Izlazimo odmah jer STOP samo gasi releje

         case UI_COMMAND_UNLOCK:
            // KLJUÄŒNA ISPRAVKA: Brava se vizuelno otvara samo DOK traje puls
            handle->current_state = GATE_STATE_OPEN; // VIZUELNO: Otvoreno
            shouldDrawScreen = 1;
            // Ne prekidamo ovde, jer treba da poÅ¡aljemo sirovu komandu u nastavku
            break;

        default:
            break;
    }
    
    // Slanje stvarne komande na relej
    Gate_SendRawCommand(handle, akcija->target_relay_index, akcija->is_pulse);
}
/**
 ******************************************************************************
 * @brief       Å alje sirovu komandu (pulsnu ili kontinuiranu) na relej.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Funkcija prima generiÄki indeks releja (1-4) i na osnovu njega
 * pronalazi stvarnu adresu releja u konfiguraciji. Zatim Å¡alje
 * BINARY_ON komandu. Ako je `is_pulse` postavljeno na `true`,
 * aktivira `GATE_TIMER_PULSE` koji Ä‡e nakon isteka vremena
 * poslati BINARY_OFF komandu.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @param       relay_index GeneriÄki indeks releja (1-4).
 * @param       is_pulse Da li je komanda pulsna.
 ******************************************************************************
 */
static void Gate_SendRawCommand(Gate_Handle* handle, uint8_t relay_index, bool is_pulse)
{
    if (relay_index == 0) return;
    uint16_t relay_addr = Gate_GetRelayAddressByIndex(handle, relay_index);
    if (relay_addr == 0) return;
    
    uint8_t buff[3] = { (relay_addr >> 8) & 0xFF, relay_addr & 0xFF, BINARY_ON };
    AddCommand(&binaryQueue, BINARY_SET, buff, 3);
    
    if (is_pulse) {
        handle->active_timer_type = GATE_TIMER_PULSE;
        handle->timer_start_tick = HAL_GetTick() ? HAL_GetTick() : 1;
        handle->pulse_relay_index = relay_index;
    }
}

/**
 ******************************************************************************
 * @brief       Zaustavlja sve motore slanjem OFF komande na sve releje ureÄ‘aja.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Iterira kroz sve 4 generiÄke adrese releja za dati handle i Å¡alje
 * im `BINARY_OFF` komandu.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
static void Gate_StopAllRelays(Gate_Handle* const handle)
{
    uint8_t buff[3] = {0};
    uint16_t relay_addr;
    for (uint8_t i = 1; i <= 4; i++) {
        relay_addr = Gate_GetRelayAddressByIndex(handle, i);
        if (relay_addr > 0) {
            buff[0] = (relay_addr >> 8) & 0xFF;
            buff[1] = relay_addr & 0xFF;
            buff[2] = BINARY_OFF;
            AddCommand(&binaryQueue, BINARY_SET, buff, 3);
        }
    }
}

/**
 ******************************************************************************
 * @brief       PomoÄ‡na funkcija za dobijanje adrese generiÄkog releja po indeksu.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 * @param       index GeneriÄki indeks (1-4).
 * @retval      uint16_t Stvarna adresa releja.
 ******************************************************************************
 */
static uint16_t Gate_GetRelayAddressByIndex(const Gate_Handle* handle, uint8_t index)
{
    if (!handle) return 0;
    switch (index) {
        case 1: return handle->config.relay_cmd1.tf;
        case 2: return handle->config.relay_cmd2.tf;
        case 3: return handle->config.relay_cmd3.tf;
        case 4: return handle->config.relay_cmd4.tf;
        default: return 0;
    }
}

/**
 ******************************************************************************
 * @brief       [POSTOJEÄ†A FUNKCIJA, SADA INTERNA] ObraÄ‘uje promjenu stanja senzora.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Ova funkcija sadrÅ¾i logiku za aÅ¾uriranje maÅ¡ine stanja.
 * Poziva je `GATE_BusEvent` nakon Å¡to je potvrÄ‘eno da je dogaÄ‘aj
 * relevantan za datu instancu (`handle`).
 ******************************************************************************
 */
static void HandleSensorEvent(Gate_Handle* handle, uint16_t sensor_addr, uint8_t state)
{
    // Ako senzor nije aktivan (stanje 0), ne radimo niÅ¡ta.
    // Logika se okida samo na aktivaciju senzora.
    if (state == 0) return;

    // Provjeravamo da li je aktiviran senzor za OTVORENO.
    if (handle->config.feedback_input1.tf == sensor_addr)
    {
        handle->current_state = GATE_STATE_OPEN;
        // Ako je kapija stigla na kraj, zaustavi sve tajmere i motore.
        handle->active_timer_type = GATE_TIMER_NONE;
        handle->timer_start_tick = 0;
        Gate_StopAllRelays(handle);
    }
    // Provjeravamo da li je aktiviran senzor za ZATVORENO.
    else if (handle->config.feedback_input2.tf == sensor_addr)
    {
        handle->current_state = GATE_STATE_CLOSED;
        // Ako je kapija stigla na kraj, zaustavi sve tajmere i motore.
        handle->active_timer_type = GATE_TIMER_NONE;
        handle->timer_start_tick = 0;
        Gate_StopAllRelays(handle);
    }
    
    // Obavijesti GUI da je potrebno ponovno iscrtavanje.
    shouldDrawScreen = 1;
}

/**
 ******************************************************************************
 * @brief       PronaÄ‘i ureÄ‘aj na osnovu adrese njegovog feedback senzora.
 * @param       sensor_addr Adresa aktiviranog senzora.
 * @retval      Gate_Handle* PokazivaÄ na pronaÄ‘eni ureÄ‘aj, ili NULL.
 ******************************************************************************
 */
static Gate_Handle* Gate_FindByFeedbackSensor(uint16_t sensor_addr)
{
    if (sensor_addr == 0) return NULL;
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        if (gates[i].config.control_type != CONTROL_TYPE_NONE)
        {
            if (gates[i].config.feedback_input1.tf == sensor_addr ||
                gates[i].config.feedback_input2.tf == sensor_addr ||
                gates[i].config.feedback_input3.tf == sensor_addr)
            {
                return &gates[i];
            }
        }
    }
    return NULL;
}

/**
 ******************************************************************************
 * @brief       Inicijalizuje jedan ureÄ‘aj iz EEPROM-a, uz validaciju.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        ÄŒita blok podataka iz EEPROM-a za dati indeks. Provjerava
 * "magiÄni broj" i CRC. Ako bilo koja provjera ne uspije,
 * poziva `Gate_SetDefault` da postavi fabriÄke vrijednosti i
 * odmah ih snima nazad u EEPROM.
 * @param       index Indeks ureÄ‘aja (0-5).
 ******************************************************************************
 */
static void Gate_Init_Single(uint8_t index)
{
    if (index >= GATE_MAX_COUNT) return;
    
    Gate_Handle* handle = &gates[index];
    uint16_t address = EE_GATES + (index * sizeof(Gate_EepromConfig_t));
    
    EE_ReadBuffer((uint8_t*)&handle->config, address, sizeof(Gate_EepromConfig_t));
    
    if (handle->config.magic_number != EEPROM_MAGIC_NUMBER)
    {
        Gate_SetDefault(handle);
        Gate_Save_Single(index);
    }
    else
    {
        uint16_t received_crc = handle->config.crc;
        handle->config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Gate_EepromConfig_t));
        if (received_crc != calculated_crc)
        {
            Gate_SetDefault(handle);
            Gate_Save_Single(index);
        }
    }
    
    // Inicijalizacija runtime varijabli
    handle->current_state = GATE_STATE_UNDEFINED;
    handle->timer_start_tick = 0;
    handle->active_timer_type = GATE_TIMER_NONE;
    handle->pulse_relay_index = 0;
}

/**
 ******************************************************************************
 * @brief       Snima konfiguraciju jednog ureÄ‘aja u EEPROM.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        Prije snimanja, postavlja "magiÄni broj" i izraÄunava novi CRC.
 * @param       index Indeks ureÄ‘aja (0-5).
 ******************************************************************************
 */
static void Gate_Save_Single(uint8_t index)
{
    if (index >= GATE_MAX_COUNT) return;
    
    Gate_Handle* handle = &gates[index];
    uint16_t address = EE_GATES + (index * sizeof(Gate_EepromConfig_t));
    
    handle->config.magic_number = EEPROM_MAGIC_NUMBER; 
    handle->config.crc = 0;
    handle->config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Gate_EepromConfig_t));
    
    EE_WriteBuffer((uint8_t*)&handle->config, address, sizeof(Gate_EepromConfig_t));
}

/**
 ******************************************************************************
 * @brief       Postavlja konfiguraciju jednog ureÄ‘aja na fabriÄke vrijednosti.
 * @author      Gemini & [VaÅ¡e Ime]
 * @note        BriÅ¡e cijelu `config` strukturu i postavlja `control_type` na
 * `CONTROL_TYPE_NONE`, Äime efektivno deaktivira ureÄ‘aj.
 * @param       handle PokazivaÄ na instancu ureÄ‘aja.
 ******************************************************************************
 */
static void Gate_SetDefault(Gate_Handle* const handle)
{
    if (!handle) return;
    memset(&handle->config, 0, sizeof(Gate_EepromConfig_t));
    handle->config.control_type = CONTROL_TYPE_NONE;
}

