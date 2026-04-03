# Interna Arhitektura: Modul Scena (`scene.c` / `scene.h`)

Sistem Scena služi kao glavno "Ljepilo" za sistemski komfor i automatizaciju u ovom projektu.

## 1. Dijagram Modula (Backend Arhitektura i Blokatori)

```mermaid
graph TD
    %% Eksterni Izvori i EEPROM
    EEPROM[(EEPROM <br> EE_SCENES)]
    GUI((Ekran UI/UX - <br> ČAROBNJAK Čeka Redizajn!))

    subgraph "Public API (scene.h)"
        Init[Scene_Init / Scene_Save]
        Service[Scene_Service]
        Memorize{Scene_Memorize <br> Brzo pamćenje stanja}
        Activate{Scene_Activate <br> Pokretanje Scene}
    end

    subgraph "Sistemski Interfejsi (Polu-Završeno)"
        Light[LIGHT_SetState / itd]
        Termo[Thermostat_SP_Temp_Set]
        Curtain[Curtain_Move - PROBLEM!]
    end

    subgraph "Zavisnosti (State Management)"
        SystemState[SystemState_e <br> HOME / AWAY / SETTLING]
        SceneType[SCENE_TYPE_* <br> LEAVING / HOMECOMING / SLEEP]
    end

    Init <-->|Struktura: Scene_EepromBlock_t| EEPROM
    GUI -->|Pritisak na Memorisanje| Memorize
    GUI -->|Aktivacija| Activate

    %% Memorisanje sakupljanjem stanja
    Memorize -.->|Kopira Maske i Stanja| Light
    Memorize -.->|Kopira Setpoint| Termo
    Memorize -.->|Čita Curtain_getNewDirection| Curtain
    
    %% Problem kod Roletni
    Curtain -.-x|BLOKIRANO: <br> Nije riješeno pamćenje nultih tački| GUI

    %% Aktivacija razvodi stanja
    Activate -->|1. Standard: Okida promjene| Light
    Activate --> Termo
    Activate --> Curtain
    Activate -->|2. Sleep| Security(Perimetar)
    Activate -->|3. Leaving| TimerD[scene_runtime_data <br> LEAVING_DELAY]
    
    TimerD -.-> Service
    Service -->|Nakon 60s -> AWAY_ACTIVE| SystemState
```

## 2. Analiza Hardversko-Kodne Asimilacije

- **EEPROM Block i Strukture:** Struktura `Scene_t` dominira mapiranjem (dijeli maske za svjetla, roletne, termostate i sigurnosnu konfiguraciju unutar jednog entiteta). Niz od 6 scena se čuva atomarno unutar bloka `Scene_EepromBlock_t` koji štiti CRC broj.
- **Odložena (Asinhorna) Logika:** Kroz `Scene_Activate()`, scene koje nisu standardne (poput `SCENE_TYPE_LEAVING`) delegiraju zadatak i pale `SCENE_RUNTIME_STATE_LEAVING_DELAY` tajmer u paralelu sa main petljom. `Scene_Service` detektuje istek i prebacuje cijelu zgradu na "Away" mod. Prava event driven softverska izvedba!

## 3. ZATEČENI BLOKATOR (Status: Neispravno do kraja)
Modul implementira izvanredan "Comfort" sistem (svjetla i prenos setpoint komandi), ali nailazi na veliki GAP u razvoju:
1. **Zavisnost na glupim Roletnama:** Trenutna metoda pamćenja snima samo `UP/DOWN/STOP` stanja kroz `Curtain_getNewDirection`. Scena "Čarobnjaka" mora moći u budućnosti mjeriti relativno vrijeme pozicije (Nulta Tačka do stajanja) kako bi znala naćrtati to stanje kroz `curtain_timers` niz - i ovo prvo treba završiti na kodu Modula Roletni da bi Scene radile.
2. **UI/UX Čarobnjak nije napisan:** Dio koda vezan za `display.c` koji navodi korisnika "Odaberi koje želiš svjetlo", "Odaberi koju želiš roletnu", a ne samo brzo `Scene_Memorize` dugme. 
