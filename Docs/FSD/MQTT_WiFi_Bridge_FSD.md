# FSD: MQTT WiFi Bridge & Home Assistant Integracija

## 1. Opis Funkcionalnosti
Ovaj modul proširuje bazični industrijski RS485 sistem na IoT nivo, omogućavajući povezivanje na WiFi mrežu i integraciju sa Home Assistant (HA) platformom preko MQTT protokola. Arhitektura prati "Local-First" princip: primarna sigurnost i kontrola se obavljaju preko žice (RS485), dok WiFi služi kao luksuzna nadogradnja za mobilne aplikacije, tablete i glasovne asistente.

### Ključne prednosti:
- **Decentralizacija**: Svaki od 20-30 kontrolera u vili je nezavisni MQTT čvor.
- **Self-Presentation**: Uređaj automatski javlja svoje funkcije (svjetla, termostate, kapije) Home Assistant-u.
- **Offline Resilience**: Potpuna funkcionalnost sistema ostaje očuvana u slučaju pada WiFi mreže.
- **Manualna Konfiguracija**: Potpuni unos parametara direktno na ekranu uređaja bez eksternih alata.

## 2. Hardverska Topologija
- **Main MCU (STM32)**: Glavni procesor koji drži UI i biznis logiku.
- **Wireless Modem (ESP32)**: Namjenski modul na PCB-u povezan preko UART-a.
- **Protokol**: **TinyFrame** se koristi za pouzdanu razmjenu podataka između STM32 i ESP32 modema.

## 3. Korisnički Interfejs (UI/UX)
Korištenjem postojeće alfanumeričke tastature, korisnik konfiguriše sistem kroz dva nova ekrana:

### 3.1 SCREEN_SETTINGS_WIFI
- **Scan**: Lista dostupnih WiFi mreža.
- **SSID/Password**: Ručni unos ili odabir iz liste.
- **Status**: Prikaz jačine signala (RSSI) i IP adrese.
- **Indikator**: WiFi ikona na glavnom ekranu sa statusom konekcije.

### 3.2 SCREEN_SETTINGS_MQTT
- **Broker IP/Host**: Adresa Mosquitto ili HA brokera.
- **Port**: Default 1883 (ili 8883 za SSL).
- **Kredencijali**: Username i Password za broker.
- **Topic Prefix**: Jedinstveno ime sobe (npr. `dnevna_soba`) koje definiše korijen MQTT topika.

## 4. MQTT Discovery (Home Assistant Integracija)
Sistem koristi HA Discovery protokol za "Plug & Play" iskustvo. Svaki kontroler šalje JSON payload na `homeassistant/config` topic.

### Primjer Discovery logike:
1. **STM32** čita svoju konfiguraciju (npr. ima 4 svjetla).
2. **STM32** šalje TinyFrame paket ESP32-u: `MSG_DISCOVERY_LIGHT, ID: 1, NAME: "Luster"`.
3. **ESP32** objavljuje na MQTT: 
   ```json
   {
     "name": "Dnevna Soba Luster",
     "stat_t": "jubera/dnevna_soba/light1/state",
     "cmd_t": "jubera/dnevna_soba/light1/set",
     "uniq_id": "mac_address_light1",
     "device": { "identifiers": ["mac_address"], "name": "Room Controller 1" }
   }
   ```

## 5. Protokol Redirekcije (RS485 <-> MQTT)
- **Status Update (Bus -> MQTT)**: Kada se svjetlo upali preko displeja ili fizičkog tastera na RS485 busu, STM32 šalje update ESP32-u, koji ga momentalno objavljuje na MQTT `state` topic.
- **Komanda (MQTT -> Bus)**: Home Assistant šalje `ON` na `command` topic -> ESP32 prima -> šalje TinyFrame paket STM32-u -> STM32 izvršava RS485 komandu prema aktuatoru.

## 6. Sigurnost i Pouzdanost
- **LWT (Last Will and Testament)**: Ako kontroler izgubi napajanje ili WiFi, ESP32 šalje "Offline" status brokeru, a HA ga prikazuje kao nedostupnog (zasivljen).
- **Watchdog**: STM32 nadzire rad ESP32 modema; ako modem prestane odgovarati, vrši se hardverski reset ESP modula.
- **No Blocking**: Komunikacija sa ESP modema se obavlja asinhrono; mrežni problemi ne smiju usporiti UI ili RS485 bus.

## 7. Trenutno Implementirano Stanje
- **UI**: Potrebno kreirati `SCREEN_SETTINGS_WIFI/MQTT` unutar `GUI_Settings.c`.
- **Backend (STM32)**: Potrebno razviti `mqtt_modem.c` servis.
- **Backend (ESP32)**: Potrebno razviti "Bridge" firmware sa TinyFrame podrškom.
