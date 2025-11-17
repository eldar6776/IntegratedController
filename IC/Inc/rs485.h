/**
 ******************************************************************************
 * File Name          : rs485.h
 * Date               : 28/02/2016 23:16:19
 * Description        : rs485 communication modul header
 ******************************************************************************
 *
 ******************************************************************************
 */
 
#ifndef __RS485_H__
#define __RS485_H__                         FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include "TinyFrame.h"
/* Exported Define  ----------------------------------------------------------*/
#define COMMAND_QUEUE_SIZE (32)   // Maksimalan broj komandi u redu
#define BINARY_ON          0x01   // Novo stanje za binarni izlaz: UKLJUCENO 
#define BINARY_OFF         0x02   // Novo stanje za binarni izlaz: ISKLJUCENO
/* Exported Type  ------------------------------------------------------------*/
// Definicija komande
typedef struct {
    uint8_t commandType;  // CUSTOM_SET, BINARY_SET, RGBW, CURTAIN...
    uint8_t data[32];     // Maksimalna dužina komande (prilagodi po potrebi)
    uint8_t length;       // Dužina podataka u data[]
} Command;

// Red komandi
typedef struct {
    Command commands[COMMAND_QUEUE_SIZE];
    uint8_t head;  // Pokazivac na prvi neobradeni element
    uint8_t tail;  // Pokazivac na slobodno mjesto za upis
    uint8_t count; // Broj elemenata u redu
} CommandQueue;


typedef struct {
    bool ready;
    uint8_t commandType;
    uint8_t data[COMMAND_QUEUE_SIZE];  // Dovoljno velik bafer za razne GET odgovore
    uint8_t length;
} GetResponseBuffer;
/* Exported variables  -------------------------------------------------------*/
extern uint8_t  rec;
extern uint8_t  tfifa;
extern uint16_t sysid;
extern volatile bool fw_flag;
extern CommandQueue binaryQueue;
extern CommandQueue dimmerQueue;
extern CommandQueue rgbwQueue;
extern CommandQueue curtainQueue;
extern CommandQueue thermoQueue;

/* Exported macros     -------------------------------------------------------*/
#define StartFwUpdate()             fw_flag = true
#define StopFwUpdate()              fw_flag = false
#define IsFwUpdateActiv()           fw_flag == true
/* Exported functions ------------------------------------------------------- */
void RS485_Init(void);
void RS485_Tick(void);
void RS485_Service(void);
void RS485_RxCpltCallback(void);
void RS485_TxCpltCallback(void);
void RS485_ErrorCallback(void);
bool GetState(uint8_t commandType, uint16_t address, uint8_t *response);
bool AddCommand(CommandQueue *queue, uint8_t commandType, uint8_t *data, uint8_t length);
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
