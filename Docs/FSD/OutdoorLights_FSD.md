# FSD: Outdoor Lights (Vanjska Rasvjeta)

## 1. Opis Funkcionalnosti
Modul upravlja sa 10+ vanjskih potrošača podijeljenih u 3 logičke zone. Podržava astronomski sat (Sunrise/Sunset) i integraciju sa modulom kapije i alarma. Centralizuje kontrolu vanjskog ambijenta uz maksimalnu energetsku efikasnost.

## 2. Korisnički Interfejs (SCREEN_OUTDOOR)
Klikom na ikonu `OUTDOOR` na navigacionom ekranu otvara se namjenski ekran sa **4 ikone (2x2 matrica)**:
1. **FASADA**: Dekorativna rasvjeta. Prati Astro-sat i Night-Cap.
2. **DVORIŠTE**: Ambijentalna rasvjeta (vrt, bazen).
3. **PUTANJA**: Rasvjeta prilaza i staza. Integrisana sa kapijom.
4. **POSTAVKE**: Ikona zupčanika koja vodi na `SCREEN_SETTINGS_OUTDOOR`.

## 3. Korisnička Podešavanja (SCREEN_SETTINGS_OUTDOOR)
Ovaj ekran je dizajniran da bude interaktivan i jednostavan za vlasnika:
- **[Toggle] Automatski rad**: Glavni prekidač za aktivaciju Astro-tajmera.
- **[Spinbox] Pomak paljenja**: Minute (+/-) u odnosu na izračunati zalazak sunca.
- **[Spinbox] Pomak gašenja**: Minute (+/-) u odnosu na izračunati izlazak sunca.
- **[Toggle] Noćna štednja**: Aktivacija **Night-Cap** funkcije.
- **[Sat:Min] Vrijeme štednje**: Fiksno vrijeme gašenja dekorativne rasvjete (npr. 01:30 AM).
- **DINAMIČKI STATUS**: Tekstualna informacija na dnu ekrana, npr: 
  *"Status: Svjetla se pale u 19:42 (za 35 min)"* ili *"Status: Aktivna noćna štednja"*.

## 3a. Sistemska/Instalaterska Podešavanja (Skriveni meni)
Ove opcije se konfigurišu prilikom montaže i nisu vidljive u običnom korisničkom meniju:
- **Geografske koordinate**: Lat/Long za precizan proračun sunca.
- **Mapiranje zona**: Povezivanje RS485 adresa sa zonama (Fasada, Dvorište, Putanja).
- **All-Off Lista**: Definisanje koje se adrese gase na globalnu `ALL_OFF` komandu.

## 4. Tehnološke Specifikacije i Podaci
- **Struktura**: `Outdoor_EepromConfig_t` čuva zone, adrese, astro-podatke i CRC.
- **Astro Engine**: `Outdoor_CalculateAstro()` računa Sunrise/Sunset za tekući dan.
- **Service**: `Outdoor_Service()` svake minute provjerava uslove i šalje komande preko `rs485.c`.
- **Integracija**:
    - `Outdoor_OnGateEvent()`: Pali zonu "Putanja" na signal iz `gate.c`.
    - `Outdoor_OnAlarmEvent()`: Pokreće blinkanje svih vanjskih svjetala u slučaju provale.

## 5. Trenutno Implementirano Stanje (Status)
1. **Backend**: `outdoor.c` kôd trenutno **ne postoji**. Potrebno je razviti logiku proračuna sunca i menadžment zona.
2. **Frontend**: Pripremljene su ikonice i definicije ekrana u `display.c`.
3. **Plan**: Iskoristiti postojeći kôd za Sarajevo Astro-sat i integrisati ga u modularni framework.

