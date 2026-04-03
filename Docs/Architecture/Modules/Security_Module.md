# Interna Arhitektura: Alarm & Security Modul (`security.c` / `security.h`)

Ovaj dokument je fokusiran isključivo na internu operativnu logiku i arhitekturu sigurnosnog modula `security.c`.

## 1. Dijagram Logičkog Toka (State & Data Flow)

Modul prvenstveno služi kao "Controller" između korisničkog interfejsa (Display) i komunikacionog interfejsa (RS485), oslanjajući se na asinkrone komande za izvršavanje radnji.

```mermaid
graph TD
    %% Eksterni događaji i okidači
    UI_Input((User Interface <br> display.c))
    RS485_Bus((RS485 Modbus <br> rs485.c))
    EEPROM[(EEPROM <br> EE_SECURITY)]

    %% Glavne metode (Actions)
    subgraph "Public API (security.h)"
        Init[Security_Init]
        Val[Security_ValidateUserCode]
        ToggPart[Security_TogglePartition]
        ToggSys[Security_ToggleSystem]
        SOSA[Security_TriggerSilentAlarm]
    end

    %% Privatna logika i podaci (security.c)
    subgraph "Interna (Private) Logika"
        Data[g_security_settings \n (Struktura postavki)]
        State[partition_is_armed \n system_is_in_alarm]
        Exec[Execute_Command]
        HandleEvent[HandleSensorEvent]
        Save[Security_Save]
    end

    %% Veza memorije
    Init -->|1. Čita postavke| EEPROM
    Init -->|2. Validira CRC| Data
    Save -->|Snima promjene| EEPROM
    Data -.->|Zadržava: PIN, Imena, Adrese| State

    %% Ulaz iz RS485 (Povratne adrese)
    RS485_Bus -->|SECURITY_BusEvent| HandleEvent
    HandleEvent -->|Ažurira stanje| State
    State -.->|Trigera redraw| UI_Input

    %% Ulaz iz Display-a (Komande)
    UI_Input -->|Pritisak na tipke| ToggPart
    UI_Input -->|SISTEM Uklj/Isklj| ToggSys
    UI_Input -->|SOS| SOSA
    UI_Input -->|Unos Pina| Val

    ToggPart --> Exec
    ToggSys --> Exec
    SOSA -->|Šalje Puls BINARY_ON| RS485_Bus

    Exec -->|Dodaje u binaryQueue| RS485_Bus
```

## 2. Analiza Hardversko-Kodne Asimilacije
- **Sigurnosne strukture memorije**: Korištena je struktura `Security_Settings_t` sa zaštitnim omotačem `#pragma pack(1)`. Svaki zapis ima CRC i magic number (`EEPROM_MAGIC_NUMBER`). Ukoliko dođe na nepoznato okruženje, `Security_SetDefault()` poziv briše memoriju i kreira svježe nule te defaultni pin `7892` (ili iz macro definicije).
- **Asinkrono komandovanje**: Modul se ne zadržava ni milisekundu preko mjere. Funkcije poput `Security_TogglePartition()` isključivo uzimaju `partition_relay_addr` iz postavki te "sipaju" `BINARY_ON/OFF` komande direktno preko `AddCommand(&binaryQueue...)` ka `RS485_Service()`. 
- **Pamćenje Particija**: U memoriji mikrokontrolera stoje RAM flag-ovi `partition_is_armed` koji pronalaze svoje ažuriranje kada `DIN_EVENT` stigne odozdo sa senzora, bez ikakvog `delay()` pollinga.

## 3. Tehnički Komentar i Refaktor Dokaz
Zatečeni kod u `security.c` reflektira da su prethodni zadaci odrađeni sa **Backend** strane. Prisutna su i ispravna polja `system_name[21]` i `partition_names[3][21]`.
Ovakav modul zahtijeva minimalne do nikakve izmjene osim prevođenja u front-endu.
