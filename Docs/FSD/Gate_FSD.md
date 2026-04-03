# FSD: Modul Kapija i Vrata (Gate Control)

## 1. Opis Funkcionalnosti
Omogućava univerzalnu logiku za upravljanje različitim barijerama kroz jedinstvenu Universalnu State Mašinu (USM) i biblioteku tipova. Kontrolisani i podržani uređaji obuhvataju:
- Krilne i klizne kapije
- Garažna vrata
- Rampe 
- Pješačke kapije i sigurnosne brave (Smart Locks)

## 2. Ponašanje i Pametna Kontrola
Ponašanje svakog uređaja nije ugrađeno tvrdim kodiranjem u logiku, već je definisano u "Biblioteci Profila Kontrole" (`ProfilDeskriptor_t`). Promjena tipa motora mijenja jedino mapni odabir ("Profile id") u kodu (npr. BFT S-S step by step).
To osigurava da Universal State Mašina uzme profil i donese odluke šta čita kao feedback i koje releje pali.

### 2.1 Podrazumijevane komande za vrste uređaja
Dostupne komande iz korisničkog interfejsa na bazi profila:
- Potpuno Otvori / Zatvori komande
- Pješak / Djelomično otvaranje
- Stop komanda (Pulse signal)
- Smart Toggle / Step-By-Step (Jedno dugme rotira kroz Otvori-Stop-Zatvori)

### 2.2 Reakcije interfejsa (Dashboard SCREEN_GATE)
Ikone i komande oponašaju modul rasvjete, klikom na prikazanu ikonu vrši se "Toggle" ponašanje, gdje ikonica trenutno preslikava izmjenjeno traženo stanje, dajući osjećaj trenutnog odgovora na izvršenu naredbu. Ne čekaju se callback-ovi za prikaz promjene, ali u slučaju izostanka signala otvoriće se alarmni "DEVICE FAULT".

## 3. Podešavanja (Settings Meni)
Ekran dijeli identičnu logiku i layout postavkama modula rasvjete.
- Nudi odabir vizuelnih ikona (`ICON_GATE_SWING`, `ICON_GATE_SLIDING`, itd.)
- Podešava se Modbus adresa uređaja, Control_Type ID
- Dodatna parametrika podesivih Cycle Delay-a i Pulse Delay-a. 
- Opcije koje ne pripadaju odabranom profilu mašine, prebacuju se u "Disabled" (Sivo) vizuelno stanje bez nestajanja elemenata.

## 4. Tehnološke Specifikacije i Podaci
Struktura pohrane: `Gate_EepromConfig_t`. Profil za mapiranje relleja i funkcija kontrolisan je varijablom `control_type`.  
Biblioteka Profila (`g_ControlProfileLibrary[]`) uključuje mapping za Step-By-Step logiku, izričite komande i Brave.
Uređaj se očitava sa 3 potencijalna digitalna ulaza koja su mapirana na: Krajnji Senzor gore, Krajnji Dolje, Signalno Svjetlo ("Rotacija/Blic Lampa"). 

## 5. Trenutno Implementirano Stanje (Status & Napomene)
**Zaključak analize (`gate.c` / `gate.h`):**
1. **Zračenje dizajna (USM)**: Implementiran je genijalni "Device Descriptor" pristup. Sistem sadrži Universalnu State Mašinu (USM) potpomognutu kroz `g_ControlProfileLibrary` koji presijava "pametne" logike ponašanja za obijekte u letu (NICE rutine, S-S rutine, Smart Lock). U backendu je u potpunosti podržana razlika između `GATE_TIMER_PULSE` i `GATE_TIMER_CYCLE` što omogućava elegantno preskakanje faza (brava samo okida impuls i gotova je, dok kapija ima timeout vožnje).
2. **Event Driven Logika**: Modul direktno prihvata status o limitnim senzorskim čitačima preko `GATE_BusEvent` te bez delay-a okida interno obavještavanje o zaustavljanju (`HandleSensorEvent`), dok timeout `Gate_Service()` bdi nad greškama motora.
3. **Frontend (Čeka Analizu)**: Tražimo animirane ikone i toggle ponašanje dugmadi kroz display grid logiku u sklopu generalne analize `display.c` modula (provjera maski za sakrivanje spinboxeva u čarobnjaku/postavkama zavisno na `visible_settings_mask`).
