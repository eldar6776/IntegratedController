/**
 ******************************************************************************
 * @file    curtain.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija kompletne logike za upravljanje roletnama.
 *
 * @note
 * Verzija 2.0: Kompletna implementacija koja odgovara originalnoj
 * funkcionalnosti, ali sa sakrivenim podacima. Pristup pojedinacnoj
 * roletni izvana moguc je iskljucivo preko `Curtain_GetInstanceByIndex()`
 * i `Curtain_GetByLogicalIndex()` funkcija.
 ******************************************************************************
 */

#if (__CURTAIN_CTRL_H__ != FW_BUILD)
#error "curtain header version mismatch"
#endif

#include "main.h"
#include "curtain.h"
#include "display.h"
#include "stm32746g_eeprom.h"
#include "rs485.h"

/**
 * @brief Puna definicija glavne "runtime" strukture za jednu roletnu.
 */
struct Curtain_s
{
    Curtain_EepromConfig_t config;  /**< Konfiguracioni podaci koji se cuvaju. */
    uint8_t  upDown;                /**< Trenutno željeno stanje (STOP, UP, DOWN). */
    uint8_t  upDown_old;            /**< Prethodno stanje (za detekciju promjene). */
    uint32_t upDownTimer;           /**< Vrijeme starta tajmera za kretanje. */
    bool     external_cmd;          /**< Fleg koji ukazuje da je komanda došla sa busa. */
};

/**
 * @brief Struktura koja se puni iz EEPROM-a pri inicijalizaciji.
 * @note  `static` - vidljiva samo u ovom fajlu.
 */
static Curtains_EepromData_t curtains_eeprom_data;

/**
 * @brief Niz sa runtime podacima za sve roletne. Sakriven od ostatka programa.
 * @note  `static` - vidljiv samo u ovom fajlu.
 */
static struct Curtain_s curtains[CURTAINS_SIZE];

/**
 * @brief Brojac stvarno konfigurisanih roletni.
 * @note  `static` - vidljiv samo u ovom fajlu.
 */
static uint8_t curtains_count = 0;

static void HandleCurtainMovement(Curtain_Handle* const handle);
static void HandleCurtainDirectionChange(Curtain_Handle* const handle);
static void Curtains_CountConfigured(void);
static Curtain_Handle* FindCurtainByRelay(uint16_t relay);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

// --- Grupa 1: Inicijalizacija i Servis ---

void Curtains_Init(void)
{
    EE_ReadBuffer((uint8_t*)&curtains_eeprom_data, EE_CURTAINS, sizeof(Curtains_EepromData_t));

    if (curtains_eeprom_data.magic_number != EEPROM_MAGIC_NUMBER) {
        Curtains_SetDefault();
        Curtains_Save();
    } else {
        uint16_t received_crc = curtains_eeprom_data.crc;
        curtains_eeprom_data.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&curtains_eeprom_data, sizeof(Curtains_EepromData_t));
        if (received_crc != calculated_crc) {
            Curtains_SetDefault();
            Curtains_Save();
        }
    }

    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        curtains[i].config = curtains_eeprom_data.curtains[i];
        curtains[i].upDown = CURTAIN_STOP;
        curtains[i].upDown_old = CURTAIN_STOP;
        curtains[i].upDownTimer = 0;
        curtains[i].external_cmd = false;
    }

    Curtains_CountConfigured();
}

void Curtain_Service(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain_Handle* handle = &curtains[i];
        if(!Curtain_hasRelays(handle)) continue;

        HandleCurtainMovement(handle);

        if (!handle->external_cmd) {
            HandleCurtainDirectionChange(handle);
        }
    }
}

// --- Grupa 2: Konfiguracija i Cuvanje ---

void Curtains_Save(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++) {
        curtains_eeprom_data.curtains[i] = curtains[i].config;
    }

    curtains_eeprom_data.magic_number = EEPROM_MAGIC_NUMBER;
    curtains_eeprom_data.crc = 0;
    curtains_eeprom_data.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&curtains_eeprom_data, sizeof(Curtains_EepromData_t));
    EE_WriteBuffer((uint8_t*)&curtains_eeprom_data, EE_CURTAINS, sizeof(Curtains_EepromData_t));

    Curtains_CountConfigured();
}

void Curtains_SetDefault(void)
{
    memset(&curtains_eeprom_data, 0, sizeof(Curtains_EepromData_t));
    curtains_eeprom_data.upDownDurationSeconds = 15;
}

void Curtain_SetMoveTime(const uint8_t seconds) { curtains_eeprom_data.upDownDurationSeconds = seconds; }
uint8_t Curtain_GetMoveTime(void) { return curtains_eeprom_data.upDownDurationSeconds; }

void Curtain_setRelayUp(Curtain_Handle* const handle, uint16_t relay) { if(handle) handle->config.relayUp = relay; }
uint16_t Curtain_getRelayUp(const Curtain_Handle* const handle) { return handle ? handle->config.relayUp : 0; }
void Curtain_setRelayDown(Curtain_Handle* const handle, uint16_t relay) { if(handle) handle->config.relayDown = relay; }
uint16_t Curtain_getRelayDown(const Curtain_Handle* const handle) { return handle ? handle->config.relayDown : 0; }

// --- Grupa 3: Pristup i Brojaci ---

Curtain_Handle* Curtain_GetInstanceByIndex(uint8_t index)
{
    if (index < CURTAINS_SIZE) {
        return &curtains[index];
    }
    return NULL;
}

Curtain_Handle* Curtain_GetByLogicalIndex(const uint8_t logical_index)
{
    uint8_t found_count = 0;
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
        if (Curtain_hasRelays(&curtains[i])) {
            if (found_count == logical_index) {
                return &curtains[i];
            }
            found_count++;
        }
    }
    return NULL;
}

uint8_t Curtains_getCount(void)
{
    return curtains_count;
}

// --- Grupa 4: Selekcija (za GUI) ---

void Curtain_Select(const uint8_t curtain_index) { curtain_selected = curtain_index; }
uint8_t Curtain_getSelected(void) { return curtain_selected; }
bool Curtain_areAllSelected(void) { return curtain_selected == curtains_count; }
void Curtain_ResetSelection(void) { curtain_selected = curtains_count; }

// --- Grupa 5: Kontrola i Upravljanje ---

void Curtain_Stop(Curtain_Handle* const handle)
{
    if(handle) handle->upDown = CURTAIN_STOP;
}

void Curtains_StopAll()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Stop(&curtains[i]);
    }
}

void Curtain_Move(Curtain_Handle* const handle, const uint8_t direction)
{
    if(handle && Curtain_hasRelays(handle))
    {
        handle->external_cmd = false;
        handle->upDown = direction;
        handle->upDownTimer = HAL_GetTick();
        if(!handle->upDownTimer) handle->upDownTimer = 1;
    }
}

void Curtains_MoveAll(const uint8_t direction)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Move(&curtains[i], direction);
    }
}

void Curtain_MoveSignal(Curtain_Handle* const handle, const uint8_t direction)
{
    if (!handle) return;
    if(Curtain_isMoving(handle) && (handle->upDown == direction)) {
        Curtain_Stop(handle);
    } else {
        Curtain_Move(handle, direction);
    }
}

void Curtains_MoveSignalAll(const uint8_t direction)
{
    bool any_moving_in_direction = Curtains_areAllMovinginSameDirection(direction);

    for (uint8_t i = 0; i < CURTAINS_SIZE; ++i) {
        if (Curtain_hasRelays(&curtains[i])) {
            if (any_moving_in_direction) {
                Curtain_Stop(&curtains[i]);
            } else {
                Curtain_Move(&curtains[i], direction);
            }
        }
    }
}

void Curtain_HandleTouchLogic(const uint8_t direction)
{
    if(Curtain_areAllSelected()) {
        Curtains_MoveSignalAll(direction);
    } else {
        Curtain_Handle* handle = Curtain_GetByLogicalIndex(curtain_selected);
        if (handle) {
            Curtain_MoveSignal(handle, direction);
        }
    }
}

void Curtain_Update_External(uint16_t relay, uint8_t state)
{
    Curtain_Handle* handle = FindCurtainByRelay(relay);
    if(handle)
    {
        handle->upDown_old = state;
        handle->upDown = state;
        handle->upDownTimer = HAL_GetTick() ? HAL_GetTick() : 1;
        handle->external_cmd = true;
    }
}

// --- Grupa 6: Provjera Stanja ---

bool Curtain_hasRelays(const Curtain_Handle* const handle) { return handle && (handle->config.relayUp != 0 || handle->config.relayDown != 0); }
bool Curtain_isMoving(const Curtain_Handle* const handle) { return handle && (handle->upDown_old != CURTAIN_STOP); }
bool Curtain_isMovingUp(const Curtain_Handle* const handle) { return handle && (handle->upDown_old == CURTAIN_UP); }
bool Curtain_isMovingDown(const Curtain_Handle* const handle) { return handle && (handle->upDown_old == CURTAIN_DOWN); }

bool Curtains_isAnyCurtainMoving(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMoving(&curtains[i])) return true;
    }
    return false;
}

bool Curtains_isAnyCurtainMovingUp(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingUp(&curtains[i])) return true;
    }
    return false;
}

bool Curtains_isAnyCurtainMovingDown(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingDown(&curtains[i])) return true;
    }
    return false;
}

bool Curtains_areAllMovinginSameDirection(const uint8_t direction)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(&curtains[i]) && (curtains[i].upDown_old != direction)) return false;
    }
    return true; // Vraca true ako nema konfigurisanih roletni, što je ispravno ponašanje
}

uint8_t Curtain_getNewDirection(const Curtain_Handle* const handle)
{
    return handle ? handle->upDown : CURTAIN_STOP;
}


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 * @brief Pronalazi roletnu na osnovu Modbus adrese jednog od njenih releja.
 * @param relay Adresa releja.
 * @retval Curtain_Handle* Pointer na pronadenu roletnu, ili `NULL`.
 */
static Curtain_Handle* FindCurtainByRelay(uint16_t relay)
{
    if (relay == 0) return NULL;
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if ((curtains[i].config.relayUp == relay) || (curtains[i].config.relayDown == relay))
        {
            return &curtains[i];
        }
    }
    return NULL;
}


/**
 * @brief Interna funkcija koja prebrojava koliko je roletni konfigurisano.
 */
static void Curtains_CountConfigured(void)
{
    curtains_count = 0;
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if(Curtain_hasRelays(&curtains[i])) {
            curtains_count++;
        }
    }
}


/**
 * @brief Upravlja tajmerom za automatsko zaustavljanje roletne.
 * @param handle Pointer na instancu roletne.
 */
static void HandleCurtainMovement(Curtain_Handle* const handle)
{
    if (handle->upDownTimer != 0 && (HAL_GetTick() - handle->upDownTimer) >= (Curtain_GetMoveTime() * 1000))
    {
        Curtain_Stop(handle);
    }
}

/**
 * @brief Detektuje promjenu smjera i šalje odgovarajucu Modbus komandu.
 * @param handle Pointer na instancu roletne.
 */
static void HandleCurtainDirectionChange(Curtain_Handle* const handle)
{
    if(handle->upDown != handle->upDown_old)
    {
        uint8_t sendDataBuff[3] = {0};
        uint16_t relay = 0;
        uint8_t command = 0;

        // Odredivanje koji relej i komandu treba poslati
        if (handle->upDown == CURTAIN_UP) {
            relay = handle->config.relayUp;
            command = BINARY_ON;
        } else if (handle->upDown == CURTAIN_DOWN) {
            relay = handle->config.relayDown;
            command = BINARY_ON;
        } else { // CURTAIN_STOP
            if(handle->upDown_old == CURTAIN_UP) relay = handle->config.relayUp;
            else if(handle->upDown_old == CURTAIN_DOWN) relay = handle->config.relayDown;
            command = BINARY_OFF;
        }

        if(relay != 0) {
            sendDataBuff[0] = (relay >> 8) & 0xFF;
            sendDataBuff[1] = relay & 0xFF;
            
            // Odabir protokola i slanje komande
            if (handle->config.relayUp != handle->config.relayDown) {
                 sendDataBuff[2] = command;
                 AddCommand(&binaryQueue, BINARY_SET, sendDataBuff, 3);
            } else {
                sendDataBuff[2] = handle->upDown; // Jalousie protokol koristi 0, 1, 2
                AddCommand(&curtainQueue, JALOUSIE_SET, sendDataBuff, 3);
            }
        }

        if(screen == SCREEN_CURTAINS) shouldDrawScreen = 1;

        // Ažuriranje internog stanja
        if(handle->upDown == CURTAIN_STOP) {
            handle->upDown_old = CURTAIN_STOP;
            handle->upDownTimer = 0;
            handle->external_cmd = false;
        } else {
            handle->upDown_old = handle->upDown;
        }
    }
}
