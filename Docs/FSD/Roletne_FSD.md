# FSD: Roletne Modul

## 1. Opis Funkcionalnosti
Roletne modul pruža individualni i zbirni menadžment motorima za pokretanje roletni i zavjesa. On omogućava ručnu kontrolu svih roletni u prostoru kao i složenu implementaciju vremenskog upravljanja u kontekstu SCENA interfejsa koristeći "Čarobnjaka".

## 2. Ponašanje i Interakcija (Korisnički Interfejs)
### 2.1 Panel pojedinačne roletne
- Fokus: Jednostavno `UP` i `DOWN` pomjeranje motora, te dugme zaustavljanja na pola ciklusa.
- Centralno informisanje na bazi "Poštenog UI statusa": Prikazi podignute, spuštene ili poluotvorene logike - na bazi završenosti procesa do kraja zadanih tajmera. U svim situacijama prekida, stanje se vodi pod nepoznatim (poluotvorenim).

### 2.2 Panel za kontrolu "SVE ROLETNE" (Grupna Kontrola)
Ovaj panel adresira kritični UX fenomen mješovitog pre-konfigurisanog stanja fizičkih motora.
- Komande `UP` i `DOWN` su izričite "Broadcast" komade roletnama ove sub-mreže.
- Ukoliko se iskoristi akcija "SVE ROLETNE UP", apsolutno sve roletne koje su ispod krajnje pozicije biće aktivirane da idu nagore, a na panelu UI "animacija mješovitog pomicanja" strelicama ilustruje trenutno stanje u kojem se fizički odvija pomicanje mješovitog statusa u uniformni zadani smjer pokretanja.

## 3. Podešavanja ("Čarobnjak za Scene")
U svrhu pamćenja stanja, unutar funkcije Scena koristi se čarobnjak.
- Pokreće sve odabrane roletne apsolutno do gornjeg, nultog stanja ispisujući `MOLIM SAČEKAJTE`.
- Nakon postizanja pozicije 0 - Starta se poseban `curtains_wiz_start_time` i pozivaju prepreke za softversko mjerenje manualne operacije iscrtavanja roletni u novu "scene friendly" poziciju.
- Pritiskom na 'Stop' mjeri se proteklo vrijeme pomjeranja (u sekundama) koje se pakuje u perzistentni EEPROM array `curtain_timers`.

## 4. Tehnološke Specifikacije i Podaci
### 4.1 Backend
- Roletne koriste array `curtain_timers` pohranjen unutar opšte strukture scene u kojoj se memoriše broj sekundi provedenih u down kretanju.
- Zbirne maskari bitmask `curtains_mask`.

### 4.2 RS485 Operativni Sistem
- Kroz RS485 funkcija `Curtain_Move(handle, CURTAIN_DOWN)` inicira fizički rad releja.
- Tajmer broji i finalno operativni nalog šalje prekid kretanja usljed postizanja izmjerenog vremena (`Curtain_Stop(handle)`).

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`curtain.c` / `curtain.h`):**
1. **Zatečeno Backend Stanje**: Trenutna verzija kôda je besprijekorna implementacija Opaque logike tajmerisanih "glupih" motora. `Curtain_Service` koristi non-blocking prolaz kroz limit kretanja zapisan globalno u `curtains_eeprom_data.upDownDurationSeconds`. Postoji trag Jalousie protokola kao _fallback_ (`handle->config.relayUp.tf == handle->config.relayDown.tf`).
2. **Novi Zahtjevi i Gap Analiza**:
   - **Scene Integacija za Obične Roletne**: Da bi uveli module glupih roletni ("CURTAIN") u "Scene", kôd se mora doraditi da pamti relativne nulte lokacije i vremenska kašnjenja.
   - **Smart Jalousie Integracija**: Modul roletni se mora asinhrono proširiti i razdvojiti! Nova arhitektura će predviđati poseban C modul imenom `JALOUSIE` koji će rukovati isključivo apsolutnom procentualnom logikom pametnih motora (pozitivni feedback statusi sa mreže, Modbus set_position komande), oslabađajući IC od tajmerskih opterećenja.
   - Dosadašnji Opaque modul (`curtain.c`) će se zadržati onakav kakav jeste prvenstveno za podršku sistemima sa bazičnim motorima.
3. **Frontend (Čeka Analizu)**: Tražimo pozive i prozor čarobnjaka kroz `display.c`.
