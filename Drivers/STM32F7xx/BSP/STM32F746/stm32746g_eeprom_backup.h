/**
  ******************************************************************************
  * @file    stm32746g_discovery_eeprom.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for 
  *          the stm32746g_discovery_eeprom.c firmware driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H__                        
#define __EEPROM_H__                        FW_BUILD // version

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32746g.h"


/* EEPROM hardware address and page size */ 
#define EE_PGSIZE                           64
#define EE_PGNUM                            0x100  // number of pages
#define EE_MAXSIZE                          0x4000 /* 64Kbit */
#define EE_ENDADDR                          0x3FFF    
#define EE_ADDR                             0xA0
#define EETOUT                              1000
#define EE_WR_TIME                          15
#define EE_TRIALS                           200
#define EE_MAX_TRIALS                       3000
#define EE_OK                               0x0
#define EE_FAIL                             0x1
#define EE_TOUT                             0x2
#define EE_MARKER                           0x55

#define EE_TERMFL                           0x0    // first group of system flags
#define EE_MIN_SETPOINT                     0x4
#define EE_MAX_SETPOINT                     0x5
#define EE_THST_SETPOINT                    0x6
#define EE_NTC_OFFSET                       0x7
#define EE_DISP_LOW_BCKLGHT                 0xA
#define EE_DISP_HIGH_BCKLGHT                0xB
#define EE_SCRNSVR_TOUT                     0xC
#define EE_SCRNSVR_ENABLE_HOUR              0xD
#define EE_SCRNSVR_DISABLE_HOUR             0xE
#define EE_SCRNSVR_CLK_COLOR                0xF
#define EE_SCRNSVR_ON_OFF                   0x10    
#define EE_SYS_STATE                        0x11
#define EE_FW_UPDATE_BYTE_CNT               0x14	// firmware update byte count
#define EE_FW_UPDATE_STATUS                 0x18	// firmware update status
#define EE_ROOM_TEMP_SP                     0x19	// room setpoint temp  in degree of Celsious
#define EE_ROOM_TEMP_DIFF		            0x1B	// room tempreature on / off difference
#define EE_TFIFA			                0x20	// tinyframe interface address
#define EE_TFGRA				            0x21	// tinyframe group  address
#define EE_TFBRA			                0x22	// tinyframe broadcast address
#define EE_TFGWA                            0x23    // tinyframe gateway address
#define EE_TFBPS					        0x24	// tinyframe interface baudrate
#define EE_SYSID				            0x28	// system id (system unique number)
#define EE_pas0				                0x28	// password 0
#define EE_BLIND_TOUT                       0x2A
#define EE_CTRL1                            0x30
#define EE_CTRL2                            0x50
#define EE_THST1                            0x70
#define EE_INIT_ADDR                        0x90    // value 0xA5U written if default value written

#define EE_spnbxTest                        0x91
#define EE_spnbxTest1                       0x92
#define EE_spnbxTest2                       0x93
#define EE_spnbxTest3                       0x94
#define EE_spnbxTest4                       0x95
#define EE_spnbxTest5                       0x96

#define EE_spnbxLight1                      0x97
#define EE_spnbxLight2                      0x98
#define EE_spnbxLight3                      0x99
#define EE_spnbxLight4                      0x9A
#define EE_spnbxLight5                      0x9B
#define EE_spnbxLight6                      0x9C
#define EE_spnbxLight7                      0x9D
#define EE_spnbxLight8                      0x9E

#define EE_spnbxUlaz                        0x9F
#define EE_spnbxSrednja                     0xA0
#define EE_spnbxSecurity                    0xA1
#define EE_spnbxProlaz                      0xA2
#define EE_spnbxGaraza                      0xA3

#define EE_spnbxAlarm1                      0xA4
#define EE_spnbxAlarm2                      0xA5
#define EE_spnbxAlarm3                      0xA6

#define EE_settingsPassword                 0xA7
#define EE_secAlarmPassword                 0xA8

#define EE_CurtainsUp1                      0xA9
#define EE_CurtainsDown1                    0xAA
#define EE_CurtainsUp3                      0xAB
#define EE_CurtainsDown3                    0xAC
#define EE_MusicPlay                        0xAD
#define EE_MusicUp                          0xAE
#define EE_MusicDown                        0xAF

#define EE_ShutdownTime                     0xB0

#define EE_CurtainsUp2                      0xB1
#define EE_CurtainsDown2                    0xB2

#define EE_LightSelectScreen                0xB3
#define EE_SOS                              0xB4

#define EE_CurtainsUp4                      0xB5
#define EE_CurtainsDown4                    0xB6

#define EE_PoliceLight                      0xB7
#define EE_VinotekaLight                    0xB8
#define EE_VinotekaPoliceLight              0xB9
#define EE_ClosetLight                      0xBA
#define EE_LedHallway                       0xBB
#define EE_Lighting6                        0xBC
#define EE_Lighting7                        0xBD
#define EE_Lighting8                        0xBE
#define EE_PoliceDimm                       0xBF
#define EE_VinotekaDimm                     0xC0
#define EE_VinotekaPoliceDimm               0xC1
#define EE_LightingDimm4                    0xC2
#define EE_LightingDimm5                    0xC3
#define EE_LightingDimm6                    0xC4
#define EE_LightingDimm7                    0xC5
#define EE_LightingDimm8                    0xC6
#define EE_Lighting9                        0xC7
#define EE_Lighting10                       0xC8
#define EE_LED                              0xC9
#define EE_WallLamp                         0xCA
#define EE_LightingDimm9                    0xCB
#define EE_LightingDimm10                   0xCC
#define EE_LightingDimm11                   0xCD
#define EE_LightingDimm12                   0xCE
#define EE_Lighting1                        0xD0
#define EE_Lighting2                        0xD1
#define EE_Lighting3                        0xD2
#define EE_Lighting4                        0xD3
#define EE_Lighting5                        0xD4
#define EE_LightingDimm1                    0xD5
#define EE_LightingDimm2                    0xD6
#define EE_LightingDimm3                    0xD7
#define EE_Lighting11                       0xD8
#define EE_Lighting12                       0xD9

#define EE_SleepTime                        0xDA



#define EE_LightingLivingRoomShelves            EE_PoliceLight
#define EE_LightingDimmLivingRoom               EE_PoliceDimm
#define EE_LightingWineCellarShelves            EE_VinotekaLight
#define EE_LightingDimmWineCellar               EE_VinotekaDimm

#define EE_LightingDimmEntrance                 EE_LightingDimm1
#define EE_LightingDimmMirror                   EE_LightingDimm2
#define EE_LightingDimmShower                   EE_LightingDimm3
#define EE_LightingDimmToilet                   EE_LightingDimm4


#define EE_THST_CTRL_OLD                        EE_PoliceLight

#define EE_SelectScreenDimm_to_MainDimm         EE_spnbxUlaz

#define EE_VentilatorOnTime                     EE_PoliceLight

#define EE_PoolHydMassage                       EE_LightingLivingRoomShelves
#define EE_PoolWaterfall                        EE_LightingDimmLivingRoom
#define EE_LightPool                            EE_LightingWineCellarShelves

#define EE_PairedDeviceID                       EE_LightingWineCellarShelves

/* Link function for I2C EEPROM peripheral */
void     EE_Init         (void);
uint32_t EE_ReadBuffer   (uint8_t *pBuffer, uint16_t ReadAddr,  uint16_t NumByteToRead);
uint32_t EE_WriteBuffer  (uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);


#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_H__ */

/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
