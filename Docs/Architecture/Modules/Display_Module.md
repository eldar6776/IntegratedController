# Interna Arhitektura: GUI Display Modul (`display.c` / `display.h`)

Mega-modul `display.c` ima preko 12,000 linija koda. Međutim, njegova unutrašnja organizacija je izuzetno serijska i pridržava se strogih obrazaca (Pattern-a) dizajniranih na temelju **STemWin** biblioteke. Ovdje objašnjavamo taj obrazac za svakog programera koji sleti u ovaj fajl.

## 1. Dijagram Logičkog Toka (Init-Service-Kill Ponašanje)

Za razliku od RTOS taskova, `display.c` je jedno-nitni "State Machine" upravljan `enum`-om _eScreen_.

```mermaid
graph TD
    %% Glavna petlja iz main.c
    Main[main.c <br> while(1) petlja]
    Service[DISP_Service]
    Touch[PID_Hook / HandleTouchPressEvent]

    Main --> Service
    Main --> Touch

    subgraph "State Machine (eScreen Router)"
        Router{Trenutni 'screen'?}
    end
    Service --> Router

    subgraph "Primjer: Životni Ciklus Ekrana (SCREEN_LIGHTS)"
        DSP_InitLighting[1. DSP_InitLightsScreen <br> (Kreira Widgete, prvi Frame)]
        ServiceLighting[2. Service_LightsScreen <br> (Čita Gettere, Crtanje na GUI)]
        DSP_KillLighting[3. DSP_KillLightsScreen <br> (Uništava Handle-ove, briše RAM)]
    end

    Router -->|Trajanje| ServiceLighting

    %% Touch interakcije
    Touch -.->|Korisnik bira novi ekran| DSP_KillLighting
    Touch -.->|Postavlja novi 'screen' enum| DSP_InitNoviEkran(DSP_Init_SljedećiEkran)
```

## 2. Hardversko-Kodne Asimilacije i Karakteristike
- **Dizajnerske Strukture (`Layout` tabele):** Modul ne koristi hardkodirane pozicije razbacane po fajlu. Sav raspored koordinata nalazi se odmah ispod includa u strukturama tipa `select_screen2_drawing_layout`. Time je omogućeno brzo mijenjanje padding-a i centralizovanje ekrana bez "lova" na brojeve 120, 240, 190.
- **X-Macro Generisanje ID-jeva:** Korišten je izuzetan `#define WIDGET` i `#include "settings_widgets.def"` potez kojim se `SettingsWidgetID_e` enum popunjava u trenutku kompajliranja, što iznimno čisti scope i dodjele ID-jeva.
- **ScreenSaver i Ghost Cleaning:** Pored standardnih ekrana, `Handle_PeriodicEvents` vrti pozadinske skripte popud `GHOST_WIDGET_SCAN_INTERVAL` (brisanje zabuljujućih elemenata u RAM-u STemWin-a) i tajmeri noćnog zaslona.

## 3. ZATEČENI PROBLEM (Zašto Refaktorizacija?)
Ovaj monolitni fajl i pored perfektnih pravila sadrži **previše funkcija u istom pre-processor scope-u**. Neke sekcije (poput UI alarma) i sekcije za (Termostat, Roletne, Kapije) se miješaju. To stvara predug cache prozor, dug build-time u STM32CubeIDE i veoma teško timsko git održavanje.
Zato je neophodno pokrenuti plan cjepanja u modularne `.c` panele koji zadržavaju isti Init-Service-Kill obrazac ali svaki pod svojom ekskluzivnom domenom.
