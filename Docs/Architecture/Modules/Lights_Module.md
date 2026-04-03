# Interna Arhitektura: Modul Rasvjeta (`lights.c` / `lights.h`)

Modul Rasvjeta predstavlja izuzetno zrelu i hibridnu osnovu koda koja u jednom "Opaque Pointer" nizu konvertuje tri različita domena operisanja: RS485 mrežne uređaje, direktne GPIO pinove sa same procesorske ploče i PWM (I2C bazu).

## 1. Dijagram Logičkog Toka (Hibridni Kontroler)

```mermaid
graph TD
    %% Ulazni podaci i API
    EEPROM[(EEPROM <br> EE_LIGHTS_MODBUS)]
    UI((Display / UI <br> `LIGHT_Flip()`))
    ButtonExt((Ožičeni Ulaz <br> `IsButtonActive()`))

    subgraph "Public API (lights.h)"
        Init[LIGHTS_Init]
        Service[LIGHT_Service]
        API_Control[LIGHT_SetState / LIGHT_SetBrightness]
        Defrag[DefragmentLights]
    end

    subgraph "EEPROM Config (LIGHT_EepromConfig_t)"
        Config[Adresa, Tip (BIN, DIM, RGB) <br> Primitive (on_hour, local_pin, tiedToMainLight)]
    end

    subgraph "Interna (Private) Runtime Logika"
        Handle[lights_modbus <br> 100+ Instanci]
        PLC_Engine[HandleExternalButtonActivity]
        Timers_Engine[HandleOnDelayTimers <br> HandleOffTimeTimers <br> HandleLightNightTimer]
        BusState_Engine[HandleLightStatusChanges]
    end

    subgraph "Izlazni Drajveri (Aktuatori)"
        RS485_Bus(RS485 Redovi <br> binaryQueue, dimmerQueue, rgbwQueue)
        GPIO(STM32 Lokalni GPIO <br> SetPin_Output)
        PWM(I2C Ekspander <br> PCA9685_SetOutput)
    end

    %% Životni Ciklus
    Init -->|1. Učitavanje| Config
    Init -->|2. Inicijalizacija Runtime| Handle

    UI --> API_Control
    API_Control --> Handle

    Service -->|Obrada Tajmera| Timers_Engine
    Timers_Engine -.->|Ako istekne| API_Control
    
    Service -->|Obrada Primitivnog Ulaza| PLC_Engine
    ButtonExt --> PLC_Engine
    PLC_Engine -.->|Flip| API_Control

    %% Izlaz
    Service --> BusState_Engine
    BusState_Engine -.->|Stanje Promjenjeno| RS485_Bus
    API_Control -.->|Ako local_pin > 0| GPIO
    API_Control -.->|Ako local_pin != 0| PWM
```

## 2. Hardversko-Kodne Primitivne Karakteristike

Ovaj modul se razlikuje od pametnih modula sa 'Device Deskriptorima' kakav je kapija. Koncipiran je potpuno na gigantskom nizu varijabli `LIGHT_EepromConfig_t` od kojih svaka struktura nudi nevjerovatan miks funkcionalnosti za jedno rasvjetno mjesto.
- **Defragmentacija RAM memorije:** Sistem u pozadini tokom snimanja briše `0` slotove i efektivno "šalta" sve ostale nizove naprijed, rješavajući curenja iterabilne memorije.
- **Delay Save Brightness Logic:** Fenomenalno dizajniran koncept da dimer ili RGB traka dok se svajpa na slajderu ne generiše hiljade `EE_WriteBuffer` udara. `is_dirty_for_saving` registar drži stanje izmjenjenim 5 sekundi. Po stabilizaciji `BRIGHTNESS_SAVE_DELAY_MS` pamti stanje unutar EEPROM i prevenira uništenje čipa.
- **Primitivna PLC i Tajmerska logika ("Kolegin Legacy"):** Modul ne oslanja svoj tajmer na `timer.c` pametni sat, već posjeduje svoje primitivne registratorne varijable `on_hour`, `off_time` minutažne brojalice.
- **TiedToMainLight i Night Timer:** Modul rasvjete upravlja sa makro "All On / All Off" ponašanjem na ekranu. Centralno, sva svjetla asimilirana preko UI `tiedToMainLight=true` dobiće naredbu isključenja kada istekne vremenski predefinisani NightTimer krug.

## 3. Zaključak

Modul je izuzetno otporan i pokriva sve vrste hard-wired prepreka (lokalnih pinova) i napredne mrežne rasvjete kroz jednostavnu IF-THEN petlju i jedan Service Thread bez blokiranja RTOS-a. Spreman za rad!
