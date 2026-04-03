# FSD: Alfanumerička Tastatura & i18n
*(Patch i ispravke postojećeg UI-ja)*

## 1. Opis Očekivanog Ponašanja Modula (KeyboardScreen)
Namijenjen rješavanju specifičnih bugova postojećeg alfa-numeričkog koda i dodavanje napredne "Dugi pritisak - Kontinuirano brisanje" funkcionalnosti, kao i Bosanskog Layout-a.

## 2. Sistemske Promjene i Unaprjeđenja
1. **Rješavanje greške dugmeta "OK"**: Ometa snimanje adresa. Sistem mora vratiti generičku vrijednost "OK", prosljeđujući uništen GUI kontekst u parent caller instancu preko `is_confirmed = true`.
2. **Specijalni Karakteri**: Promjeniti hardcodovani GUI Font u varijantu "GUI_FontVerdana20_LAT" da omogući rad *š, č, ć, ž, đ* oznakama.
3. **Automatsko brisanje `clear_on_first_input`**: Flag unutar tastature koji provjerava da li prvi pritisnut taster treba da prvo automatski izbriše bafer `Naziv Sistema`.
4. **Kontinuirano Brisanje ("DEL" Holding)**: Preplitanjem sa tajmerom pritiska provodi se backspace petlja brisanja karaktera sve dok se backspace taster ne pusti.

## 3. Internacionalizacija
Kompletan prevodni mod sistema prolazi kroz mapu u backendu `language_strings` unutar config. Modifikacije alarma su u potpunosti podržane, kao i svi TextID podaci vezani za novije module (Gate, Timer, itd.).
