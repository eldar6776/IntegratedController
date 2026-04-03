# Arhitektura "Pametne Vile" – Generalni Sistemski Prikaz

Ovaj dokument vizualno i logički prezentuje cjelokupan raspored mikrokontrolerskog sistema (STM32F7xx) koji se vrti oko mehanizma "Super-Loop" (Beskonačna petlja) unutar `main.c`. Dizajn omogućava brzu realizaciju procesa bez potrebe za kompleksnim Real-Time Operativnim Sistemom (RTOS), koristeći isključivo asinkrone statične mašine.

## 1. Sistemski Dijagram (Core System Architecture)

Raspored resursa svake komponente te tok informacija:

```mermaid
graph TD
    %% Hardverski Sloj
    subgraph "Hardware & Peripherals (STM32F7 / HAL)"
        RTC[RTC - Real Time Clock]
        EEPROM[EEPROM / Konfiguracije]
        TIMER[Hardware Timers & Interrupts]
        ADC[ADC - Lokalni NTC Senzori]
        IWDG[Watchdog]
        bus(RS485 Transceiver)
        LCD(LCD & Touch Controller)
    end

    %% Glavni Loop
    subgraph "Core Execution (main.c)"
        MAIN((Super Loop))
        INIT[Sistemska Inicijalizacija]
        INIT --> MAIN
        MAIN -- Zove sekvencijalno --> SERV
    end

    %% Moduli i Servisi
    subgraph "Moduli i Servisi (Poslovna Logika)"
        SERV((Servisna Petlja))
        
        S_RS485[RS485_Service]
        S_DISP[DISP_Service]
        S_THSTAT[THSTAT_Service]
        S_SECURITY[Security_Service]
        S_CURTAIN[Curtain_Service]
        S_GATE[Gate_Service]
        S_TIMER[Timer_Service]
        S_SCENE[Scene_Service]
        S_OUTDOOR[Outdoor_Service]
        S_FWUP[FwUpdateAgent_Service]
        
        SERV --> S_RS485
        SERV --> S_DISP
        SERV --> S_THSTAT
        SERV --> S_SECURITY
        SERV --> S_CURTAIN
        SERV --> S_GATE
        SERV --> S_TIMER
        SERV --> S_SCENE
        SERV --> S_OUTDOOR
        SERV --> S_FWUP
    end

    %% Veze
    RTC -. Čita vrijeme .-> S_TIMER
    RTC -. Čita vrijeme .-> S_OUTDOOR
    EEPROM <-. Auto Save / Load .-> S_THSTAT
    EEPROM <-. Auto Save / Load .-> S_SCENE
    ADC -. Prikuplja Temperaturu .-> S_THSTAT
    bus <--> S_RS485
    LCD <--> S_DISP
    
    S_RS485 <-->|Poruke| S_THSTAT
    S_RS485 <-->|Poruke| S_SECURITY
    S_RS485 <-->|Poruke| S_SCENE
    
    S_DISP -->|Touch Akcije| S_THSTAT
    S_DISP -->|Touch Akcije| S_SECURITY
    S_DISP -->|Touch Akcije| S_SCENE
    S_DISP -->|Touch Akcije| S_GATE
    
    IWDG -. Osvježavanje .-> MAIN
```

## 2. Opis Pojedinačnih Slojeva
- **Hardware & Peripherals**: Sav rad sa senzorima i hardware interfejsom prolazi kroz osnovne drivere. Svi izlazi (`bus`) prema instalaciji idu preko UART-a na RS485.
- **EEPROM**: Najvažniji modul trajne pohrane svake komponente sistema na tačno određenim adresama i offset-ima. Memorisanje scena i postavki (Vrijeme, PIN-ovi, imena).
- **Servisna Petlja (Super Loop)**: Brzina izvršavanja mora biti izuzetno visoka, zbog čega funkcije unutar svake "Service" petlje po pravilu ne smiju koristiti "delay" komande (bez funkcija tipa `HAL_Delay()`), već se oslanjaju na non-blocking `HAL_GetTick()` provjeru za stanje svih softverskih tajmera.

## 3. Analiza Modula ("Divide and Conquer")
Kako ogromna datoteka od +18,000 linija (`display.c`) ne bi opteretila budući kod, moraćemo provesti plan refaktorisanja izdvajanja ekrana u zasebne C fajlove (npr. `display_thermostat.c`, `display_security.c`). U međuvremenu, sve ispod se preuzima za Modul_po_Modul diagrame koji pokreću ovaj Loop u posebnim analizama.
