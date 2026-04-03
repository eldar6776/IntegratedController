# Interna Arhitektura: Termostat Modul (`thermostat.c` / `thermostat.h`)

Modul termostata predstavlja visoko enkapsuliran Singleton objekat, koristeći Opaque Pointer obrazac sakrivanja podataka (`THERMOSTAT_TypeDef_s`). Na ovaj način, "main.c" ne zna kako termostat raspolaže podacima, čime se postiže robusno objektno-orijentisano inženjerstvo u C-u.

## 1. Dijagram Logičkog Toka (Master/Slave & PID Logika)

```mermaid
graph TD
    %% Eksterni Izvori
    ADC((ADC3_Read <br> iz main.c))
    RS485_Bus((RS485 Modbus <br> rs485.c))

    subgraph "Public API (thermostat.h)"
        SetTemp[Thermostat_SetMeasuredTemp]
        G_S[Getters & Setters]
        Service[THSTAT_Service]
    end

    subgraph "Interna (Private) Logika"
        Data[THERMOSTAT_TypeDef_s <br> (Runtime & EEPROM configs)]
        PID[Logika Grijanja/Hlađenja i FanCoil]
        LocalFan[Upravljanje GPIO <br> Puhalicom]
        Delay[FanCoil Timers & Delays]
        
        Service --> |if Master == true <br> ili group == 0| PID
        PID --> Delay
        Delay --> LocalFan
    end
    
    %% Veze
    ADC --> |Termistor Očitanje| SetTemp
    SetTemp --> |Značajna Razlika > 0.2C| Data
    
    %% RS485 komunikacija (Slave/Master sync)
    Service --> |if hasInfoChanged| RS485_Bus
    RS485_Bus --> |Ažurira Slave| Data

    %% Izlazi 
    LocalFan --> |FanLow/Mid/High| Hardware_Pins(Lokalni Releji/Triaci)
```

## 2. Analiza Hardversko-Kodne Asimilacije (Master vs Slave)

- **Master/Slave Arhitektura:** Termostat može raditi na dva načina zasnovana na `group` i `master` postavkama u EEPROM strukturi (`THERMOSTAT_EepromConfig_t`).
  - **Standalone / Master:** Čita fizički spojeni senzor putem `Thermostat_SetMeasuredTemp` pozivan otprilike iz ADC petlje u `main.c`. Ukoliko je Master u mreži sa `hasInfoChanged`, on formira emitovanje stanja (`THERMOSTAT_INFO`) ka RS485 RS485 mreži.
  - **Slave:** Služi kao UI kontrola. Ne obavlja kontrolu ventilatora direktno, već preuzima temperaturu i parametre preko bus-a i na isti način vraća izmjene postavki `sp_temp` nazad ka Master-u.
  
- **FanCoil Kontrola i Kašnjenja:** Modul obrađuje napredni "3-Band" i "3-Speed" FanCoil sistem uz integrisani vremenski limiter `FANC_FAN_MIN_ON_TIME`. Prisutna je zaštita releja, tako da on prolazi redoslijed isključivanja/uključivanja na `HAL_GetTick()`.

## 3. Gap Analiza naspram "Gemini Plan"
Zatečeno stanje posjeduje robustan FanCoil (Histerezni) kontroler. **Ne posjeduje** napredne tajmere za "Pumpe" ili više nezavisnih krugova grijanja. Ovo ukazuje da će, u sklopu realizacija, biti potrebno ozbiljno nadograditi logiku za odgodu pumpi o kojoj govori FSD!
