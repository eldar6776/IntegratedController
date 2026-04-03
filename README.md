# Integrated Controller (Pametna Vila)

**Integrated Controller (IC)** predstavlja napredni, više-modularni sistem kontrolera osmišljen za pametne vile i zgrade. Baziran na **STM32F746** hardveru, ovaj repozitorij pruža centralizovanu logiku za grafički interfejs osjetljiv na dodir i izrazito robustan RS485/Modbus interfejs za komunikaciju sa pametnim aktuatorima.

---

## 🏗 Arhitektura i Kodni Dizajn

Pisan na čistom C programskom jeziku uz FreeRTOS scheduler, projekat se striktno pridržava "Best-Practice" obrazaca:
- **Opaque Pointers**: Većina glavnih modula (npr. Termostat, Lights, Kapije) koriste neprozirne pokazivače (Handle-ove), apsolutno skrivajući privatne _struct_ podatke unutar njihovih izvršnih `.c` datoteka i osiguravajući komunikaciju preko striktnih API funkcija (`_GetState`, `_Update...`).
- **RAM Defragmentacija & Polling**: Moduli poput `lights.c` posjeduju vlastite sisteme dinamične defragmentacije RAM-a tokom brisanja članova, čime se sprječavaju curenja memorije STemWin biblioteke.
- **EEPROM Zaštita (Wear-Leveling)**: Sistemske postavke, stanja PWM-ova, Timeri i konfiguracije se pakuju u `#pragma pack(1)` strukture. EEPROM pisanja se odgađaju (npr. čekanje 5 sekundi nakon promjene RGB slajdera prije trajnog spremanja u memoriju) uz dodatne _Magic Number_ i _CRC_ provjere integriteta.

---

## 📦 Pregled Ključnih Modula (Backend)

Sistem posjeduje više logičkih cjelina čija detaljna State-Machine logika (FSD Gap Analiza) leži proknjižena u direktoiju `/Docs/FSD/`:

1. **Lights (Rasvjeta - Hibridni State Machine)**
   Upravlja sinhronizacijom tri domene: Direktnih STM32 lokalnih pinova, I2C ekspandera, i RS485 queue-a. Opremljen je tajmerima odgode i eksternim interaktivnim senzorima hardvera.
2. **Termostat**
   Sistem "Master-Slave" termostata sposoban da propagira informacije s jednog fizičkog senzora kroz mrežu prema 'glupim' termostatima unutar iste grupe prostorija. Posjeduje parametre histereze, hlađenja, PWM kontrole i brzine ventilatora.
3. **Gate (Kapije) i USM (Univerzalna State Mašina)**
   Moćni modul koji upravlja motorima. Koncept baziran na postojanju **Deskriptora profila**, koji dozvoljava jedinstvenoj State Mašini da preskače besmislene faze za glupe elektro-brave, dok procesira cjelokupan set "Cikličnih, Pedestrian i Time-Out" faza za ogromne klizne portale.
4. **Alarmi, Timeri i Scene**
   Razdvojeni moduli koji pokrivaju pametni kućni Alarm sa logikom particija i PIN zaključavanjem, RTC kalendarske tajmere sa 'Latch' barijerama koje sprječavaju trigerovanje dvaput u jednoj minuti, i scenske mehanizme.

---

## 📡 Transport i Umrežavanje

Svi moduli sa vanjskim svijetom operiraju mrežom preko fajla `rs485.c` koji se oslanja na async **TinyFrame** okvir. 
Mrežni zagušivači su izbjegnuti upotrebom komandnih redova (`binaryQueue`, `dimmerQueue`, itd.). TinyFrame **Type Listeners** (Listen for DIMMER_SET, BINARY_SET) osiguravaju da hardver momentalno proslijedi API instrukciju modulima za sinhronizaciju bez obzira s koje tačke komunikacija kreće.

---

## 🖥 GUI i Ekran (U tranzicionoj Fazi)

Trenutno vizuelno jezgro bazirano je na impresivnoj instanci emWin STemWin biblioteke koncentrisane u `display.c` modulu (12K linija). Operativni model ekrana radi po `DSP_Init(...)` -> `Service_(...)` -> `DSP_Kill(...)` principu zasnovanom na event triggerima sa displeja bez RTOS zadiranja.

**Plan (Trenutno Aktuelno)**: U toku je fragmentacija i refakorizacija GUI-a u pod-fajlove (`GUI_Core.c`, `GUI_Settings.c`, `GUI_Lights.c` itd.) u cilju zadržavanja performansi a smanjivanja fajla de-monolizitacijom radi mogućnosti laganog implementiranja pod-ekrana poput budućih Carobnjaka za uspostavu automatizacije scena.

---

> Projektna dokumentacija generisana za prelazak iz Prototype (Archive tekstova) faze u profesionalnu Production fazu!
