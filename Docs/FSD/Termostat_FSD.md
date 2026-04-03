# FSD: Termostat Modul

## 1. Opis Funkcionalnosti
Termostat modul predstavlja "srce" komforne kontrole i najsofisticiraniji je element mikrokontrolera. Dizajniran je za kontrolu dvije funkcionalnosti (Grijanje, Hlađenje) sa nezavisnim konfiguracijama krugova, uključivanjem pumpi pod odgodom, te različitih metoda i lokacija upravljanja ventilatorom u fancoil/klima sistemima.

## 2. Ponašanje i Pametna Kontrola
Ponašanje funkcioniše na bazi kompleksne **Državne mašine (State Machine)** termostata:
- **Grijanje**: 
  - `IDLE` -> `STARTING_HEATING`: Otvaranje ventila grijanja, start tajmera za uključivanje pumpi.
  - `STARTING_HEATING` -> `ACTIVE_HEATING`: Fizičko paljenje pumpi tek nakon isteka zadanog tajmera.
- **Hlađenje**:
  - `IDLE` -> `STARTING_COOLING`: Otvaranje ventila hlađenja i čekanje na odgodu ventilatora.
  - `STARTING_COOLING` -> `ACTIVE_COOLING`: Očekivano, ventilator radi na odabranoj brzini (1/2/3 ili PWM).
  - `STOPPING_COOLING`: Prvo se zatvara ventil hlađenja, ali ventilator nastavlja raditi X sekundi prije ponovnog vraćanja u `IDLE` boks kako bi se isušen isparivač (blowdown).

## 3. Podešavanja (Settings Meni)
Meni za termostat je podijeljen u 2 ekrana radi jasnoće interfejsa.
- **Ekran 1 (Logika i Tip Kontrole)**:
  - Odabir tipa uloge u sistemu: [x] Master, [x] Slave.
  - Odabir senzora: [x] Koristi Ekst. Senzor.
  - Odabir ventilatora (Fancoil tip): None, ON_OFF, 3_SPEED, PWM_BLDC.
  - Lokacija kontrole ventilatora: Lokalna/Bus.
  - Univerzalni spinboksi čije se labele mijenjaju ovisno o tipu ventilatora (histereze / prop. opseg).
- **Ekran 2 (Hardverske Postavke)**:
  - Adresno mapiranje (za krugove grijanja i hlađenja, pumpe, pomoćni releji stanja/režima rada).
  - Kaskadni spinboxi za odabir pinova ukoliko je kontrola lokalna, ili RS485 adresa ukoliko je odabrana kontrola putem Bus-a.

## 4. Tehnološke Specifikacije i Podaci
### 4.1 Backend (Strukture)
EEPROM struktura `THERMOSTAT_EepromConfig_t` zadržava detaljne postavke kao i logiku dvaju univerzalnih tajmera spojenih posredstvom `ThermostatState_e` runtime stanja uređaja.

### 4.2 RS485 i Eksterni Senzori
- **Konflikt menadžment**: Ako uređaj postavljen kao `Master` i uključen mu je flag `use_external_sensor`, zanemaruje lokalni on-board NTC senzor temperature na svom ADC portu. 
- Prihvata temperaturne podatke sa bus-a od eksternog senzora isključivo označenog flagom `master_flag == 2`. Moduli degradirani na master statusa prate samo `master_flag == 1`.

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`thermostat.c` / `thermostat.h`):**
1. **Zračenje dizajna**: Kod genijalno odvaja runtime stanje od Opaque Pointer strukture `THERMOSTAT_TypeDef_s`. Master/Slave grupisanje je efikasno i osigurava minimalan saobraćaj zahvaljujući `hasInfoChanged` flagu koji `THSTAT_Service` šalje na obradu `AddCommand()` u RS485 redu čekanja. Sam Master samostalno obrađuje NTC ADC `Thermostat_SetMeasuredTemp` pozive sa pragom odobrene minimalne promjene histereze.
2. **Gap Analiza / Nedostaje**: 
   - C kôd posjeduje 3-speed kontrolu i timer limit na fan releje (`FANC_FAN_MIN_ON_TIME`), ali **ne postoje** definicije dodatnih odgoda za *"cirkulacione pumpe"*, niti parametri za *"podno grijanje i ventile"*.
   - Potrebno je modifikovati `THERMOSTAT_EepromConfig_t` kako bi se dodalo definisanje PWM/Fan ponašanja te dodalo upravljanje vanjskim senzorima ukoliko želimo 100% poštovati "Plan Termostat".
3. **Frontend (Čeka Analizu)**: U sklopu analize `display.c` utvrdit ćemo kako ovaj Opaque instancirani model baca podatke na ekran (posebno UI za odgodu ventila i ventilatora).
