# FSD: Alarm Modul

## 1. Opis Funkcionalnosti
Modul za kontrolu alarma služi za upravljanje sigurnosnim sistemom kuće koristeći PIN autentifikaciju. Omogućava kontrolu pojedinačnih particija (Perimetar, Prizemlje, Sprat) i cjelokupnog sistema, sa podrškom za globalne sistemske scene ("Sleep", "Leaving", "Homecoming"). Ovaj sistem mora podržavati "Stay Arm / Interior Follower" mod logiku na samoj alarmnoj centrali.

## 2. Ponašanje i Interakcija (Korisnički Interfejs)
- **Ekran za odabir i status (SCREEN_SECURITY)**: Prikazuje se klikom na "ALARM".
- Dinamički iscrtava kontrole u zavisnosti od stvarnog stanja particija (čitanje sa digitalnog input modula na bus-u prije otvaranja menija) umjesto korištenja override (tipkovnicom) stanja.
- Prikazana su dugmad sa nazivima particija (koji mogu biti sistemski predefinisani tipa `Particija 1` ili *custom* iz EEPROM).
- Status je "toggle" taster (ako je on_status=ARM, nudi se DISARM, i obrnuto).
- **Proces Autentifikacije**:
  - Klik na ARM/DISARM dugme prebacuje korisnika na numpad (SCREEN_NUMPAD) za unos globalnog `system_pin`.
  - Upravljanje logikom se obavlja sa dugim pritiskom na "Settings/ALARM" ikonicu na navigacionim ekranima. Dugi pritisak vodi u `SCREEN_SETTINGS_ALARM`.

## 3. Podešavanja (Settings Meni)
- **Modifikacija PIN-a**: Trostupanjski proces promjene "System PIN-a" (Unos starog -> Novi PIN -> Potvrda novog).
- **Postavke Particija**: Spinbox-ovi za definisanje kontrolnih adresa komunikacije i adresa za povratni feedback statusa svake particije.
- **Tip Kontrole (Pulse/Labeled)**: Spinbox za definisanje dužine pulsa (Momentary/Toggle vs. Latched/Maintained).
- **Silent Alarm (SOS)**: Zasebna adresa za tihu dojavu alarma koja se može trigerovati iz GUI-a.
- **Nazivi Particija**: Zadržana je struktura višestrukih stringova (`system_name[21]` i `partition_names[3][21]`).

## 4. Tehnološke Specifikacije i Podaci
### 4.1 Backend (Strukture)
- Prelazak sa više user PIN-ova na jedan **Globalni `system_pin`**.
- Modifikacija u `Security_Settings_t`:
  - Izbaciti strukturu za više korisnika.
  - Dodati stringove za custom imena particija i sistema.
- Validacija šifre se provjerava funkcijom `Security_ValidateUserCode` upoređivanjem sa globalnim pinom.

### 4.2 RS485 Komunikacija
- Alarmi modul direktno adresira odgovarajuće releje i prima feedback sa modula digitalnih ulaza instaliranih u alarmni panel.
- Podržava i integraciju u obliku broadcast poruka tipa `DEVICE_FAULT` (za sigurne greške).

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`security.c` / `security.h`):**
- **Backend (Odobren i Spreman)**: Struktura uspješno inkorporira `system_name[21]`, `partition_names[3][21]`, jedan globalni `pin` te podržava CRC EEPROM validaciju. Višestruki korisnici iz RAM memorije su u potpunosti opozvani i `Security_ValidateUserCode` koristi poređenje direktno sa `g_security_settings.pin`. Ostavljen je jedino "prototip" junk funkcija u `.h` fajlu koje možemo obrisati tokom sređivanja C koda.
- **Frontend (Čeka Analizu)**: U sklopu analize golemog fajla `display.c`, potvrdićemo prelazak stringova na "pametnu" logiku ispisa.

## 6. Sinergija sa Scenama
Alarmi mehanizam sada napušta nezavisni "SCENE_SECURITY" u korist unaprijeđene "**SCENE_SLEEP**" i "**SCENE_LEAVING**".
- U sceni `SLEEP`: Korisnik čekira `[x] Aktiviraj alarm uz ovu scenu` i nudi mu se samo izbor između Particija Perimetra i Prizemlja, obezbjeđujući komfor kretanja na spratu.
- U sceni `LEAVING`: Odgađanje napuštanja (exit delay x10), oružanje particija unaprijed i simulacija prisustva.
