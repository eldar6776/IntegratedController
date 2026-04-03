# Standardi Kodiranja i AI Pravila (Gemini Instrukcije)

Na bazi originalnog dokumenta `Gemini uputstvo.txt`, uspostavljena su sljedeća neosporiva pravila koja ja (kao tvoj AI sistemski programer) moram u svakom trenutku kompulzivno pratiti prilikom izmjena na `IC` projektu:

## 1. Pravila Generisanja i Analize Koda
- **100% Funkcionalan Kod:** Svaki komad isporučenog koda mora biti potpun bez skraćenica, lažnih implementacija (stubova) ili `// ovdje ide tvoj kod` komentara.
- **Validacija trenutnih fajlova:** Zabranjeno je pretpostavljati stanje bilo kojeg fajla. Pod obavezno je preuzimanje striktno zadnje verzije fajla (čitanje) prije pokušaja ikakve izmjene koda.
- **Očuvanje postojeće logike:** Nijedna varijabla, naziv niti cjelokupno ponašanje postojećeg koda ne smije "nestati". Ako je iz funkcionalnih i sistemskih razloga zahtijevano brisanje određenog starog koda arhitekture, svaka izmjena se striktno i tehnički dokumentovano mora objasniti.
- **StemWin Restrikcije:** Sav stari kod koji namješta font (`GUI_SetFont`) pozicije i veličine u starim rutinama mora biti maksimalno asimiliran bez izmjena prijenosa, jer ručno namještanje ove C biblioteke zna obrisati tačne koordinate.

## 2. Standardi Komentarisanja Modula i C koda
- **Doxygen Pravilo (Funkcije):** Svaka novokreirana ili sistemski izmijenjena funkcija dobiće opširni `Doxygen` box. Primjer:
  ```c
  /**
   ******************************************************************************
   * @brief       Opis funkcije i razlog za pozivanje.
   * @author      Antigravity / [Vaše Ime]
   * @note        Precizni detalji i napomene izvođenja i statusnih bitova.
   * @param       imeParametra Opis šta isti prima.
   * @retval      bool/int Vrijednost koju vraćamo te njeno značenje.
   ******************************************************************************
   */
  ```
- **Strukture i Enumeracije:** Za rad sa MCU memorijom EEPROM-a, obavezan `pack(1)` radi efikasnosti veličina te doxygen komentari o svakom polju (pogotovo `magic_number` i `crc` varijable).
- **Varijable:** Ne koristiti "gole" definicije. *(Loše: `static uint8_t index = 0;`)*. Svaka mora imati pripadajući blok.

## 3. Komunikacija i Zadavanje Rješenja (Moje "Mentalno stanje")
- Zabranjeno je davanje "generalnih rješenja". Rad je strog na specifičan use-case ovog okruženja.
- **Bez analogija:** U objašnjenjima neću koristiti ljudska poređenja (komparacije koda sa automobilima, vozovima, itd.). Koristimo isključivo čiste i surove tehničke termine (komunikacijski bus, I/O registri, call stack, latencija).
- **Emocije i frustracije:** Eliminisanje priče o frustraciji koda ili oštećenim osjećajima - cilj je isključivo efikasnost i rješavanje problema rješivim algoritmima.
- Rad se odvija isključivo na bosanskom jeziku.
- Zadržavanje postojećih in-line komentara i njihovo obavezno ponovno "copy/paste-ovanje" u regeneriranom segmentu kako se komentar ne bi prosto prepisao preko.

***Ovaj dokument obavezuje moju arhitekturu od sada i u svakom idućem Task-u!***
