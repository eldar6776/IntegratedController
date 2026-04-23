# FSD: STemWin GUI Display System i Touch Menadžement

## 1. Opis Funkcionalnosti
Centralni vizuelni podsistem aplikacije. Modul `display.c` instancira, crta na LCD ekran i ubire touch događaje nad hiljadama emWin widgeta. Podržava preko 20 stanja ekrana, internacionalizaciju stringova (BSHC, ENG, GER...), numeričku i alfanumeričku touch tastaturu, i EEPROM pamćenje podešavanja za izgled zaslona.

## 2. Ponašanje i Interakcija
- Kontrola GUI aplikacije usklađena je isključivo na event-based interakciju tipa kratki klik i dugi klik (`HandleTouchPressEvent` vs `HandleTouchReleaseEvent` vs GUI_PID timer tracking).
- Cijeli sistem posjeduje globalni meni u "Hamburger" Layout zonama (gore-desno ili dolje-lijevo na rubovima 480x272 px rezolucije) koje provode povratak (Return) metode ka Main screenovima.

## 3. Podešavanja (Settings Meni)
UI podsistem pruža veliku tablicu `Display_EepromSettings_t` strukture. Njeni parametri:
- Definisanje RS485 arhitekture na nivou GUI ploče (Baud rate i Protocol opcije).
- Timeout kontrole Screensaver-a i boja fonta na istom.
- Globalni enable/disable flegovi za scene, security module itd, čije isključenje automatski skriva ikonice sa navigacije (`SCREEN_SELECT_2`).
- Definisanje okidača (Triggers) za pametne scenske dolaske.

## 4. Tehnološke Specifikacije i Podaci
Najveća odlika ovog modula je izolacija API funkcija. `display.c` nikada ne čita podatke iz `lights_modbus` pozadinskih fajlova na silu, već isključivo koristi metode `LIGHTS_GetInstance()` prateći "Opaque Pointer" arhitekturu čitavog projekta! 

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`display.c` / `display.h`):**
1. **Zatečeno Backend Stanje**: GUI kod je masivno opterećen (cca 12.000 linija). Pojedinačni pod-ekrani su briljantno vođeni praksama (`DSP_Init`, `Service`, `DSP_Kill`), a memory lekovi su dovedeni na minimum rigoroznim provjeravanjem emWin memorijskih redova.
2. **Novi Zahtjevi i Gap Analiza**: Zbog iznimne monolitnosti, fail dostigao granicu skalabilnosti.

---

## 6. PLAN TRANZICIJE: Modularna Arhitektura (Fragmentacija `display.c`)

Budući da je trenutni kôd fantastično organizovan i pridržava se grupisanja (Init, Service i Kill za svaki od ekrana), tranzicija u modularnu formu neće poremetiti memorijske ni izvedbene performanse aplikacije, već će radikalno smanjiti build-time i olakšati dodavanje UI komponenti (poput čarobnjaka za scene).

### Prijedlog Strukture Direktorija
Ideja je kreirati novi poddirektorij isključivo za vizuelne servise:
- `IC/Src/GUI/` (gdje idu `.c` fajlovi)
- `IC/Inc/GUI/` (gdje idu novi lokalni pre-processor scope `.h` fajlovi)

### Prijedlog Fragmentacije po Modulima

1. **`GUI_Core.c` (bivši `display.c`)**
   - **Funkcija**: Kontejner koji drži glavnu rutu. Poziva OS rutine bitnih widgeta.
   - **Zadržava**: 
     - `PID_Hook()` i `HandleTouchPressEvent` (skener prepoznavanja prsta dodirom i prosljeđivanje na ekrane)
     - `DISP_Service()` - Main switcher na loop preko `eScreen` mašine!
     - Svi generalni timer-i (poput onoga za reset ekrana na početnu liniju `Handle_PeriodicEvents`).

2. **`GUI_MainNavigation.c`**
   - **Ekrani**: `SCREEN_MAIN`, `SCREEN_SELECT_1`, `SCREEN_SELECT_2`, `SCREEN_SELECT_3`
   - **Odgovornost**: Prikaz vremena i datuma, "Hamburger i navigacijskih" dugmadi, centralno iscrtavanje glavnog Smart-Grida sa ikonicama podsistema.

3. **`GUI_Lights.c`**
   - **Ekrani**: `SCREEN_LIGHTS` (Grid sa sijalicama), `SCREEN_LIGHT_SETTINGS` (pop-up panel za setovanja Dimer i RGB boja preko UI okvira).

4. **`GUI_Thermostat.c`**
   - **Ekrani**: `SCREEN_THERMOSTAT`
   - **Odgovornost**: Crtanje krugova za termostat (temperatura + slajderi), i logike pritiska za plus, minus i Power mode kontrole sa ventilatorima.

5. **`GUI_CurtainsGate.c` (ili razdvojeno zavisno od veličine)**
   - **Ekrani**: `SCREEN_CURTAINS`, `SCREEN_GATE`, `SCREEN_GATE_SETTINGS`, `SCREEN_GATE_CONTROL_PANEL`
   - **Odgovornost**: Iscrtavanje upravljača pozicija po zadanom "Jalousie" i Gate modulu (otvori/zatvori/stop trostruka emWin dugmad).

6. **`GUI_SecurityTimer.c`**
   - **Ekrani**: `SCREEN_SECURITY`, `SCREEN_ALARM_ACTIVE`, `SCREEN_TIMER`
   - **Odgovornost**: Modul za manipulaciju alarma (aktivacija, unos tastera PIN koda, prikazi grešaka), menadžment globalnog timer-a sa izborom buđenja danima.

7. **`GUI_Keyboards.c`**
   - **Ekrani**: `SCREEN_KEYBOARD_ALPHA`, `SCREEN_NUMPAD`
   - **Odgovornost**: Dva moćna ekrana koja prenose unos tipkovnice, crtanje Array Key-layout mreže slova prema jeziku, te callback popunjavanje `g_keyboard_result`.

8. **`GUI_Settings.c`**
   - **Ekrani**: `SCREEN_SETTINGS_1` do `SCREEN_SETTINGS_9`, plus Datum/Vrijeme, Tematika, i Jezik.
   - **Odgovornost**: Najgabaritniji modul (postavke parametara rasvjete u listama sa spin-boxovima, PIN postavke, konfiguracija kapija).

9. **`GUI_ScenesUI.c` (u sklopu dodavanja nove funkcionalnosti)**
   - **Ekrani**: `SCREEN_SCENE`, i novo-izmišljeni `SCREEN_SCENE_WIZ_...` (Čarobnjak).

10. **`GUI_SkinManager.c` (Theming System)**
    - **Ekrani**: `SCREEN_SKIN` (Skin prozor pozvan dugim pritiskom na ikonu #4).
    - **Odgovornost**: Dinamičko upravljanje setovima ikona. Korištenje QSPI memory-mapped adresa za promjenu vizuelnog identiteta (Skin-a) u realnom vremenu bez restarta MCU-a.

### Faze Implementacije
- **Faza 1**: Učitavanje zaglavlja koda (Headeri) u CubeIDE projektu, definicija `/GUI/` sub-strukture.
- **Faza 2**: Iterativno ekstrakcija (Init / Service / Kill rutine) iz ekrana jedan po jedan.
- **Faza 3**: Provjera Linkanja (externing preklapanja stanja). 
- **Faza 4**: Potvrda kompajliranja i prečišćavanje mrtvog koda.
- **Faza 5**: Implementacija `GUI_SkinManager`-a i prelazak na dinamičko mapiranje QSPI resursa (Icon Sets).

