/**
 ******************************************************************************
 * @file    ventilator.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija kompletne logike za upravljanje ventilatorom.
 *
 * @note
 * Ovaj fajl sadrži privatnu, staticku `ventilator` instancu koja cuva
 * stanje i konfiguraciju, cineci je nevidljivom za ostatak programa.
 *
 * Sva interakcija sa podacima se vrši preko `handle`-a koji se prosljeduje
 * funkcijama. `Ventilator_Service()` periodicno poziva pomocne `Handle...`
 * funkcije za obradu tajmera, okidaca i promjena stanja koje treba poslati
 * na RS485 bus.
 ******************************************************************************
 */

#if (__VENTILATOR_CTRL_H__ != FW_BUILD)
#error "ventilator header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "ventilator.h"
#include "lights.h"           // Potreban za provjeru stanja svjetala (okidaca)
#include "display.h"          // Potreban za g_display_settings i signaliziranje GUI-ju
#include "stm32746g_eeprom.h" // Potreban za adrese i funkcije za upis/citanje
#include "rs485.h"            // Potreban za slanje komandi na Modbus

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI                                               */
/*============================================================================*/

/**
 * @brief Faktor za konverziju vrijednosti tajmera (npr. 5) u milisekunde.
 * @note  Vrijednost 5 iz menija se množi sa ovim faktorom da bi se dobilo 50000ms (50s).
 */
#define VENTILATOR_TIMER_FACTOR (10 * 1000) // 10 sekundi u milisekundama

/**
 * @brief Fleg za oznacavanje aktivnosti ventilatora (bit 0).
 */
#define VENTILATOR_FLAG_ACTIVE (1U << 0)

/*============================================================================*/
/* PRIVATNA DEFINICIJA STRUKTURE                                              */
/*============================================================================*/

/**
 * @brief Puna definicija glavne strukture, sakrivena od ostatka programa.
 * Ovo je konkretna implementacija `Ventilator_Handle` tipa.
 */
struct Ventilator_s {
    Ventilator_EepromConfig_t config; /**< Ugniježdena struktura sa podacima koji se cuvaju u EEPROM. */

    // --- Runtime podaci (ne cuvaju se trajno) ---
    uint32_t delayOnTimerStart;     /**< Vrijeme (`HAL_GetTick()`) kada je poceo tajmer za odloženo paljenje. 0 = neaktivan. */
    uint32_t delayOffTimerStart;    /**< Vrijeme (`HAL_GetTick()`) kada je poceo tajmer za odloženo gašenje. 0 = neaktivan. */
    uint8_t flags;                  /**< 8-bitni registar za razne flegove. Trenutno se koristi samo bit 0 za stanje ON/OFF. */
};

/*============================================================================*/
/* PRIVATNA (STATICKA) INSTANCA                                               */
/*============================================================================*/

/**
 * @brief Jedina instanca ventilatora u cijelom sistemu (Singleton pattern).
 * @note  Kljucna rijec 'static' je cini vidljivom samo unutar ovog .c fajla.
 */
static struct Ventilator_s ventilator;

/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOCNIH FUNKCIJA                                    */
/*============================================================================*/
static void HandleDelayOffTimer(Ventilator_Handle* const handle);
static void HandleDelayOnTimer(Ventilator_Handle* const handle);
static void HandleTriggerSources(Ventilator_Handle* const handle);
static void HandleVentilatorStatusChanges(Ventilator_Handle* const handle);
static bool Ventilator_isConfigured(const Ventilator_Handle* const handle);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

// --- Grupa 1: Upravljanje Instancom i Osnovne Funkcije ---

/**
 * @brief  Vraca pointer (handle) na jedinu (singleton) instancu ventilatora.
 * @note   Ovo je jedini ispravan nacin da se dobije "handle" za rad sa ventilatorom.
 * @retval Ventilator_Handle* Pointer (handle) na instancu ventilatora.
 */
Ventilator_Handle* Ventilator_GetInstance(void)
{
    return &ventilator;
}

/**
 * @brief  Inicijalizuje modul ventilatora iz EEPROM-a, uz provjeru validnosti.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Init(Ventilator_Handle* const handle)
{
    // Procitaj cijeli blok podataka iz EEPROM-a u `handle->config` dio strukture.
    EE_ReadBuffer((uint8_t*)&(handle->config), EE_VENTILATOR, sizeof(Ventilator_EepromConfig_t));

    // Provjeri magicni broj. Ako nije ispravan, podaci su nevažeci.
    if (handle->config.magic_number != EEPROM_MAGIC_NUMBER) {
        // Podaci nisu validni, postavi fabricke vrijednosti.
        Ventilator_SetDefault(handle);
        // I odmah ih snimi da bi EEPROM bio validan za sljedeci start.
        Ventilator_Save(handle);
    } else {
        // Magicni broj je OK, provjeri CRC.
        uint16_t received_crc = handle->config.crc;
        handle->config.crc = 0; // Privremeno postavi na 0 radi ispravnog izracuna.
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Ventilator_EepromConfig_t));

        if (received_crc != calculated_crc) {
            // Podaci su ošteceni (CRC se ne poklapa), vrati na fabricke vrijednosti.
            Ventilator_SetDefault(handle);
            Ventilator_Save(handle);
        }
    }

    // Inicijalizuj runtime varijable (one se ne cuvaju u EEPROM-u).
    handle->flags = 0;
    handle->delayOnTimerStart = 0;
    handle->delayOffTimerStart = 0;
}

/**
 * @brief  Cuva trenutnu konfiguraciju ventilatora u EEPROM.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Save(Ventilator_Handle* const handle)
{
    handle->config.magic_number = EEPROM_MAGIC_NUMBER; // Postavi "potpis"
    handle->config.crc = 0; // Nuliraj CRC polje radi ispravnog izracuna
    // Izracunaj CRC nad cijelom `config` strukturom.
    handle->config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Ventilator_EepromConfig_t));
    // Snimi cijelu strukturu odjednom.
    EE_WriteBuffer((uint8_t*)&(handle->config), EE_VENTILATOR, sizeof(Ventilator_EepromConfig_t));
}

/**
 * @brief  Glavna servisna petlja za ventilator.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Service(Ventilator_Handle* const handle)
{
    // Ako ventilator uopšte nije konfigurisan (nema relej ili lokalni pin), ne radi ništa.
    if (!Ventilator_isConfigured(handle)) {
        return;
    }
    // Ako u meniju na displeju nije odabran mod za ventilator, ne radi ništa.
    if (g_display_settings.selected_control_mode != MODE_VENTILATOR) {
        return;
    }

    // Pozovi privatne pomocne funkcije da obave posao.
    HandleDelayOffTimer(handle);
    HandleDelayOnTimer(handle);
    HandleTriggerSources(handle);
    HandleVentilatorStatusChanges(handle);

    // Fizicka kontrola pina se dešava na osnovu statusnog flega.
    if (handle->config.local_pin > 0)
    {
        if (Ventilator_isActive(handle))
        {
            SetPin(handle->config.local_pin, 1);
        }
        else
        {
            SetPin(handle->config.local_pin, 0);
        }
    }
}


/**
 * @brief Postavlja sve parametre ventilatora na sigurne fabricke vrijednosti.
 * @note  Ova funkcija resetuje i konfiguracione (EEPROM) i runtime clanove
 * instance na koju `handle` pokazuje.
 * @param handle Pointer na instancu ventilatora.
 */
// << IZMJENA: Uklonjen 'static' da bi funkcija postala javna.
void Ventilator_SetDefault(Ventilator_Handle* const handle)
{
    // KORAK 1: Sigurnosno nuliranje cijele strukture.
    memset(handle, 0, sizeof(struct Ventilator_s));

    // KORAK 2: Eksplicitno postavljanje default vrijednosti.
    handle->config.magic_number = 0;
    handle->config.crc = 0;
    handle->config.relay = 0;
    handle->config.delayOnTime = 0;
    handle->config.delayOffTime = 0;
    handle->config.trigger_source1 = 0;
    handle->config.trigger_source2 = 0;
    handle->config.local_pin = 0;

    // Runtime parametri se takoder resetuju.
    handle->delayOnTimerStart = 0;
    handle->delayOffTimerStart = 0;
    handle->flags = 0;
}
// --- Grupa 2: Getteri i Setteri za Konfiguraciju ---

void Ventilator_setRelay(Ventilator_Handle* const handle, const uint16_t val) { handle->config.relay = val; }
uint16_t Ventilator_getRelay(const Ventilator_Handle* const handle) { return handle->config.relay; }
void Ventilator_setDelayOnTime(Ventilator_Handle* const handle, uint8_t val) { handle->config.delayOnTime = val; }
uint8_t Ventilator_getDelayOnTime(const Ventilator_Handle* const handle) { return handle->config.delayOnTime; }
void Ventilator_setDelayOffTime(Ventilator_Handle* const handle, uint8_t val) { handle->config.delayOffTime = val; }
uint8_t Ventilator_getDelayOffTime(const Ventilator_Handle* const handle) { return handle->config.delayOffTime; }
void Ventilator_setTriggerSource1(Ventilator_Handle* const handle, const uint8_t val) { handle->config.trigger_source1 = val; }
uint8_t Ventilator_getTriggerSource1(const Ventilator_Handle* const handle) { return handle->config.trigger_source1; }
void Ventilator_setTriggerSource2(Ventilator_Handle* const handle, const uint8_t val) { handle->config.trigger_source2 = val; }
uint8_t Ventilator_getTriggerSource2(const Ventilator_Handle* const handle) { return handle->config.trigger_source2; }
void Ventilator_setLocalPin(Ventilator_Handle* const handle, const uint8_t val) { handle->config.local_pin = val; }
uint8_t Ventilator_getLocalPin(const Ventilator_Handle* const handle) { return handle->config.local_pin; }


// --- Grupa 3: Kontrola Stanja ---

/**
 * @brief  Ukljucuje ventilator.
 * @param  handle Pointer na instancu ventilatora.
 * @param  useDelay Ako je true, koristi odlaganje ako je konfigurisano.
 */
void Ventilator_On(Ventilator_Handle* const handle, const bool useDelay)
{
    if(!Ventilator_isConfigured(handle)) return;

    // Ako stigne komanda za paljenje, poništi eventualni tajmer za gašenje.
    handle->delayOffTimerStart = 0;

    if(useDelay && (handle->config.delayOnTime > 0))
    {
        // Ako je odloženo paljenje aktivno, samo pokreni tajmer.
        handle->delayOnTimerStart = HAL_GetTick();
        if(!handle->delayOnTimerStart) handle->delayOnTimerStart = 1; // Osiguraj da nije 0
    }
    else
    {
        // Inace, odmah upali ventilator.
        handle->delayOnTimerStart = 0; // Poništi tajmer za ukljucenje
        handle->flags |= VENTILATOR_FLAG_ACTIVE; // Postavi fleg za aktivnost

        // Ako je `delayOffTime` podešeno, pokreni tajmer za automatsko gašenje.
        if (handle->config.delayOffTime > 0)
        {
            handle->delayOffTimerStart = HAL_GetTick();
            if(!handle->delayOffTimerStart) handle->delayOffTimerStart = 1;
        }
    }
}

/**
 * @brief  Iskljucuje ventilator.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Off(Ventilator_Handle* const handle)
{
    if(!Ventilator_isConfigured(handle)) return;

    // Rucna komanda za iskljucenje ima najviši prioritet.
    // Poništavamo sve tajmere i odmah gasimo ventilator.
    handle->delayOnTimerStart = 0;
    handle->delayOffTimerStart = 0;
    handle->flags &= ~VENTILATOR_FLAG_ACTIVE;
}

/**
 * @brief  Provjerava da li je ventilator trenutno aktivan (upaljen).
 * @param  handle Pointer na instancu ventilatora.
 * @retval bool `true` ako je ventilator upaljen, inace `false`.
 */
bool Ventilator_isActive(const Ventilator_Handle* const handle)
{
    return (handle->flags & VENTILATOR_FLAG_ACTIVE);
}


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH POMOCNIH FUNKCIJA                                 */
/*============================================================================*/

/**
 * @brief Pomocna funkcija koja provjerava da li je ventilator uopšte konfigurisan.
 * @param  handle Pointer na instancu ventilatora.
 * @retval bool `true` ako je konfigurisan (ima relej ili pin), inace `false`.
 */
static bool Ventilator_isConfigured(const Ventilator_Handle* const handle)
{
    return (handle->config.relay > 0) || (handle->config.local_pin > 0);
}

/**
 * @brief Provjerava i upravlja tajmerom za odloženo iskljucivanje.
 * @param handle Pointer na instancu ventilatora.
 */
static void HandleDelayOffTimer(Ventilator_Handle* const handle)
{
    if(handle->delayOffTimerStart && ((HAL_GetTick() - handle->delayOffTimerStart) >= (handle->config.delayOffTime * VENTILATOR_TIMER_FACTOR)))
    {
        handle->flags &= ~VENTILATOR_FLAG_ACTIVE; // Ugasi ventilator
        handle->delayOffTimerStart = 0;         // Resetuj tajmer
        DISP_SignalDynamicIconUpdate();         // Obavijesti GUI da ažurira ikonicu
    }
}

/**
 * @brief Provjerava i upravlja tajmerom za odloženo ukljucivanje.
 * @param handle Pointer na instancu ventilatora.
 */
static void HandleDelayOnTimer(Ventilator_Handle* const handle)
{
    if(handle->delayOnTimerStart && ((HAL_GetTick() - handle->delayOnTimerStart) >= (handle->config.delayOnTime * VENTILATOR_TIMER_FACTOR)))
    {
        handle->flags |= VENTILATOR_FLAG_ACTIVE; // Upali ventilator
        handle->delayOnTimerStart = 0;          // Resetuj tajmer
        DISP_SignalDynamicIconUpdate();         // Obavijesti GUI da ažurira ikonicu
    }
}

/**
 * @brief Provjerava stanje okidackih svjetala i upravlja stanjem ventilatora.
 * @param handle Pointer na instancu ventilatora.
 */
static void HandleTriggerSources(Ventilator_Handle* const handle)
{
    static bool old_trigger_state = false;
    bool current_trigger_state = false;

    // Provjera prvog izvora okidanja
    uint8_t trigger_index_1 = Ventilator_getTriggerSource1(handle);
    if (trigger_index_1 > 0 && trigger_index_1 <= LIGHTS_MODBUS_SIZE) {
        LIGHT_Handle* light_handle1 = LIGHTS_GetInstance(trigger_index_1 - 1);
        if (light_handle1 && LIGHT_isActive(light_handle1)) {
            current_trigger_state = true;
        }
    }

    // Provjera drugog izvora, samo ako prvi nije vec aktivirao
    uint8_t trigger_index_2 = Ventilator_getTriggerSource2(handle);
    if (!current_trigger_state && trigger_index_2 > 0 && trigger_index_2 <= LIGHTS_MODBUS_SIZE) {
        LIGHT_Handle* light_handle2 = LIGHTS_GetInstance(trigger_index_2 - 1);
        if (light_handle2 && LIGHT_isActive(light_handle2)) {
            current_trigger_state = true;
        }
    }

    // Logika za paljenje/gašenje na osnovu promjene stanja
    if (current_trigger_state != old_trigger_state) {
        if (current_trigger_state) { // Stanje se promijenilo sa OFF na ON
            Ventilator_On(handle, true); // Ukljuci sa odgodom ako je konfigurisana
        } else { // Stanje se promijenilo sa ON na OFF
            // Ne gasi se odmah, nego se pokrece tajmer za odloženo gašenje
            if (handle->config.delayOffTime > 0) {
                handle->delayOffTimerStart = HAL_GetTick();
                if (!handle->delayOffTimerStart) handle->delayOffTimerStart = 1;
            }
        }
        old_trigger_state = current_trigger_state;
    }
}

/**
 * @brief Upravlja promjenama stanja i šalje Modbus komande na bus.
 * @note  Detektuje promjenu `flags` registra i ako se desila, formira i
 * šalje odgovarajucu `BINARY_SET` komandu u red za slanje.
 * @param handle Pointer na instancu ventilatora.
 */
static void HandleVentilatorStatusChanges(Ventilator_Handle* const handle)
{
    static uint8_t old_flags = 0;
    if (old_flags != handle->flags)
    {
        if(handle->config.relay > 0)
        {
            uint8_t sendDataBuff[3] = {0};
            sendDataBuff[0] = (handle->config.relay >> 8) & 0xFF;
            sendDataBuff[1] = handle->config.relay & 0xFF;
            sendDataBuff[2] = Ventilator_isActive(handle) ? BINARY_ON : BINARY_OFF;
            AddCommand(&binaryQueue, BINARY_SET, sendDataBuff, 3);
        }
        old_flags = handle->flags;
    }
}
