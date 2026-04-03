# Interna Arhitektura: Modul Kapija i Vrata (`gate.c` / `gate.h`)

Ovaj dokument reflektuje genijalnu unutrašnju arhitekturu na bazi profajlera (Device Descriptor) i Univerzalne State Mašine (USM). Postiže neviđen stepen enkapsulacije sa dinamičnim re-rutiranjem ponašanja na bazi odabranog tipa motora (pametna brava vs. klizna kapija).

## 1. Dijagram Logičkog Toka (USM Profiler)

```mermaid
graph TD
    %% Eksterni Izvori
    RS485_Bus((RS485 Modbus <br> rs485.c))
    EEPROM[(EEPROM <br> EE_GATES)]
    GUI((Ekran / GUI <br> display.c))

    subgraph "Public API (gate.h)"
        Init[Gate_Init]
        Trig[Gate_Trigger... <br> (SmartStep, Open, Close, Lock)]
        BusEvent[GATE_BusEvent]
        Service[Gate_Service]
    end

    subgraph "Metapodaci (Profiler)"
        Lib[g_ControlProfileLibrary <br> 7 Podržanih Profila]
    end

    subgraph "Interna USM Logika (gate.c)"
        Data[Opaque Tip: Gate_Handle <br> EEPROM + Runtime Timers]
        SendAct[Gate_SendAction <br> Prevođenje Komandi]
        TimerOps[Timer Manager <br> Pulse, Pedestrian, Cycle]
        SensorLogic[HandleSensorEvent]
    end

    %% Životni ciklus i Komande
    Init -->|Učitavanje| EEPROM
    GUI -->|Pritisak Ikone| Trig
    Trig --> SendAct
    
    %% Magic Sauce: Čitanje profila
    SendAct -.->|Konsultuje tip logike| Lib
    SendAct -->|Upisuje Tajmere| Data
    SendAct -->|Generiše RS485| RS485_Bus

    %% Timer Service
    Service --> TimerOps
    TimerOps -.->|Istekao Delay/Timeout| SendAct
    TimerOps -.->|Istekao Puls Lock-a| RS485_Bus

    %% Feedback / Senzori
    RS485_Bus -->|Senzor Stigao| BusEvent
    BusEvent --> SensorLogic
    SensorLogic -.->|Parsira Senzor (Limit)| Data
    SensorLogic -.->|Prekida Tajmere| TimerOps
```

## 2. Analiza Hardversko-Kodne Asimilacije

- **Device Deskriptori (`ProfilDeskriptor_t`):** Arhitektura je bazirana na metapodacima. Učitavanjem jednog `gate` elementa, USM provjera u `g_ControlProfileLibrary` za koji profil je vezan (npr: `CONTROL_TYPE_NICE_SLIDING_PULSE`). U tom slučaju on zna da posjeduje 4 komandna pulta i 2 limitna senzora. Ako se mapira na "Pametnu Bravu", USM zna da posjeduje samo 1 Unlock i eventualno jedan feedback, potpuno isključujući nepotrebnu kompleksnost za ostale faze ("skip" faze).
- **Hardverski Tajmeri Izvršavanja (`active_timer_type`):** Sadrži tri nivoa tajmera (Pulse, Pedestrian, Cycle). Bravara okida samo "Pulse" do 1 sec (i vraća state na Zatvoreno), dok velika klizna kapija okida "Cycle" tajmer koji nadgleda timeout failstate (`GATE_STATE_FAULT`).
- **Event-Driven RS485 Očitavanje:** Senzorski event prolazi odozdo iz RS485 buffera, okida pretragu instance putem `Gate_FindByFeedbackSensor` i instantno zatvara petlju tajmera i zaustavlja motore!

## 3. Zaključak i Mogućnosti Proširenja
Ovaj modul je najelegantnije riješen u sistemu i ostavlja otvoren put za dodavanje novih profila roletni/vrata BEZ ikakvog miješanja u glavnu `Gate_Service` strukturu izvođenja. Potrebno je samo proširiti konstantni enum objekat profila.
