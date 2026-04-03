# FSD: Rasvjeta i Lokalni PLC Komutatori

## 1. Opis Funkcionalnosti
Centralizovana operativna komponenta bazirana na potpunoj Opaque Handle enkapsulaciji koja manipuliše svim binarnim relejima, PWM (DALI / DMX i slično) i I2C lokalnim pinovima ploče zaduženim za osvjetljenje. Omogućava logičku razdvojenost na Frontend matricu i snažan Backend niz, i sadrži tajmerske i macro sisteme.

## 2. Sistemske Funkcije i Primitive
Svako svjetlo kroz svoj niz može imati potpuno odvojen "PLC" doživljaj:
- Parametrizacija `local_pin` uvezuje hardverski izlaz STM32 pinova za direktno prekidanje na PCB-u umjesto slanja komandi putem Modbusa.
- Modbus komunikacja se obavlja slanjem komandi na namjenske redove sa prioritetima (`binaryQueue`, `dimmerQueue`, `rgbwQueue`). 
- Funkcionalnost `button_external` postavlja "FLIP/ON/OFF" opciju koja se preko `HandleExternalButtonActivity()` detektuje ukoliko mehanički senzor baci High signal – idealno za primitivna hardverska dugmad smještena izvan kontrolera ekrana.

## 3. Pametno Čuvanje Stanja (EEPROM Wear-Leveling)
Razvijen je tajmerski mehanizam koji zaštićuje memoriju:
- Prilikom dinamične promjene slajdera za Brightness/RGB, ne vrši se stotinjak EEPROM upisa, već se postavlja `is_dirty_for_saving` fleg.
- `save_brightness_timer_start` odgodi upis preko EE_GATES memorije tačno 5 sekundi od posljednje poslane promjene.

## 4. Defragmentacija i UI Sinhronizacija
GUI prikazuje komponente iz niza, ali brisanje srednjih članova otežava iterabilnu RAM petlju. C modul automatski defragmentira konfiguracije (`DefragmentLights`) i prebacuje validne članove ka nultom indeksu prije trajne izmjene u EEPROM-u!

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`lights.c` / `lights.h`):**
1. **Zatečeno Backend Stanje**: Potvrđeno operaciono stanje sa visokokvalitetnom if-then state mašinom. Funkcije za globalni `LIGHT_NIGHT_TIMER_DURATION` podržavaju gašenje svih zavezanih svjetala (`tiedToMainLight`). Tajmerske odgode "Delayed on" uspješno izvršavaju aktivaciju naknadnih kontrolera preko iste mašine.
2. **Novi Zahtjevi i Gap Analiza**: 
   - Backend je 100% zreo, implementiran, hibridan za lokalne portove i RS485! 
   - Posjeduje opcije u EEPROM bloku poput `sleep_time` koje kako izvor komentariše u `.h` fajlu trenutno ne koristi nikakav mehanizam te su rezervisane za vizuelne dodatke.
3. **Frontend**: Ovaj modul zahtijeva detaljnu grid pretragu prilikom ulaska u Display kod u pogledu kako se vrši dodjeljivanje ovih primitivnih vrijednosti preko panela za opcije jer sadrži apsolutno najveći broj `EepromConfig` varijabli u cijelom domenu koda.
