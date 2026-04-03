# FSD: Outdoor Lights (Razno i Vanjska Rasvjeta) & ALL OFF funkcija

## 1. Opis Funkcionalnosti
Centralizovana funkcionalnost i modul koji obuhvata do maksimalno 20 vanjskih svjetala / zajedničkih potrošača i brzu reakciju gašenja ("All Off"). Outdoor Lights integriše ON/OFF funkcionalnost u kombinaciji sa RTC predefinisanim tajmerima kroz perzistentnu listu i masku aktivnosti.

## 2. Ponašanje i Interakcija (Korisnički Interfejs)
Ova funkcija može da se javi u dva međusobno uslovljena moda u dropdown menijima modova zaduženih za ikonicu 4 na početnom ekranu `SELECT_SCREEN_1/2`.
- Opcija 1: Ikona `ALL_OFF`. Samo click dugme koje proizvodi efikasan prekid (gašenje svih uvezanih adresa). Ne posjeduje navigaciju kroz dugi pritisak.
- Opcija 2: Ikona `OUTDOOR`. Ponaša se dvostruko. Kratki klik komanduje sa "Toggle" akcijom svih svjetala konfiguriranih kroz modul. 
- Dugi pritisak na `OUTDOOR` ikonicu korisnika usmjerava prema "Outdoor rasvjeta Tajmer" meniju.
- Sadrži dva bloka podešavanja (Uključivanje u T1 H/M vrijeme) i (Isključivanje H/M vrijeme). Povezano sa maskama navedenih željenih dan-u-sedmici maski, veoma identično funkcionisanju običnog alarma timera.
- `IsRtcTimeValid()` postavlja prioritet (ne dozvoljava pristup i vrši preusmjeravanje sa porukom ako lokalni sat nije podešen).

## 3. Podešavanja (Settings Meni)
Dizajnerski predviđen Ekran `SCREEN_SETTINGS_OUTDOOR` nudi listu unosa (list widget i spinbox opcije).
- Indeks array-a stavke (1-20).
- RS485 Adresa svjetla za pridruživanje stavki sa indeksom.
- Tip komunikacije (ON/OFF binary, dimmer, rgb command mode array index). - Napomena, samo se izvršavaju komande isključi ili uključi, iako moduli trpe promjene opisa dimming/rgb komandnog stanja.
- Brisanja adresa / dodavanja adresa i spremanja preko ugrađene provjere na EEPROM. 

## 4. Tehnološke Specifikacije i Podaci
Struktura pod nazivom `OutdoorLight_t` pripada listi od 20 u `Outdoor_EepromConfig_t`. Dodatne vrijednosti unutar strukture pokrivaju `trigger` flag "Da li je aktivan", sate i minute On funkcije, isključenja i `repeatMask`.
`last_checked_minute` se implementira unutar `Outdoor_Service()` koji obavlja polling RTC baze svake minute provjeravajući uvijet ispunjivosti sa trenutnim minutama vremena.
Ukoliko se aktivira Toggle mode pošto svi mogu imati offset stanje, koristi se bool statusni registar modula kako bi izbacio kontra-stanje prilikom grupne razmjene RS485 komandi (`Outdoor_ToggleAll()`). 

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`IC/Src`, `IC/Inc`):**
1. **Zatečeno Backend Stanje**: Potvrđeno skeniranjem repozitorija - fajlovi za procesiranje i upravljanje Outdoor elementima (`outdoor.c`, strukture timera za `OutdoorLight_t`) uopšte ne postoje u backendu, niti su razvijeni! Ovo je za sada samo arhitektonska fantazija i konceptualna naracija zapisana za budući rad.
2. **Novi Zahtjevi i Gap Analiza (BLOKATORI)**:
   - Sva navedena EEPROM struktura, tajmeri, RTC latched polling koji se navodi u specifikacijama će morati biti ručno implementirani, a za sada preskačemo ovaj fajl iz integracije dok se ne pojavi stvarna potreba i ne odradi arhitektonski kostur iz "nule".
   - Funkcija `ALL_OFF` koja se asimilira u ovaj domen nema ni backenda, dok `SCENE_LEAVING` ima backend. Da bi se napravio pravi Macro All-off, radiće se prepis na neki `macro.c` ili implementacija unutar outdoor backenda u budućnosti.
3. **Frontend**: Postoje tragovi animacija ili priprema menija kroz `display.c` (oblik ekrana i ikonica), ali sve visi u zraku jer nema C koda na kom bi funkcionisali getteri. Modul je arhiviran pod "Neiskodirano."
