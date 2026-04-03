# Interna Arhitektura: Modul Pametni Alarm (`timer.c` / `timer.h`)

Pametni Alarm služi kao globalni sistemski "Cron-job" interfejs. Zastupljen je kao klasični Singleton bez višestrukih instanci. Njegova logička povezanost sa integrisanim funkcijama je visoka (zujalica, poziv scena).

## 1. Dijagram Logičkog Toka (RTC Latching)

```mermaid
graph TD
    %% Eksterni Izvori
    RTC((Sistemski sat <br> HAL_RTC))
    EEPROM[(EEPROM <br> EE_TIMER)]
    GUI((Ekran / GUI <br> display.c))

    subgraph "Public API (timer.h)"
        Init[Timer_Init]
        GetSet[SetHour, SetMinute, SetRepeatMask]
        Service[Timer_Service]
    end

    subgraph "Interna (Private) Logika (timer.c)"
        Data[Strukture \n Config + hasTriggeredThisMinute]
    end

    %% Akcije
    Init -->|Čitanje| EEPROM
    GUI -->|Podešavanje i Snimanje| GetSet
    GetSet --> Data
    
    %% Service Loop
    Service -.->|Čitanje Sata| RTC
    RTC -.->|Generiše Tick/Vrijeme| Service
    Service -->|Provjera Vremena i Maske| Data

    %% Izlazi 
    Data -->|Poklapanje & hasTriggered == false| Okidac(Aktivacija Alarma)
    Okidac -->|actionBuzzer == true| Buzzer(Buzzer_StartAlarm)
    Okidac -->|sceneIndex != -1| Scena(Scene_Activate)
    
    %% Latch reset
    RTC -->|Promjena Minute| LatchReset[Reset hasTriggered = false]
    LatchReset --> Data
```

## 2. Analiza Hardversko-Kodne Asimilacije

- **Singleton bez Opaque Pointera:** Zanimljivo je da arhitektonski `timer.c` ne koristi `handle` dodjele (`Opaque pokazivač`) kao što su radili `curtains` i `gate`, jer ne nadgleda polje raznih senzora. Konstruisan je kao opšti hardkodovani sistem na jednoj varijabli `static Timer_Runtime_t timer;`. Njegov zadatak je jednostavan i ne nalaže "Device Deskriptore".
- **RTC Latch Sistem (`hasTriggeredThisMinute`):** Pošto `Timer_Service` upada u `main() while(1)` petlju vjerovatno desetine hiljada puta unutar jedne minute, isrogramiran je hardverski i softverski sigurni `latch` mehanizam – bilježi se izvršenje i zaleđava sve okidače unutar tekuće minute, sprječavajući da MCU "zabije" uzastopnim paljenjem zujalice/scene. Reset se dešava precizno prelaskom u narednu minutu.

## 3. Zaključak
Ovo je izuzetno čisto i elegantno postavljen kod. Pruža jednostavnost u razvoju bez ikakvih tehnoloških opstrukcija. U cjelosti se poklapa sa dokumentacijom koja je arhivirana.
