/**
 ******************************************************************************
 * @file    buzzer.c
 * @author      Gemini & [Vaše Ime]
 * @brief       Implementacija ne-blokirajućeg drajvera za zujalicu sa
 * logikom eskalacije alarma.
 ******************************************************************************
 */
#include "buzzer.h"

// --- KONSTANTE ZA PODEŠAVANJE ALARMA ---
static const uint32_t buzzer_alarm_cycle_pause = 100;     // Pauza između "ti-ti" u ms
static const uint32_t buzzer_alarm_pause_all = 800;       // Pauza između "ti-ti-ti-ti" ciklusa u ms
static const uint32_t buzzer_alarm_start_duration = 5;   // Početna dužina jednog "ti" u ms (najtiše)
static const uint32_t buzzer_alarm_increase_duration = 10;// Koliko se produžava "ti" nakon svakog koraka
static const uint32_t buzzer_alarm_end_duration = 200;    // Maksimalna dužina jednog "ti" u ms (najglasnije)
static const uint8_t  buzzer_alarm_repeat_cycle = 10;     // Broj ciklusa prije pojačavanja
static const uint8_t  buzzer_alarm_repeat_all = 100;       // Ukupan broj "ti-ti-ti-ti" ciklusa prije gašenja (ukupno trajanje)

/**
 * @brief Stanja za internu mašinu stanja (state machine) zujalice.
 */
typedef enum {
    BUZZER_STATE_IDLE,
    BUZZER_STATE_SINGLE_CLICK,
    BUZZER_STATE_ALARM_BEEP,
    BUZZER_STATE_ALARM_CYCLE_PAUSE,
    BUZZER_STATE_ALARM_ALL_PAUSE
} BuzzerState_e;

// Statičke varijable za praćenje stanja
static BuzzerState_e buzzer_state;
static uint32_t state_timer_start;
static uint32_t current_beep_duration;
static uint8_t beep_counter;
static uint8_t cycle_repeat_counter;
static uint8_t all_repeat_counter;

/**
 * @brief Inicijalizuje ili resetuje drajver zujalice u početno, neaktivno stanje.
 */
void Buzzer_Init(void) {
    buzzer_state = BUZZER_STATE_IDLE;
    state_timer_start = 0;
    current_beep_duration = 0;
    beep_counter = 0;
    cycle_repeat_counter = 0;
    all_repeat_counter = 0;
    BuzzerOff();
}

/**
 * @brief Pokreće kompletnu sekvencu eskalacije alarma.
 */
void Buzzer_StartAlarm(void) {
    if (buzzer_state == BUZZER_STATE_IDLE) {
        all_repeat_counter = 0;
        cycle_repeat_counter = 0;
        beep_counter = 0;
        current_beep_duration = buzzer_alarm_start_duration;
        state_timer_start = HAL_GetTick();
        buzzer_state = BUZZER_STATE_ALARM_BEEP;
        BuzzerOn();
    }
}

/**
 * @brief Trenutno zaustavlja alarm ili bilo koji zvuk i resetuje mašinu stanja.
 */
void Buzzer_Stop(void) {
    Buzzer_Init();
}

/**
 * @brief Generiše jedan kratak, ne-blokirajući zvučni signal.
 */
void Buzzer_SingleClick(void) {
    if (buzzer_state == BUZZER_STATE_IDLE) {
        state_timer_start = HAL_GetTick();
        buzzer_state = BUZZER_STATE_SINGLE_CLICK;
        BuzzerOn();
    }
}

/**
 * @brief Glavna servisna funkcija drajvera. Mora se pozivati periodično iz main() petlje.
 */
void Buzzer_Service(void) {
    if (buzzer_state == BUZZER_STATE_IDLE) return;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - state_timer_start;

    switch (buzzer_state) {
        case BUZZER_STATE_SINGLE_CLICK:
            if (elapsed >= 2) {
                Buzzer_Stop();
            }
            break;
            
        case BUZZER_STATE_ALARM_BEEP:
            if (elapsed >= current_beep_duration) {
                BuzzerOff();
                state_timer_start = now;
                beep_counter++;
                if (beep_counter >= 4) {
                    buzzer_state = BUZZER_STATE_ALARM_ALL_PAUSE;
                } else {
                    buzzer_state = BUZZER_STATE_ALARM_CYCLE_PAUSE;
                }
            }
            break;

        case BUZZER_STATE_ALARM_CYCLE_PAUSE:
            if (elapsed >= buzzer_alarm_cycle_pause) {
                BuzzerOn();
                state_timer_start = now;
                buzzer_state = BUZZER_STATE_ALARM_BEEP;
            }
            break;

        case BUZZER_STATE_ALARM_ALL_PAUSE:
            if (elapsed >= buzzer_alarm_pause_all) {
                all_repeat_counter++;
                if (all_repeat_counter >= buzzer_alarm_repeat_all) {
                    Buzzer_Stop();
                    break;
                }
                
                cycle_repeat_counter++;
                if (cycle_repeat_counter >= buzzer_alarm_repeat_cycle) {
                    cycle_repeat_counter = 0;
                    if (current_beep_duration < buzzer_alarm_end_duration) {
                        current_beep_duration += buzzer_alarm_increase_duration;
                        if (current_beep_duration > buzzer_alarm_end_duration) {
                            current_beep_duration = buzzer_alarm_end_duration;
                        }
                    }
                }
                
                beep_counter = 0;
                BuzzerOn();
                state_timer_start = now;
                buzzer_state = BUZZER_STATE_ALARM_BEEP;
            }
            break;

        case BUZZER_STATE_IDLE:
        default:
             break;
    }
}
