# FSD: Modul Pametni Alarm / Timer (Buđenje / Akcije)

## 1. Opis Funkcionalnosti
Logički nezavisan RTC Timer sistem ("Pametni Alarm") orijentisan prema automatskom aktiviranju opštih modbus akcija unutar sistema kućne opreme. Konfigurira se zasebno i nije povezano na scene (može buditi korisnike i aktivirati scene kao svoju izvršnu funkciju).

## 2. Ponašanje i Pametna Kontrola
Prikaz je uslovljen provjerom lokalnog vremena MCU-a.
Ako `IsRtcTimeValid()` postavi false, korisnik biva blokiran da postavlja tajmer. Mora uspostaviti ispravno vrijeme/datum sistema. 
Kada sat poklopi parametre uključujući i ponavljanje po bitmaski unutar sedmice, tajmer aktivira svoju obradu. 

## 3. Podešavanja (Settings Meni)
Dizajnerski predviđen ekran `SCREEN_SETTINGS_TIMER` postavlja odabire:
- Sate i Minute alarma (2 spinboxa)
- Radio odabir (Pop-out checkboxi za 'Ručno', odnosno bitmaskiranje dana `repeatMask`)
- `Zujalica` (On/Off checkbox - aktivacija lokalnog Buzzer-a za alarmnu ton funkciju)
- Služenje Scena karusel elementom (ili `Nijedna`). Pokreće predefinisan dizajn.

## 4. Tehnološke Specifikacije i Podaci
Cijela logika obvija se sa strukturom `Timer_EepromConfig_t`.
`hasTriggeredThisMinute` flag se re-armira sa promjenom RTC minutnog tick-a unutar glavne service metode. 

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`timer.c` / `timer.h`):**
1. **Zatečeno Backend Stanje**: Plan pametnog tajmera opisan u arhivskim fajlovima se podudara u piksel s iskodiranim stanjem. MCU na bazi `HAL_RTC_GetTime` komandira pametnim interfejsom bez redundancije zahvaljujući efikasnom Latch timeru koji ne dozvoljava prekomjeran broj procesiranja u istoj sekundi poklapanja vremena alarma.
2. **Novi Zahtjevi i Gap Analiza**: 
   - Modul ne posjeduje probleme u zadatoj logici i ne zahtijeva refaktorisanje. 
   - Sistem maskiranja (`TIMER_WEEKDAYS`, itd.) i aktiviranja predefinisanih pametnih Scena je prisutan i implementiran kroz `Scene_Activate()`.
3. **Frontend (Čeka Analizu)**: U displej koodu moramo konfirmirati ispravno preslikavanje bitmaski unutar čarobnjaka i pojavu `SCREEN_ALARM_ACTIVE` ekrana ukoliko on mora postojati za vrijeme buđenja.
