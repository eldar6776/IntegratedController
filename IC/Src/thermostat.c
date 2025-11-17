/**
 ******************************************************************************
 * @file    thermostat.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Modul za upravljanje termostatom sa sakrivenim podacima (enkapsulacija).
 *
 * @note
 * Ovaj fajl sadrži privatnu definiciju strukture termostata i implementaciju
 * svih javnih API funkcija. Koristi staticku instancu strukture kako bi
 * osigurao da postoji samo jedan termostat u sistemu (Singleton pattern).
 ******************************************************************************
 */

#if (__TEMP_CTRL_H__ != FW_BUILD)
#error "thermostat header version mismatch"
#endif

/* Includes ------------------------------------------------------------------*/
/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h" // Uvijek prvi
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "curtain.h"
#include "lights.h"
#include "display.h"
#include "stm32746g_eeprom.h" // Mapa ide nakon svih definicija
#include "rs485.h"

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI                                               */
/*============================================================================*/

/**
 * @brief  Makroi za lokalnu kontrolu ventilatora spojenog na GPIO.
 * @note   Ova logika je sada iskljucivo unutar ovog modula.
 */
#define FanLowSpeedOn()             (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET))
#define FanLowSpeedOff()            (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET))
#define FanMiddleSpeedOn()          (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET))
#define FanMiddleSpeedOff()         (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET))
#define FanHighSpeedOn()            (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_SET))
#define FanHighSpeedOff()           (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_RESET))
#define FanOff()                    (FanLowSpeedOff(),FanMiddleSpeedOff(),FanHighSpeedOff())

/* Flegovi za ntc_flags clan strukture */
#define NTC_CONNECTED_FLAG          (1U << 0)
#define NTC_ERROR_FLAG              (1U << 1)

/*============================================================================*/
/* PRIVATNA DEFINICIJA STRUKTURE                                              */
/*============================================================================*/

// Puna definicija glavne strukture, sakrivena od ostatka programa
struct THERMOSTAT_TypeDef_s {
    THERMOSTAT_EepromConfig_t config; // Podaci koji se cuvaju
    // Runtime podaci:
    uint8_t  th_state;
    int16_t  mv_temp;
    uint8_t  fan_speed;
    bool     hasInfoChanged;
    uint8_t  ntc_flags; // Zamjena za globalnu 'termfl' varijablu
};

/*============================================================================*/
/* PRIVATNA (STATICKA) INSTANCA                                               */
/*============================================================================*/
/**
 * @brief Jedina instanca termostata u cijelom sistemu.
 * @note  Kljucna rijec 'static' je cini vidljivom samo unutar ovog .c fajla.
 */
static THERMOSTAT_TypeDef thst;

/*============================================================================*/
/* IMPLEMENTACIJA JAVNIH FUNKCIJA                                             */
/*============================================================================*/
/**
  * @brief Vraca pointer na jedinu (singleton) instancu termostata.
  */
THERMOSTAT_TypeDef* Thermostat_GetInstance(void) // <-- DODATA IMPLEMENTACIJA
{
    return &thst;
}
/**
  * @brief Inicijalizuje termostat iz EEPROM-a, uz provjeru validnosti podataka.
  */
void THSTAT_Init(THERMOSTAT_TypeDef* const handle)
{
    // Ucitavanje cijelog konfiguracionog bloka iz EEPROM-a.
    EE_ReadBuffer((uint8_t*)&handle->config, EE_THERMOSTAT, sizeof(THERMOSTAT_EepromConfig_t));

    if (handle->config.magic_number != EEPROM_MAGIC_NUMBER) {
        Thermostat_SetDefault(handle);
        THSTAT_Save(handle);
    } else {
        uint16_t received_crc = handle->config.crc;
        handle->config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(THERMOSTAT_EepromConfig_t));

        if (received_crc != calculated_crc) {
            Thermostat_SetDefault(handle);
            THSTAT_Save(handle);
        }
    }

    // Inicijalizacija runtime varijabli.
    handle->hasInfoChanged = false;
    handle->mv_temp = 0;
    handle->fan_speed = 0;
    handle->th_state = 0;
    handle->ntc_flags = 0;

    // Postavljanje pocetnog moda rada.
    //Thermostat_SetControlMode(handle, 2); // Heating
}

/**
  * @brief Cuva kompletnu konfiguraciju termostata u EEPROM, ukljucujuci CRC.
  */
void THSTAT_Save(THERMOSTAT_TypeDef* const handle)
{
    // KORAK 1: Postavljanje magicnog broja kao "potpisa" da su podaci naši.
    handle->config.magic_number = EEPROM_MAGIC_NUMBER;
    // KORAK 2: Privremeno postavljanje CRC polja na 0 radi ispravnog izracuna.
    handle->config.crc = 0;
    // KORAK 3: Izracunavanje CRC-a nad cijelom 'config' strukturom.
    handle->config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(THERMOSTAT_EepromConfig_t));
    // KORAK 4: Snimanje cijelog bloka podataka u EEPROM na definisanu adresu.
    EE_WriteBuffer((uint8_t*)&handle->config, EE_THERMOSTAT, sizeof(THERMOSTAT_EepromConfig_t));
}

/**
 * @brief  Postavlja sve postavke termostata na sigurne fabricke vrijednosti.
 */
void Thermostat_SetDefault(THERMOSTAT_TypeDef* const handle)
{
    memset(handle, 0, sizeof(THERMOSTAT_TypeDef));

    handle->config.group = 0;
    handle->config.master = false;
    handle->config.th_ctrl = 0;
    handle->config.sp_temp = 22;
    handle->config.sp_min = 15;
    handle->config.sp_max = 30;
    handle->config.sp_diff = 5;
    handle->config.mv_offset = 0;
    handle->config.fan_ctrl = 0;
    handle->config.fan_diff = 10;
    handle->config.fan_loband = 10;
    handle->config.fan_hiband = 20;
}
/**
 * @brief  Glavna servisna petlja za termostat.
 * @note   Ova funkcija sadrži logiku za kontrolu grijanja/hladenja i ventilatora.
 * Treba je pozivati periodicno iz glavne `while(1)` petlje.
 * Koristi "handle" za pristup privatnim podacima modula.
 * @param  handle Pointer na instancu termostata dobijen sa Thermostat_GetInstance().
 * @retval None
 */
void THSTAT_Service(THERMOSTAT_TypeDef* const handle)
{
    // Bafer za slanje podataka preko RS485. 15 bajtova je dovoljno za najvecu poruku.
    uint8_t buf[15];

    // Provjera da li termostat radi samostalno (nije dio grupe).
    if (handle->config.group == 0)
    {
        // Staticke varijable za cuvanje stanja izmedu poziva funkcije.
        static int16_t temp_sp = 0U;
        static uint32_t fan_pcnt = 0U;
        static uint32_t old_fan_speed = 0U;
        static uint32_t fancoil_fan_timer = 0U;

        // ============================================================================
        // KONTROLA TEMPERATURE I BRZINE VENTILATORA
        // ============================================================================

        // Ako je regulator ISKLJUCEN (mod 0).
        if (handle->config.th_ctrl == 0) { // Zamjena za makro IsTempRegActiv()
            fan_pcnt = 0U;
            handle->fan_speed = 0U;
            // FanOff(); // Logika za fizicko gašenje ventilatora.
        }
        // Ako je regulator UKLJUCEN (hladenje ili grijanje).
        else
        {
            temp_sp = (int16_t)((handle->config.sp_temp & 0x3FU) * 10);

            // Logika za HLAÐENJE (mod 1).
            if (handle->config.th_ctrl == 1) { // Zamjena za IsTempRegCooling()
                if      ((handle->fan_speed == 0U) && (handle->mv_temp > (temp_sp + handle->config.fan_loband)))                                                    handle->fan_speed = 1U;
                else if ((handle->fan_speed == 1U) && (handle->mv_temp > (temp_sp + handle->config.fan_hiband)))                                                    handle->fan_speed = 2U;
                else if ((handle->fan_speed == 1U) && (handle->mv_temp <= temp_sp))                                                                                  handle->fan_speed = 0U;
                else if ((handle->fan_speed == 2U) && (handle->mv_temp > (temp_sp + handle->config.fan_hiband + handle->config.fan_loband)))                           handle->fan_speed = 3U;
                else if ((handle->fan_speed == 2U) && (handle->mv_temp <=(temp_sp + handle->config.fan_hiband - handle->config.fan_diff)))                             handle->fan_speed = 1U;
                else if ((handle->fan_speed == 3U) && (handle->mv_temp <=(temp_sp + handle->config.fan_hiband + handle->config.fan_loband - handle->config.fan_diff)))    handle->fan_speed = 2U;
            }
            // Logika za GRIJANJE (mod 2).
            else if (handle->config.th_ctrl == 2) // Zamjena za IsTempRegHeating()
            {
                if      ((handle->fan_speed == 0U) && (handle->mv_temp < (temp_sp - handle->config.fan_loband)))                                                    handle->fan_speed = 1U;
                else if ((handle->fan_speed == 1U) && (handle->mv_temp < (temp_sp - handle->config.fan_hiband)))                                                    handle->fan_speed = 2U;
                else if ((handle->fan_speed == 1U) && (handle->mv_temp >= temp_sp))                                                                                  handle->fan_speed = 0U;
                else if ((handle->fan_speed == 2U) && (handle->mv_temp < (temp_sp - handle->config.fan_hiband - handle->config.fan_loband)))                           handle->fan_speed = 3U;
                else if ((handle->fan_speed == 2U) && (handle->mv_temp >=(temp_sp - handle->config.fan_hiband + handle->config.fan_diff)))                             handle->fan_speed = 1U;
                else if ((handle->fan_speed == 3U) && (handle->mv_temp >=(temp_sp - handle->config.fan_hiband - handle->config.fan_loband + handle->config.fan_diff)))    handle->fan_speed = 2U;
            }
        }

        // ============================================================================
        // PREBACIVANJE BRZINA VENTILATORA SA ODGODOM
        // ============================================================================
        if (handle->fan_speed != old_fan_speed)
        {
            if((HAL_GetTick() - fancoil_fan_timer) >= FANC_FAN_MIN_ON_TIME) {
                if(fan_pcnt > 1U)  fan_pcnt = 0U;
                if(fan_pcnt == 0U) {
                    FanOff();
                    if (old_fan_speed) fancoil_fan_timer = HAL_GetTick();
                    ++fan_pcnt;
                }
                else if(fan_pcnt == 1U) {
                    if      (handle->fan_speed == 1) FanLowSpeedOn();
                    else if (handle->fan_speed == 2) FanMiddleSpeedOn();
                    else if (handle->fan_speed == 3) FanHighSpeedOn();
                    if (handle->fan_speed) fancoil_fan_timer = HAL_GetTick();
                    old_fan_speed = handle->fan_speed;
                    ++fan_pcnt;
                }
            }
        }
    } // Kraj `if (handle->config.group == 0)`

    // ============================================================================
    // SLANJE INFO PAKETA NA RS485 BUS
    // ============================================================================
    // Ako je ovaj termostat MASTER i ako je došlo do promjene...
    if(handle->config.master && handle->hasInfoChanged)
    {
        buf[0] = handle->config.group;
        buf[1] = handle->config.master;
        buf[2] = handle->config.th_ctrl;
        buf[3] = handle->th_state;
        buf[4] = (handle->mv_temp >> 8) & 0xFF;
        buf[5] = handle->mv_temp & 0xFF;
        buf[6] = handle->config.sp_temp;
        buf[7] = handle->config.sp_min;
        buf[8] = handle->config.sp_max;
        buf[9] = handle->config.sp_diff;
        buf[10] = handle->fan_speed;
        buf[11] = handle->config.fan_loband;
        buf[12] = handle->config.fan_hiband;
        buf[13] = handle->config.fan_diff;
        buf[14] = handle->config.fan_ctrl;
        // AddCommand(&thermoQueue, THERMOSTAT_INFO, buf, 15); // Otkomentarisati kada RS485 bude spreman
        handle->hasInfoChanged = false; // Resetuj fleg nakon slanja
    }
    // Ako je ovaj termostat SLAVE i ako je došlo do promjene...
    else if(!handle->config.master && handle->hasInfoChanged)
    {
        buf[0] = handle->config.group;
        buf[1] = handle->config.master;
        buf[2] = handle->config.th_ctrl;
        buf[3] = handle->th_state;
        buf[4] = (handle->mv_temp >> 8) & 0xFF;
        buf[5] = handle->mv_temp & 0xFF;
        buf[6] = handle->config.sp_temp;
        // AddCommand(&thermoQueue, THERMOSTAT_INFO, buf, 7); // Otkomentarisati kada RS485 bude spreman
        handle->hasInfoChanged = false; // Resetuj fleg nakon slanja
    }
}

// --- Grupa 2: Kontrola Zadate Temperature (Setpoint) ---

uint8_t Thermostat_GetSetpoint(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.sp_temp;
}

void Thermostat_SP_Temp_Set(THERMOSTAT_TypeDef* const handle, const uint8_t setpoint)
{
    if(handle->config.sp_temp != setpoint) {
        if      (setpoint < handle->config.sp_min) handle->config.sp_temp = handle->config.sp_min;
        else if (setpoint > handle->config.sp_max) handle->config.sp_temp = handle->config.sp_max;
        else                                       handle->config.sp_temp = setpoint;
        handle->hasInfoChanged = true;
    }
}

void Thermostat_SP_Temp_Increment(THERMOSTAT_TypeDef* const handle)
{
    Thermostat_SP_Temp_Set(handle, handle->config.sp_temp + 1);
}

void Thermostat_SP_Temp_Decrement(THERMOSTAT_TypeDef* const handle)
{
    Thermostat_SP_Temp_Set(handle, handle->config.sp_temp - 1);
}

// --- Grupa 3: Glavna Kontrola i Konfiguracija ---
/**
 * @brief  Potpuno iskljucuje termostat i sve povezane funkcije.
 */
void Thermostat_TurnOff(THERMOSTAT_TypeDef* const handle)
{
    // 1. Postavi mod rada na OFF.
    handle->config.th_ctrl = 0;

    // 2. Postavi logicku brzinu ventilatora na 0.
    handle->fan_speed = 0;

    // 3. Pozovi privatni makro da ugasi i fizicke releje ventilatora.
    FanOff();

    // 4. Oznaci da je došlo do promjene stanja, kako bi se poslala info poruka.
    handle->hasInfoChanged = true;
}

uint8_t Thermostat_GetControlMode(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.th_ctrl;
}

void Thermostat_SetControlMode(THERMOSTAT_TypeDef* const handle, uint8_t mode)
{
    handle->config.th_ctrl = mode;
}

uint8_t Thermostat_Get_SP_Max(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.sp_max;
}

void Thermostat_Set_SP_Max(THERMOSTAT_TypeDef* const handle, const uint8_t value)
{
    // Postojeca logika validacije
    if(value <= handle->config.sp_min)       handle->config.sp_max = handle->config.sp_min + 1;
    else if(value > THST_SP_MAX)             handle->config.sp_max = THST_SP_MAX;
    else                                     handle->config.sp_max = value;
}

uint8_t Thermostat_Get_SP_Min(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.sp_min;
}

void Thermostat_Set_SP_Min(THERMOSTAT_TypeDef* const handle, const uint8_t value)
{
    // Postojeca logika validacije
    if(value >= handle->config.sp_max)       handle->config.sp_min = handle->config.sp_max - 1;
    else if(value < THST_SP_MIN)             handle->config.sp_min = THST_SP_MIN;
    else                                     handle->config.sp_min = value;
}

bool Thermostat_IsMaster(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.master;
}

void Thermostat_SetMaster(THERMOSTAT_TypeDef* const handle, bool is_master)
{
    handle->config.master = is_master;
}


// --- Grupa 4: Kontrola Ventilatora (Getteri i Setteri) ---
/**
 * @brief  Dobija prag za prelazak ventilatora na NISKU brzinu.
 */
uint8_t Thermostat_GetFanLowBand(THERMOSTAT_TypeDef* const handle)
{
    // Vraca vrijednost direktno iz privatne config strukture.
    return handle->config.fan_loband;
}

/**
 * @brief  Postavlja prag za prelazak ventilatora na NISKU brzinu.
 */
void Thermostat_SetFanLowBand(THERMOSTAT_TypeDef* const handle, uint8_t value)
{
    // Postavlja novu vrijednost u privatnu config strukturu.
    handle->config.fan_loband = value;
    // Oznacava da je došlo do promjene koja se treba prenijeti na bus.
    handle->hasInfoChanged = true;
}

/**
 * @brief  Dobija prag za prelazak ventilatora na VISOKU brzinu.
 */
uint8_t Thermostat_GetFanHighBand(THERMOSTAT_TypeDef* const handle)
{
    // Vraca vrijednost direktno iz privatne config strukture.
    return handle->config.fan_hiband;
}

/**
 * @brief  Postavlja prag za prelazak ventilatora na VISOKU brzinu.
 */
void Thermostat_SetFanHighBand(THERMOSTAT_TypeDef* const handle, uint8_t value)
{
    // Postavlja novu vrijednost u privatnu config strukturu.
    handle->config.fan_hiband = value;
    // Oznacava da je došlo do promjene koja se treba prenijeti na bus.
    handle->hasInfoChanged = true;
}
uint8_t Thermostat_GetFanDifference(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.fan_diff;
}

void Thermostat_SetFanDifference(THERMOSTAT_TypeDef* const handle, uint8_t value)
{
    handle->config.fan_diff = value;
    handle->hasInfoChanged = true; // Setter automatski postavlja fleg
}

uint8_t Thermostat_GetFanControlMode(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.fan_ctrl;
}

void Thermostat_SetFanControlMode(THERMOSTAT_TypeDef* const handle, uint8_t mode)
{
    handle->config.fan_ctrl = mode;
}


// --- Grupa 5: Ocitavanje Stanja (Runtime Getters) ---
uint8_t Thermostat_GetState(THERMOSTAT_TypeDef* const handle)
{
    return handle->th_state;
}

uint8_t Thermostat_GetSetpointDifference(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.sp_diff;
}

void Thermostat_SetSetpointDifference(THERMOSTAT_TypeDef* const handle, uint8_t value)
{
    if (handle->config.sp_diff != value) {
        handle->config.sp_diff = value;
        handle->hasInfoChanged = true;
    }
}
int16_t Thermostat_GetMeasuredTemp(THERMOSTAT_TypeDef* const handle)
{
    return handle->mv_temp;
}

uint8_t Thermostat_GetFanSpeed(THERMOSTAT_TypeDef* const handle)
{
    return handle->fan_speed;
}

bool Thermostat_IsActive(THERMOSTAT_TypeDef* const handle)
{
    return (handle->config.th_ctrl != 0);
}

bool Thermostat_IsNtcConnected(THERMOSTAT_TypeDef* const handle)
{
    return (handle->ntc_flags & NTC_CONNECTED_FLAG);
}

bool Thermostat_IsNtcError(THERMOSTAT_TypeDef* const handle)
{
    return (handle->ntc_flags & NTC_ERROR_FLAG);
}


// --- Grupa 6: Postavljanje Eksternih Podataka (External Setters) ---
/**
 * @brief  Postavlja vrijednost izmjerene temperature (poziva se iz ADC logike).
 * @note   Ova funkcija je sada "pametna". Sadrži logiku histereze i ona je
 * ta koja odlucuje da li je promjena temperature dovoljno znacajna da
 * se ažurira interno stanje i obavijeste GUI i mreža.
 * @param  handle Pointer na instancu termostata.
 * @param  temp Nova izmjerena vrijednost temperature (vrijednost x10, npr. 235 za 23.5°C).
 * @retval None
 */
void Thermostat_SetMeasuredTemp(THERMOSTAT_TypeDef* const handle, int16_t temp)
{
    // Histereza je sada centralizovana unutar ovog modula.
    // Provjeravamo da li je apsolutna razlika izmedu stare i nove temperature
    // veca od 2 (što odgovara 0.2°C, jer su vrijednosti pomnožene sa 10).
    if (abs(handle->mv_temp - temp) > 2)
    {
        // Ako je promjena znacajna, ažuriramo internu vrijednost temperature.
        handle->mv_temp = temp;

        // Podižemo interni fleg koji `THSTAT_Service` koristi da zna da treba
        // poslati info paket na RS485 mrežu.
        handle->hasInfoChanged = true;

        // Podižemo globalni fleg koji `display.c` koristi da zna da treba
        // osvježiti prikaz izmjerene temperature na ekranu.
        MVUpdateSet();
    }
}

/**
 * @brief  Ažurira status NTC senzora (poziva se iz ADC logike).
 * @note   Ova funkcija enkapsulira rad sa internim `ntc_flags` flegom.
 * @param  handle Pointer na instancu termostata.
 * @param  is_connected Da li je senzor konektovan.
 * @param  has_error Da li postoji greška.
 * @retval None
 */
void Thermostat_SetNtcStatus(THERMOSTAT_TypeDef* const handle, bool is_connected, bool has_error)
{
    // Ažuriranje flega za konekciju koristeci bitwise operacije.
    if (is_connected) {
        handle->ntc_flags |= NTC_CONNECTED_FLAG;
    } else {
        handle->ntc_flags &= ~NTC_CONNECTED_FLAG;
    }

    // Ažuriranje flega za grešku.
    if (has_error) {
        handle->ntc_flags |= NTC_ERROR_FLAG;
    } else {
        handle->ntc_flags &= ~NTC_ERROR_FLAG;
    }
}

uint8_t Thermostat_GetGroup(THERMOSTAT_TypeDef* const handle)
{
    return handle->config.group;
}


void Thermostat_SetGroup(THERMOSTAT_TypeDef* const handle, uint8_t value)
{
    handle->config.group = value;
    handle->hasInfoChanged = true;
}

void Thermostat_SetInfoChanged(THERMOSTAT_TypeDef* const handle, bool state)
{
    handle->hasInfoChanged = state;
}
/******************************   RAZLAZ SIJELA  ********************************/
