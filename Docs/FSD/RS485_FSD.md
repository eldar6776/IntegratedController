# FSD: RS485 Komunikacioni Bus & Queue Transfer Modul

## 1. Opis Funkcionalnosti
Modul `rs485.c` predstavlja glavni mrežni saobraćajni čvor i most između vanjskog svijeta, aktuatora i softverskih domena na glavnom PCB MCU-u. Sistem se usko oslanja na open-source **TinyFrame** biblioteku kako bi organizovao, enkapsulirao i provjeravao strukture paketa slatih (Tx) i primanih (Rx) preko UART-a na fizički TIA/EIA-485 standard. 

## 2. Sistemske Funkcije i Primitive
Sistem operiše koristeći asinkrone metode sa paralelnim Task Queue principom za različite tipove uređaja:
- Modul zadržava statički TinyFrame (`tfapp`) objekat na kojem registruje "Listnere" zadužene da prisluhnu svaki tip Modbus upita po payload `msg->type` hederu.
- Kreirani su namjenski Ring-Bufferi ("Queues") poput `binaryQueue`, `dimmerQueue`, i `thermoQueue` kako moduli sistema (npr. iz `lights.c` ili `curtain.c`) ne bi blokirali rad i čekali slobodan slot na serijskoj liniji. `AddCommand()` osigurava red i mjesto.

## 3. Interrupti i U/I Logika
Modul je dizajniran da presreće svaki bajt momentalno putem `RS485_RxCpltCallback`. Svaki poslati frejm podliježe re-send mehanizmu (`MAX_RETRIES`) sa specifičnim pozicioniranim ACK bajtom koji dokazuje ispravnost izvršenja na `TIMEOUT_MS = 10` ograničenju.

## 4. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`rs485.c` / `rs485.h`):**
1. **Zatečeno Backend Stanje**: Konstrukcija je stabilna. Sva primanja prosljeđuju se konkretnim podsistemima (npr. `DIMMER_SET_Listener` proslijedi adresu i vrijednost na funkciju `LIGHTS_UpdateExternalBrightness`). Termostati koriste teži blok API poziva nad `thermostat.c` funkcijama.
2. **Novi Zahtjevi i Gap Analiza (IDEJE i DODACI)**:
   - **Backoff Timer i Rješenje Sudara (Collisions)**: Trenutno postoji HAL_Delay zagušenje na 1 do 2ms i slanje kroz petlju blokira sve do odgovora. U master planu za ovaj fajl potrebno je uvesti zastavicu ("Flag") koji striktno odbija transmisiju x vremena od posljednjeg bita! RNG (Random Number Generator) ili Unique MCU ID formula sprječavaće da dva ili više ekrana koji su probuđeni iz restarta zaguše bus šaljući istovremeno `TF_WriteImpl`.
   - **Background Boot Harvesting (Sakupljanje Stanja)**: Planirani refaktoring obuhvata kreiranje funkcije (kroz GUI Boot Logo Loading ekran) koja će pingati sve konfigurisane adrese, a listeneri ovog fajla će u tišini ažurirati `lights.c`/`scene.c` interne strukture bez iscrpljujućih ACK upita!
3. **Frontend**: Ovaj podsistem ostaje u tami (non-visual), ali utiče usporavajuće i trzaće UI grafiku sve dok se `HAL_Delay(1)` i timeout `while(timeout--)` petlje radikalno ne oslobode iz srži hardvera koristeći gore navedene DMA UART ili state-machine callback pristupe!
