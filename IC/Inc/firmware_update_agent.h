/**
 ******************************************************************************
 * @file    firmware_update_agent.h
 * @author  Gemini
 * @brief   Javni API za modul "Firmware Update Agent".
 *
 * @note
 * Ovaj modul enkapsulira kompletnu logiku za prijem, validaciju i pripremu
 * novog firmvera za aktivaciju od strane bootloadera. Implementiran je kao
 * mašina stanja (State Machine) kako bi se osigurala robusnost i predvidljivo
 * ponašanje, čak i u slučaju grešaka u komunikaciji ili prekida napajanja.
 ******************************************************************************
 */

#ifndef __FIRMWARE_UPDATE_AGENT_H__
#define __FIRMWARE_UPDATE_AGENT_H__

#include "TinyFrame.h"
#include <stdbool.h>

/**
 * @brief Inicijalizuje Firmware Update Agent.
 * @note  Ovu funkciju je potrebno pozvati jednom pri startu sistema, npr. iz main().
 * Postavlja mašinu stanja u početno, IDLE stanje.
 * @param None
 * @retval None
 */
void FwUpdateAgent_Init(void);

/**
 * @brief Glavna servisna funkcija (drajver) za Agent.
 * @note  Ovu funkciju je potrebno pozivati periodično iz glavne `while(1)` petlje u main.c.
 * Odgovorna je za upravljanje internim tajmerima, kao što je timeout
 * zbog neaktivnosti servera tokom transfera.
 * @param None
 * @retval None
 */
void FwUpdateAgent_Service(void);

/**
 * @brief Prosljeđuje dolaznu TinyFrame poruku Agentu na obradu.
 * @note  Ovo je glavna ulazna tačka za sve komande vezane za update. Poziva se
 * iz `FIRMWARE_UPDATE_Listener`-a u `rs485.c`. Agent će obraditi poruku
 * u zavisnosti od svog trenutnog stanja.
 * @param tf    Pokazivač na TinyFrame instancu.
 * @param msg   Pokazivač na primljenu TF_Msg poruku.
 * @retval None
 */
void FwUpdateAgent_ProcessMessage(TinyFrame *tf, TF_Msg *msg);

/**
 * @brief Provjerava da li je proces ažuriranja trenutno u toku.
 * @note  Ovu funkciju koristi `display.c` modul da bi znao da li treba prikazati
 * poruku "Update in progress..." i blokirati GUI.
 * @param None
 * @retval bool `true` ako je ažuriranje aktivno (Agent nije u IDLE stanju), inače `false`.
 */
bool FwUpdateAgent_IsActive(void);

#endif // __FIRMWARE_UPDATE_AGENT_H__
