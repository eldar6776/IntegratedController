# FSD: Sistem Scena, UI Navigacija i Sistemske Interakcije

Ovaj dokument je kompozicija na bazi opštih planova i fajla "Gemini zadatak.txt", te dodatne analize dinamike selekcijskih ekrana.

## 1. Navigacija i Ekran Modeli 
Slijed ekrana bez dinamičkih uslova na klik hambuger menija je koncipiran na:
`MAIN_SCREEN` -> `SELECT_1` -> (`SELECT_LAST` formalno poznato kao `SELECT_2`). Ostaje preimenovanje u bazi koda radi lakših dodavanja trećih pod-ekrana kasnije. 

### Ekran matrice predefinisane logike modula:
- Četiri pozicije (Lijevo G-D, Desno G-D). 
- Novi UI moduli zahtijevaju opcionalne ikonice zavisno od hard/soft konfiguracije.
- Ikona broj 4 se nalazi u dinamičkim modovima kroz oba ekrana `SELECT_1` i `SELECT_2` (`SELECT_LAST`) i posjeduje drop-down meni u postavkama da se spriječi dupliranje.

### Dinamička Lista funkcija (Ikona #4):
- **LANGUAGE**: Kratki klik lokalno prilagođava UI jezik MCU, dugi pritisak šalje na novi prozor gdje promjena jezika šalje `LANGUAGE_SET` broadcast preko modbus-a instalaciji kako bi cijeli dom okrenuo jezik.
- **THEME**: (Kratki nema, Dugi pritisak = Skin prozor, neimplementirano još).
- **SOS**: Pravi klik ukusan zvukom a ukoliko je alarm programiran, aktivira tihu dojavu provalne situacije.
- **ALL OFF**: Ikona za aktiviranje macro-a za isključenje više adresa (ne-scene model).
- **OUTDOOR**: Kratki = Toggle izabranih svjetala; Dugi = Set Outdoor timera.
- **DEFROSTER** / **VENTILATOR**: Legacy moduli identični prošlosti, toggle/ON/OFF u backendu.

## 2. Filozofija SCENA Konfigurativnog Modula
Sistem automatske i inteligentne komande raspršenog "Super-Loop" i Decentralizovanog State upravljanja modifikuje stanje kuće kroz `SCENA` modele ponašanja. Kroz UI ekran `SCREEN_SCENE` pristupa se matrici do 6 lokalnih scena (3x2). Odmah vidimo brzu komandu, a dugi pristup ulazi u podešavanja ili uklanjanja date scene. 

### Tipovi Scena:
* **"Komfor" Scene (Lokalni model)**: Opoziva ambijent. (Film, Odmor, Večera). Koriste brzi "Memoriši Trenutno Stanje" snapshot, ali im je dostupno i detaljno anketno postavljanje kroz Čarobnjaka (Screen-by-screen setup). Njihova aktivacija je strogo lokalna (samo svjetla na tom i na povezanim adresnim elementima).
* **"Sistemske" Scene (Globalni Događaji)**: Orijentisane globalno kao emitovane poruke (Leaving the home i Homecoming). Postavljaju sistem kuće u `SYSTEM_STATE_AWAY_ACTIVE`. Ovo inicijalizuje akcije preko cijele instalacije bus arhitekture. 

### Integracije kroz Scene:
1) SCENE_SLEEP (Mijenja raniju SCENE_SECURITY). Ova komfor/sistem scena aktivira alarme za perimetar i donji kat uz ostavljanje sigurnosti u mirovanju kroz dio prizemlja kako biste komforno prolazili.
2) SCENE_HOMECOMING posjeduje sigurnu funkciju okidača (Trigger) gdje preko DIGITAL_INPUT event stringova reaguje na pritisak daljinskog "Otvori ulazna Vrata" od modula kapije. 

## 3. Generalne Scene Postavke
Ubačen `SCREEN_SETTINGS_7` ili centralni modul. 
Ovdje se popunjava globalna `is_configured` ili enable/disable mašina scena. Mapiranje specifičnih adresa okidača za RS485 detekciju Homecoming okidača.

## 4. Tehnološki Oslonac
1) *Broadcast* tipa `SCENE_CONTROL`. Za obavještenje na bus da su i drugi displeji dužni promjeniti status globalnog sistema u "Home" ili "Away".
2) Aplikacijsko rješavanje ne-blokirajućih sistemskih grešaka sa globalnim flagom `DEVICE_FAULT` (npr. gate_error).

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`scene.c` / `scene.h`):**
1. **Zatečeno Backend Stanje**: Implementirana je visoka podjela logike (`SceneType_e` i `SystemState_e` - npr. Home i Away). Metode memorisanja komfor scena sakupljaju stanja kroz bus preko getter funkcija povezanih C modula, i smještaju to u moćan EEPROM niz obavijen CRC kontrolom. Podržana je asinhrona aktivacija odgođenih "Leaving/Odlazak" funkcija.
2. **Novi Zahtjevi i Gap Analiza (BLOKATORI)**:
   - **Roletne**: Modul roletni mora biti usavršen da nudi relativne "scene pozicije", do tada `curtain_states` samo uzima smjer što nije dovoljno za prave korisničke automatizacije. 
   - **Pametne Jalousie Moduli**: `Scene_t` će morati dobiti mapiranje i za `Jalousie` module koji direktno znaju % (procenat)!
   - **Zatvorena Arhitektura okidača**: Metoda `Scene_Memorize()` je funkcionalna samo za brzi "Snapshot", ali nedostaje UX UI Čarobnjak koji po sobama anketira korisnika šta i koliko želi u sceni!
3. **Frontend**: Ovaj modul ostaje u statusu "in-progress" do preplitanja sa UI/UX revizijom koja slijedi na kraju. Kod se za sada ostavlja djelomično definisan čekajući operativnost roletni.
