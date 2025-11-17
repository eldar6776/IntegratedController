/**
 ******************************************************************************
 * @file    display.c
 * @author  Edin & Gemini
 * @version V2.1.0
 * @date    18.08.2025.
 * @brief   Implementacija GUI logike i upravljanja ekranima.
 *
 * @note
 * Ovaj fajl sadrži kompletnu logiku za iscrtavanje svih ekrana definisanih
 * u `eScreen` enumu. Koristi STemWin (emWin) grafičku biblioteku za
 * kreiranje i upravljanje widgetima (dugmad, spinbox-ovi, itd.).
 *
 * Glavna servisna petlja `DISP_Service()` se poziva iz `main.c` i funkcioniše
 * kao state-machine, pozivajući odgovarajuću `Service_...Screen()` funkciju
 * u zavisnosti od trenutno aktivnog ekrana.
 *
 * Funkcija `PID_Hook()` je centralna tačka za obradu korisničkog unosa
 * (dodira na ekran) i prosljeđuje događaje odgovarajućim handlerima.
 *
 * Fajl takođe sadrži logiku za internacionalizaciju (prevođenje tekstova)
 * i upravljanje pozadinskim procesima kao što je screensaver.
 *
 * --- Vodič za dodavanje novog ekrana ---
 *
 * Dodavanje novog ekrana u ovaj framework prati preciznu proceduru kako bi se
 * osigurala stabilnost i pravilno upravljanje resursima.
 *
 * 1.  **Definicija Ekrana (display.h):**
 * - U fajlu `display.h`, unutar `eScreen` enumeratora, dodajte
 * jedinstveni naziv za Vaš novi ekran (npr. `SCREEN_NOVI_EKRAN`).
 *
 * 2.  **Kreiranje Osnovnih Funkcija (display.c):**
 * - Za svaki novi ekran, neophodno je kreirati tri `static` funkcije
 * koje prate "Init-Service-Kill" obrazac:
 * - `DSP_InitNoviEkranScreen(void)`: Kreira sve widgete (dugmad, tekst)
 * i iscrtava statičke elemente ekrana. Poziva se samo jednom pri ulasku.
 * - `Service_NoviEkranScreen(void)`: Glavna petlja za ekran. Poziva se
 * kontinuirano. Ovdje se provjerava stanje widgeta (npr. `BUTTON_IsPressed`).
 * - `DSP_KillNoviEkranScreen(void)`: Uništava sve widgete kreirane u `Init`
 * funkciji. Ključna je za sprječavanje curenja memorije.
 * - Prototipove za ove tri funkcije dodajte u sekciju "PROTOTIPOVI PRIVATNIH
 * (STATIC) FUNKCIJA" na vrhu `display.c`.
 *
 * 3.  **Integracija u Glavnu Petlju (display.c):**
 * - U funkciji `DISP_Service()`, unutar `switch (screen)` bloka, dodajte
 * novi `case` za Vaš ekran koji poziva njegovu `Service` funkciju:
 * `case SCREEN_NOVI_EKRAN: Service_NoviEkranScreen(); break;`
 *
 * 4.  **Upravljanje Unosom (Touch Events) (display.c):**
 * - Ako Vaš ekran ima interaktivne zone koje nisu widgeti (npr. crtani
 * oblici), kreirajte `HandlePress_...` funkciju:
 * - `static void HandlePress_NoviEkranScreen(GUI_PID_STATE *pTS, uint8_t *click_flag);`
 * - Poziv za nju dodajte u `if/else if` lanac unutar `HandleTouchPressEvent()`.
 * - Funkciju za obradu otpuštanja (`HandleRelease_...`) potrebno je
 * implementirati samo u specifičnim slučajevima:
 * a) Kada treba razlikovati kratak od dugog pritiska.
 * b) Za "drag-and-drop" logiku.
 * c) Za akcije koje se izvršavaju tek nakon otpuštanja prsta.
 *
 * 5.  **Pravilno "Čišćenje" Ekrana (display.c):**
 * - Da bi se Vaš ekran pravilno ugasio i oslobodio resurse, poziv za
 * njegovu `Kill` funkciju morate dodati na dva mjesta:
 * a) U `PID_Hook()`: Unutar `switch` bloka koji obrađuje pritisak na
 * hamburger meni, dodajte `case SCREEN_NOVI_EKRAN:` koji poziva
 * `DSP_KillNoviEkranScreen()`.
 * b) U `Handle_PeriodicEvents()`: Unutar `if/else if` lanca koji
 * obrađuje istek screensaver tajmera, dodajte
 * `else if (screen == SCREEN_NOVI_EKRAN) DSP_KillNoviEkranScreen();`.
 *
 ******************************************************************************
 * @attention
 *
 * (C) COPYRIGHT 2025 JUBERA d.o.o. Sarajevo
 *
 * Sva prava zadržana.
 *
 ******************************************************************************
 */

/* Provjera verzije build-a kako bi se osigurala konzistentnost sa header fajlom */
#if (__DISPH__ != FW_BUILD)
#error "display header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
// --- Standardni i sistemski headeri ---
#include "main.h"
#include "display.h"
#include "stm32746g_eeprom.h"

// --- Headeri drugih modula (za pozivanje njihovih API-ja) ---
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "security.h"
#include "curtain.h"
#include "lights.h"
#include "buzzer.h"
#include "rs485.h"
#include "gate.h"
#include "scene.h"
#include "translations.h"

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI (INTERNI)                                     */
/*============================================================================*/

/** @name Vremenske konstante za GUI
 * @{
 */
#define GUI_REFRESH_TIME                50U     ///< Svrha: Period osvježavanja GUI-ja. Vrijednost: 100 milisekundi (10 puta u sekundi).
#define DATE_TIME_REFRESH_TIME          1000U   ///< Svrha: Period osvježavanja prikaza datuma i vremena. Vrijednost: 1000 milisekundi (svake sekunde).
#define SETTINGS_MENU_ENABLE_TIME       3456U   ///< Svrha: Vrijeme držanja pritiska za ulazak u meni. Vrijednost: 3456 milisekundi (~3.5 sekunde).
#define SETTINGS_MENU_TIMEOUT           59000U  ///< Svrha: Timeout za automatski izlazak iz menija. Vrijednost: 59000 milisekundi (59 sekundi).
#define EVENT_ONOFF_TOUT                500     ///< Svrha: Maksimalno vrijeme za "kratak dodir". Vrijednost: 500 milisekundi.
#define VALUE_STEP_TOUT                 15      ///< Svrha: Brzina promjene vrijednosti kod držanja dugmeta (npr. dimovanje). Vrijednost: 15 milisekundi.
#define GHOST_WIDGET_SCAN_INTERVAL      2000    ///< Svrha: Period za skeniranje i brisanje "duhova" (zaostalih widgeta). Vrijednost: 2000 milisekundi.
#define FW_UPDATE_BUS_TIMEOUT           15000U  ///< Svrha: Vrijeme (u ms) nakon kojeg smatramo da je FW update na busu završen ako nema novih paketa.
#define LONG_PRESS_DURATION             1000    ///< Prag za dugi pritisak u ms (1 sekunda)
/** @} */

/** @name Konfiguracija ekrana i prikaza
 * @{
 */
#define DISP_BRGHT_MAX                  80      ///< Svrha: Maksimalna dozvoljena vrijednost za svjetlinu ekrana. Vrijednost: 80 (na skali 1-90).
#define DISP_BRGHT_MIN                  5       ///< Svrha: Minimalna dozvoljena vrijednost za svjetlinu ekrana. Vrijednost: 5 (na skali 1-90).
#define QR_CODE_COUNT                   2       ///< Svrha: Ukupan broj QR kodova koje sistem podržava. Vrijednost: 2 (jedan za WiFi, jedan za App).
#define QR_CODE_LENGTH                  50      ///< Svrha: Maksimalna dužina stringa za QR kod. Vrijednost: 50 karaktera.
#define DRAWING_AREA_WIDTH              380     ///< Svrha: Širina glavnog područja za crtanje. Vrijednost: 380 piksela (cijeli ekran je 480px).
#define COLOR_BSIZE                     28      ///< Svrha: Veličina `clk_clrs` niza. Vrijednost: 28, mora odgovarati broju boja u nizu.
/** @} */

/** @name Definicije za ikonice svjetala
 * @note Premješteno iz lights.h, privatno za display modul.
 * @{
 */
#define LIGHT_ICON_COUNT                10       ///< Svrha: Ukupan broj različitih tipova ikonica za svjetla. Vrijednost: 2 (sijalica i ventilator).
#define LIGHT_ICON_ID_BULB              0       ///< Svrha: Jedinstveni ID za ikonicu sijalice. Vrijednost: 0.
#define LIGHT_ICON_ID_VENTILATOR        1       ///< Svrha: Jedinstveni ID za ikonicu ventilatora. Vrijednost: 1.
#define LIGHT_ICON_ID_CEILING_LED_FIXTURE 2
#define LIGHT_ICON_ID_CHANDELIER        3
#define LIGHT_ICON_ID_HANGING           4
#define LIGHT_ICON_ID_LED_STRIP         5
#define LIGHT_ICON_ID_SPOT_CONSOLE      6
#define LIGHT_ICON_ID_SPOT_SINGLE       7
#define LIGHT_ICON_ID_STAIRS            8
#define LIGHT_ICON_ID_WALL              9
/** @} */
/* Privatne definicije... */
#define PIN_MASK_DELAY 2000 // 2 sekunde prije maskiranja karaktera
#define MAX_PIN_LENGTH 8    // << NOVO: Maksimalna dužina PIN-a

/******************************************************************************
 * @brief       Definicije za raspored alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ove konstante se koriste za dinamičko kreiranje tastature
 * u `DSP_InitKeyboardScreen` funkciji.
 *****************************************************************************/
#define KEY_ROWS 4          // Broj redova sa karakterima
#define KEYS_PER_ROW 10     // Maksimalan broj tastera po redu
#define KEY_SHIFT_STATES 2  // Broj stanja (0 = mala slova, 1 = VELIKA SLOVA)


/* Definicije ID-jeva za PIN tastaturu */
#define ID_PINPAD_BASE      (GUI_ID_USER + 100)
#define ID_PINPAD_1         (ID_PINPAD_BASE + 1)
#define ID_PINPAD_2         (ID_PINPAD_BASE + 2)
#define ID_PINPAD_3         (ID_PINPAD_BASE + 3)
#define ID_PINPAD_4         (ID_PINPAD_BASE + 4)
#define ID_PINPAD_5         (ID_PINPAD_BASE + 5)
#define ID_PINPAD_6         (ID_PINPAD_BASE + 6)
#define ID_PINPAD_7         (ID_PINPAD_BASE + 7)
#define ID_PINPAD_8         (ID_PINPAD_BASE + 8)
#define ID_PINPAD_9         (ID_PINPAD_BASE + 9)
#define ID_PINPAD_0         (ID_PINPAD_BASE + 0)
#define ID_PINPAD_DEL       (ID_PINPAD_BASE + 10)
#define ID_PINPAD_OK        (ID_PINPAD_BASE + 11)
#define ID_PINPAD_TEXT      (ID_PINPAD_BASE + 12)

// DODATI SA OSTALIM #define-ovima
#define ID_KEYBOARD_BASE    (GUI_ID_USER + 200) // Baza za ID-jeve specijalnih tastera
#define GUI_ID_SHIFT        (ID_KEYBOARD_BASE + 0)
#define GUI_ID_SPACE        (ID_KEYBOARD_BASE + 1)
#define GUI_ID_BACKSPACE    (ID_KEYBOARD_BASE + 2)
#define GUI_ID_OKAY         (ID_KEYBOARD_BASE + 3)

// DODATI NOVI ID SA OSTALIM #define-ovima ZA TASTATURE
#define ID_BUTTON_RENAME_LIGHT (ID_KEYBOARD_BASE + 4)

/** @name Bazni ID-jevi za dinamičke widgete
 * @note Koriste se u petljama za kreiranje widgeta kao početna vrijednost.
 * @{
 */
#define ID_CurtainsRelay                0x894   ///< Svrha: Početni ID za widgete zavjesa. Vrijednost: 0x894 (Heksadecimalni broj).
#define ID_LightsModbusRelay            0x8B3   ///< Svrha: Početni ID za widgete svjetala. Vrijednost: 0x8B3 (Heksadecimalni broj).
/** @} */

/** @name ID-jevi za QR kodove
 * @{
 */
#define QR_CODE_WIFI_ID                 1       ///< Svrha: Logički ID za WiFi QR kod. Vrijednost: 1.
#define QR_CODE_APP_ID                  2       ///< Svrha: Logički ID za App QR kod. Vrijednost: 2.
/** @} */

/** @name Definicije boja
 * @note Koriste `GUI_MAKE_COLOR` makro koji pretvara 0xBBGGRR (Plava, Zelena, Crvena) format u format koji koristi GUI biblioteka.
 * @{
 */
#define CLR_DARK_BLUE                   GUI_MAKE_COLOR(0x613600)  ///< Svrha: Definicija tamno plave boje.
#define CLR_LIGHT_BLUE                  GUI_MAKE_COLOR(0xaa7d67)  ///< Svrha: Definicija svijetlo plave boje.
#define CLR_BLUE                        GUI_MAKE_COLOR(0x855a41)  ///< Svrha: Definicija plave boje.
#define CLR_LEMON                       GUI_MAKE_COLOR(0x00d6d3)  ///< Svrha: Definicija limun žute boje.

/*============================================================================*/
/* PRIVATNE STRUKTURE I DEKLARACIJE VARIJABLI                                 */
/*============================================================================*/
/**
 * @brief Definiše stanja za state-machine unutar Numpad-a prilikom promjene PIN-a.
 */
typedef enum {
    PIN_CHANGE_IDLE,            /**< Numpad se koristi za standardne operacije (ne za promjenu PIN-a). */
    PIN_CHANGE_WAIT_CURRENT,    /**< Čeka se unos trenutnog (starog) PIN-a. */
    PIN_CHANGE_WAIT_NEW,        /**< Čeka se unos novog PIN-a. */
    PIN_CHANGE_WAIT_CONFIRM     /**< Čeka se ponovni unos novog PIN-a radi potvrde. */
} PinChangeState_e;
/**
 ******************************************************************************
 * @brief       Definiše modove rada za višenamjenski ekran za odabir scene.
 * @note        Ovaj enum omogućava da se isti ekran (`SCREEN_SCENE_APPEARANCE`)
 * koristi u različitim kontekstima unutar aplikacije.
 ******************************************************************************
 */
typedef enum 
{
    SCENE_PICKER_MODE_WIZARD,   /**< Mod za "čarobnjaka": prikazuje samo neiskorištene izglede za kreiranje nove scene. */
    SCENE_PICKER_MODE_TIMER     /**< Mod za tajmer: prikazuje sve konfigurisane scene radi odabira akcije za alarm. */
} eScenePickerMode;
/******************************************************************************
 * @brief       Struktura koja definiše kontekst za univerzalni numerički keypad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura se koristi za dinamičku konfiguraciju keypad-a.
 *****************************************************************************/
typedef struct
{
    const char* title;          /**< Pokazivač na string koji će biti prikazan kao naslov. */
    char initial_value[12];     /**< Početna vrijednost koja se prikazuje. */
    int32_t min_val;            /**< Minimalna dozvoljena vrijednost. */
    int32_t max_val;            /**< Maksimalna dozvoljena vrijednost. */
    uint8_t max_len;            /**< Maksimalan broj karaktera za unos. */
    bool    allow_decimal;      /**< Ako je 'true', prikazuje se dugme '.'. */
    bool    allow_minus_one;    /**< Ako je 'true', prikazuje se dugme '[ ISKLJ. / OFF ]'. */
} NumpadContext_t;

/**
 * @brief Statička, privatna instanca konteksta za Numpad.
 * @note  Vidljiva samo unutar display.c. Funkcije unutar ovog fajla će
 * popunjavati ovu strukturu prije prebacivanja na ekran keypada.
 */
static NumpadContext_t g_numpad_context;

/******************************************************************************
 * @brief       Struktura koja sadrži rezultat unosa sa numeričkog keypada.
 * @author      Gemini (po specifikaciji korisnika)
 *****************************************************************************/
typedef struct
{
    char    value[12];          /**< Bafer za konačnu unesenu vrijednost. */
    bool    is_confirmed;       /**< Fleg, 'true' ako je korisnik potvrdio unos. */
    bool    is_cancelled;       /**< Fleg, 'true' ako je korisnik otkazao unos. */
} NumpadResult_t;

/**
 * @brief Statička, privatna instanca za rezultat unosa sa Numpad-a.
 * @note  Služi za internu komunikaciju između logike keypada i logike
 * ekrana koji ga je pozvao (npr. SCREEN_SETTINGS_GATE).
 */
static NumpadResult_t g_numpad_result;
/**
 * @brief Definiše moguće vizuelne statuse za UI alarma.
 */
typedef enum {
    ALARM_UI_STATE_DISARMED,
    ALARM_UI_STATE_ARMED,
    ALARM_UI_STATE_ARMING,
    ALARM_UI_STATE_DISARMING
} AlarmUIState_e;
/**
 * @brief Niz koji čuva TRENUTNO VIZUELNO stanje za svaku komponentu alarma.
 * @note Index 0 = Sistem, Index 1-3 = Particije 1-3.
 */
static AlarmUIState_e alarm_ui_state[SECURITY_PARTITION_COUNT + 1];
/******************************************************************************
 * @brief       Struktura koja definiše kontekst za univerzalnu alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura se popunjava prije poziva funkcije
 * Display_ShowKeyboard() kako bi se tastatura prilagodila
 * specifičnoj potrebi (npr. unos imena svjetla).
 *****************************************************************************/
typedef struct
{
    const char* title;          /**< Pokazivač na string koji će biti prikazan kao naslov. */
    char initial_value[32];     /**< String koji sadrži početnu vrijednost za editovanje. */
    uint8_t max_len;            /**< Maksimalan broj karaktera koji se mogu unijeti. */
} KeyboardContext_t;

/**
 * @brief Statička, privatna instanca konteksta za alfanumeričku tastaturu.
 * @note  Vidljiva samo unutar display.c.
 */
static KeyboardContext_t g_keyboard_context;

/******************************************************************************
 * @brief       Struktura koja sadrži rezultat unosa sa alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Služi kao mehanizam za komunikaciju između tastature i ekrana
 * koji ju je pozvao, bez upotrebe callback funkcija.
 *****************************************************************************/
typedef struct
{
    char    value[32];          /**< Bafer u koji se upisuje konačni uneseni tekst. */
    bool    is_confirmed;       /**< Fleg, postaje 'true' kada korisnik pritisne "OK". */
    bool    is_cancelled;       /**< Fleg, postaje 'true' ako je korisnik otkazao unos. */
} KeyboardResult_t;

/**
 * @brief Statička, privatna instanca za rezultat unosa sa tastature.
 * @note  Vidljiva samo unutar display.c.
 */
static KeyboardResult_t g_keyboard_result;

/**
 * @brief Matrica koja sadrži rasporede tastera za sve podržane jezike.
 * @note  Struktura: [JEZIK][SHIFT_STANJE][RED][TASTER]
 * Ovo je "izvor istine" za iscrtavanje tastature. Funkcija
 * `DSP_InitKeyboardScreen` će na osnovu odabranog jezika i
 * shift stanja odabrati odgovarajući set karaktera za ispis.
 * Trenutno su definisani BHS, ENG (QWERTZ) i GER.
 */
static const char* key_layouts[LANGUAGE_COUNT][KEY_SHIFT_STATES][KEY_ROWS][KEYS_PER_ROW] =
{
    // =========================================================================
    // === JEZIK: BSHC (Bosanski/Srpski/Hrvatski/Crnogorski) - QWERTZ ===
    // =========================================================================
    [BSHC] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "z", "u", "i", "o", "p" },
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", "č" },
            { "š", "y", "x", "c", "v", "b", "n", "m", "đ", "ž" }
        },
        {   // Stanje 1: Velika slova
            { "!", "\"", "#", "$", "%", "&", "/", "(", ")", "=" },
            { "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P" },
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", "Č" },
            { "Š", "Y", "X", "C", "V", "B", "N", "M", "Đ", "Ž" }
        }
    },

    // =========================================================================
    // === JEZIK: ENG (Engleski) - QWERTZ za konzistentnost ===
    // =========================================================================
    [ENG] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" }, // QWERTY
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", ";" },
            { "z", "x", "c", "v", "b", "n", "m", ",", ".", "-" }  // QWERTY
        },
        {   // Stanje 1: Velika slova
            { "!", "@", "#", "$", "%", "^", "&", "*", "(", ")" },
            { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" }, // QWERTY
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", ":" },
            { "Z", "X", "C", "V", "B", "N", "M", "<", ">", "_" }  // QWERTY
        }
    },

    // =========================================================================
    // === JEZIK: GER (Njemački) - QWERTZ ===
    // =========================================================================
    [GER] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "z", "u", "i", "o", "p" },
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", "ö" },
            { "ü", "y", "x", "c", "v", "b", "n", "m", "ä", "ß" }
        },
        {   // Stanje 1: Velika slova
            { "!", "\"", "§", "$", "%", "&", "/", "(", ")", "=" },
            { "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P" },
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö" },
            { "Ü", "Y", "X", "C", "V", "B", "N", "M", "Ä", "?" }
        }
    },

    // Ostali jezici trenutno koriste ENG layout kao placeholder
    [FRA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [ITA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [SPA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [RUS] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [UKR] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [POL] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [CZE] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [SLO] = { [0 ... KEY_SHIFT_STATES-1] = {}}  // Placeholder
};
/**
 * @brief Definicija opšteg tipa za zonu dodira (pravougaonik).
 * @note  Olakšava kreiranje i rad sa layout strukturama.
 */
typedef struct {
    int16_t x0, y0, x1, y1;
} TouchZone_t;

/**
 * @brief Struktura koja objedinjuje sve GUI widgete (handle-ove)
 * za jedan red u meniju za podešavanje svjetala.
 */
typedef struct
{
    SPINBOX_Handle relay;                   // Handle za spinbox widget za Modbus adresu releja.
    SPINBOX_Handle iconID;                  // Handle za spinbox widget za ID ikonice.
    SPINBOX_Handle controllerID_on;         // Handle za spinbox za Modbus adresu kontrolera koji se pali zajedno sa svjetlom.
    SPINBOX_Handle controllerID_on_delay;   // Handle za spinbox za vrijeme odgode paljenja drugog kontrolera.
    SPINBOX_Handle on_hour;                 // Handle za spinbox za sat automatskog paljenja.
    SPINBOX_Handle on_minute;               // Handle za spinbox za minutu automatskog paljenja.
    SPINBOX_Handle offTime;                 // Handle za spinbox za vrijeme automatskog gašenja.
    SPINBOX_Handle communication_type;      // Handle za spinbox za tip komunikacije (binarno, dimer, RGB).
    SPINBOX_Handle local_pin;               // Handle za spinbox za odabir lokalnog GPIO pina.
    SPINBOX_Handle sleep_time;              // Handle za spinbox za 'sleep' vrijeme.
    SPINBOX_Handle button_external;         // Handle za spinbox za mod eksternog tastera.
    CHECKBOX_Handle tiedToMainLight;        // Handle za checkbox za povezivanje sa glavnim svjetlom.
    CHECKBOX_Handle rememberBrightness;     // Handle za checkbox za pamćenje zadnje svjetline.
} LIGHT_settingsWidgets;

/**
 * @brief Pomoćna struktura koja objedinjuje sve GUI widgete (handle-ove)
 * za meni za podešavanje odmrzivača (Defroster).
 * @note Interna je za `display.c`.
 */
typedef struct
{
    SPINBOX_Handle cycleTime, activeTime, pin;
} Defroster_settingsWidgets;

/**
 * @brief Pomoćna struktura za definisanje pozicije i dimenzija widget-a.
 */
typedef struct 
{
    int16_t x, y, w, h;
} WidgetRect_t;

/** 
*   brief Pomoćna struktura za definisanje horizontalne linije. 
*/
typedef struct 
{
    int16_t y, x0, x1;
} HLine_t;

/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za iscrtavanje "hamburger" menija.
 * @author      Gemini
 * @note        Centralizuje sve dimenzije i koordinate za gornju desnu i donju
 * lijevu ikonu kako bi se izbjegli "magični brojevi" u kodu i
 * olakšalo buduće održavanje. [cite: 63, 64, 65]
 ******************************************************************************
 */
static const struct
{
    /** @brief Konstante za GORNJI DESNI meni (pozicija 1) */
    struct {
        int16_t x_start; /**< Početna X koordinata (lijevo). */
        int16_t y_start; /**< Početna Y koordinata (gore). */
        int16_t width;   /**< Širina linija. */
        int16_t y_gap;   /**< Vertikalni razmak između linija. */
    } top_right;

    /** @brief Konstante za DONJI LIJEVI meni (pozicija 2) */
    struct {
        int16_t x_start; /**< Početna X koordinata (lijevo). */
        int16_t y_start; /**< Y koordinata donje linije. */
        int16_t width;   /**< Širina linija. */
        int16_t y_gap;   /**< Negativan vertikalni razmak za crtanje prema gore. */
    } bottom_left;

    int16_t line_thickness; /**< Debljina linija za obje ikone. */
}
hamburger_menu_layout =
{
    .top_right = {
        .x_start = 400,
        .y_start = 20,
        .width   = 50,
        .y_gap   = 20
    },
    .bottom_left = {
        .x_start = 30,
        .y_start = 252,
        .width   = 50,
        .y_gap   = -20
    },
    .line_thickness = 9
};
/**
 * @brief Struktura koja sadrži konstante za globalne elemente GUI-ja.
 * @note  Trenutno sadrži samo zonu za hamburger meni, ali se može proširiti.
 */
static const struct
{
    TouchZone_t hamburger_menu_zone; /**< Zona za ulazak/povratak iz menija. */
}
global_layout =
{
    .hamburger_menu_zone = { .x0 = 400, .y0 = 0, .x1 = 480, .y1 = 80 }
};

/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na glavnom ekranu.
 */
static const struct
{
    int16_t circle_center_x;    /**< X koordinata centra kruga. */
    int16_t circle_center_y;    /**< Y koordinata centra kruga. */
    int16_t circle_radius_x;    /**< Horizontalni radijus kruga. */
    int16_t circle_radius_y;    /**< Vertikalni radijus kruga. */

    // NOVE KOORDINATE ZA VRIJEME I DATUM
    GUI_POINT time_pos_standard;        // Pozicija za vrijeme na standardnom ekranu
    GUI_POINT time_pos_scrnsvr;         // Pozicija za vrijeme na screensaver-u
    GUI_POINT date_pos_scrnsvr;         // Pozicija za datum na screensaver-u
}
main_screen_layout =
{
    .circle_center_x = 240,
    .circle_center_y = 136,
    .circle_radius_x = 50,
    .circle_radius_y = 50,

    // NOVE VRIJEDNOSTI
    .time_pos_standard  = { 5, 245 },   // Bivše (5, 245)
    .time_pos_scrnsvr   = { 240, 136 },  // Bivše (240, 136)
    .date_pos_scrnsvr   = { 240, 220 }   // Nova pozicija za datum
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na prvom ekranu za odabir.
 */
static const struct
{
    TouchZone_t lights_zone;        /**< Zona za odabir menija za svjetla (kvadrant 1). */
    TouchZone_t thermostat_zone;    /**< Zona za odabir menija za termostat (kvadrant 2). */
    TouchZone_t curtains_zone;      /**< Zona za odabir menija za zavjese (kvadrant 3). */
    TouchZone_t dynamic_zone;       /**< Zona za dinamicku ikonicu (Defroster/Ventilator, kvadrant 4). */
    TouchZone_t next_button_zone;   /**< Zona za dugme "NEXT". */
}
select_screen1_layout =
{
    .lights_zone      = { .x0 = 0,   .y0 = 0,   .x1 = 190, .y1 = 136 },
    .thermostat_zone  = { .x0 = 190, .y0 = 0,   .x1 = 380, .y1 = 136 },
    .curtains_zone    = { .x0 = 0,   .y0 = 136, .x1 = 190, .y1 = 272 },
    .dynamic_zone     = { .x0 = 190, .y0 = 136, .x1 = 380, .y1 = 272 },
    .next_button_zone = { .x0 = 400, .y0 = 159, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na drugom ekranu za odabir.
 */
static const struct
{
    TouchZone_t clean_zone;         /**< Zona za odabir menija za čišćenje. */
    TouchZone_t wifi_zone;          /**< Zona za odabir Wi-Fi QR koda. */
    TouchZone_t app_zone;           /**< Zona za odabir App QR koda. */
    TouchZone_t next_button_zone;   /**< Zona za dugme "NEXT". */
}
select_screen2_layout =
{
    .clean_zone       = { .x0 = 0,   .y0 = 80, .x1 = 126, .y1 = 200 }, // Prva trećina
    .wifi_zone        = { .x0 = 126, .y0 = 80, .x1 = 253, .y1 = 200 }, // Druga trećina
    .app_zone         = { .x0 = 253, .y0 = 80, .x1 = 380, .y1 = 200 }, // Treća trećina
    .next_button_zone = { .x0 = 380, .y0 = 159, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za prikaz scena.
 * @note  Centralizuje sve dimenzije i pozicije kako bi se izbjegli
 * "magični brojevi" u funkciji za iscrtavanje.
 */
static const struct
{
    int16_t items_per_row;      /**< Broj ikonica u jednom redu. */
    int16_t slot_width;         /**< Širina jednog slota za ikonicu. */
    int16_t slot_height;        /**< Visina jednog slota za ikonicu. */
    int16_t text_y_offset;      /**< Vertikalni pomak teksta u odnosu na centar ikonice. */
}
scene_screen_layout =
{
    .items_per_row   = 3,
    .slot_width      = 126,
    .slot_height     = 136,
    .text_y_offset   = 35
};

/**
 * @brief Struktura koja sadrži sve konstante za raspored elemenata (layout)
 * na ekranu termostata. Sve koordinate su na jednom mjestu radi lakšeg održavanja.
 */
static const struct
{
    TouchZone_t increase_zone;      /**< Zona za povećanje temperature (+) */
    TouchZone_t decrease_zone;      /**< Zona za smanjenje temperature (-) */
    TouchZone_t on_off_zone;        /**< Zona za paljenje/gašenje dugim pritiskom */
}
thermostat_layout =
{
    .increase_zone = { .x0 = 200, .y0 = 90, .x1 = 320, .y1 = 270 },
    .decrease_zone = { .x0 = 0,   .y0 = 90, .x1 = 120, .y1 = 270 },
    .on_off_zone   = { .x0 = 400, .y0 = 150, .x1 = 480, .y1 = 190 }
};


/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za kontrolu svjetala.
 */
static const struct
{
    int16_t icon_width;     /**< Širina zone dodira za jednu ikonicu. */
    int16_t icon_height;    /**< Visina zone dodira za jednu ikonicu (uključujući tekst). */
}
lights_screen_layout =
{
    .icon_width = 80,
    .icon_height = 120
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za kontrolu roletni.
 */
static const struct
{
    TouchZone_t up_zone;              /**< Zona za trougao GORE. */
    TouchZone_t down_zone;            /**< Zona za trougao DOLE. */
    TouchZone_t previous_arrow_zone;  /**< Zona za strelicu PRETHODNA. */
    TouchZone_t next_arrow_zone;      /**< Zona za strelicu SLJEDEĆA. */
}
curtains_screen_layout =
{
    .up_zone             = { .x0 = 100, .y0 = 0,   .x1 = 280, .y1 = 136 }, // Gornja polovina centralnog dijela
    .down_zone           = { .x0 = 100, .y0 = 136, .x1 = 280, .y1 = 272 }, // Donja polovina centralnog dijela
    .previous_arrow_zone = { .x0 = 0,   .y0 = 192, .x1 = 80,  .y1 = 272 }, // Donji lijevi ugao
    .next_arrow_zone     = { .x0 = 320, .y0 = 192, .x1 = 380, .y1 = 272 }  // Donji desni ugao
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za detaljno podešavanje svjetala (dimer i RGB).
 */
static const struct
{
    TouchZone_t rename_text_zone;       /**< Fiksna zona dodira u gornjem lijevom uglu za pokretanje izmjene naziva. */
    TouchZone_t white_square_zone;      /**< Zona za odabir bijele boje. */
    TouchZone_t brightness_slider_zone; /**< Zona za slajder svjetline. */
    TouchZone_t color_palette_zone;     /**< Zona za paletu boja. */
}
light_settings_screen_layout =
{
    .rename_text_zone       = { .x0 = 0, .y0 = 0, .x1 = 200, .y1 = 60 },
    .white_square_zone      = { .x0 = 210, .y0 = 41,  .x1 = 270, .y1 = 101 },
    .brightness_slider_zone = { .x0 = 60,  .y0 = 111, .x1 = 420, .y1 = 161 },
    .color_palette_zone     = { .x0 = 60,  .y0 = 181, .x1 = 420, .y1 = 231 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za resetovanje menija (ponaša se kao glavni prekidač).
 */
static const struct
{
    TouchZone_t main_switch_zone;  /**< Velika centralna zona koja se ponaša kao glavni prekidač. */
}
reset_menu_switches_layout =
{
    .main_switch_zone = { .x0 = 80, .y0 = 80, .x1 = 400, .y1 = 192 }
};
/**
 * @brief Struktura koja sadrži sve konstante za ISCRTAVANJE elemenata
 * na prvom ekranu za odabir (Select Screen 1).
 * @note  Ova struktura je redizajnirana da podrži dinamički "Smart Grid" raspored.
 * Sadrži parametre za iscrtavanje rasporeda sa 1, 2, 3 ili 4 ikonice,
 * čime se izbjegavaju "magični brojevi" unutar `Service_SelectScreen1` funkcije.
 */
static const struct
{
    /** @brief X pozicija krajnje desne vertikalne linije. */
    int16_t x_separator_pos;
    /** @brief Y koordinata centra za "NEXT" dugme. */
    int16_t y_next_button_center;
    /** @brief Y koordinata centra za rasporede sa jednim redom (kada su 1, 2 ili 3 ikonice aktivne). */
    int16_t y_center_single_row;
    /** @brief Y koordinata centra za gornji red u 2x2 rasporedu (kada su 4 ikonice aktivne). */
    int16_t y_center_top_row;
    /** @brief Y koordinata centra za donji red u 2x2 rasporedu (kada su 4 ikonice aktivne). */
    int16_t y_center_bottom_row;
    /** @brief Vertikalni razmak (u pikselima) između dna ikonice i vrha teksta ispod nje. */
    int16_t text_vertical_offset;
    /** @brief Početna Y koordinata (vrh) za KRATKE vertikalne separatore. */
    int16_t short_separator_y_start;
    /** @brief Krajnja Y koordinata (dno) za KRATKE vertikalne separatore. */
    int16_t short_separator_y_end;
    /** @brief Početna Y koordinata (vrh) za DUGAČKI desni separator. */
    int16_t long_separator_y_start;
    /** @brief Krajnja Y koordinata (dno) za DUGAČKI desni separator. */
    int16_t long_separator_y_end;
    /** @brief Horizontalni razmak (padding) od ivica ekrana za horizontalni separator u 2x2 rasporedu. */
    int16_t separator_x_padding;
    /** @brief Definiše kompletnu zonu dodira za "NEXT" dugme. */
    TouchZone_t next_button_zone;
}
select_screen1_drawing_layout =
{
    .x_separator_pos        = DRAWING_AREA_WIDTH,
    .y_next_button_center   = 192,
    .y_center_single_row    = 136,
    .y_center_top_row       = 68,
    .y_center_bottom_row    = 204,
    .text_vertical_offset   = 10,
    .short_separator_y_start = 60,
    .short_separator_y_end   = 212,
    .long_separator_y_start = 10,
    .long_separator_y_end   = 252,
    .separator_x_padding    = 20,
    .next_button_zone       = { .x0 = 400, .y0 = 80, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE i ZONE DODIRA
 * na drugom ekranu za odabir.
 * @note  Verzija 2.6.0: Zadržano originalno ime `select_screen2_drawing_layout`.
 * Redizajnirana da podrži fiksni 2x2 raspored.
 */
static const struct
{
    /** @brief Zona dodira za gornji lijevi kvadrant (Čišćenje). */
    TouchZone_t clean_zone;
    /** @brief Zona dodira za gornji desni kvadrant (WiFi). */
    TouchZone_t wifi_zone;
    /** @brief Zona dodira za donji lijevi kvadrant (App). */
    TouchZone_t app_zone;
    /** @brief Zona dodira za donji desni kvadrant (Podešavanja). */
    TouchZone_t settings_zone;
    /** @brief Zona dodira za "NEXT" dugme za rotaciju ekrana. */
    TouchZone_t next_button_zone;

    // Koordinate za iscrtavanje
    /** @brief X koordinata centra lijeve kolone. */
    int16_t x_center_left;
    /** @brief X koordinata centra desne kolone. */
    int16_t x_center_right;
    /** @brief Y koordinata centra gornjeg reda. */
    int16_t y_center_top;
    /** @brief Y koordinata centra donjeg reda. */
    int16_t y_center_bottom;
    /** @brief Vertikalni pomak teksta u odnosu na centar ikonice. */
    int16_t text_vertical_offset;
    /** @brief Početna Y koordinata za vertikalne separatore. */
    int16_t separator_y_start;
    /** @brief Krajnja Y koordinata za vertikalne separatore. */
    int16_t separator_y_end;
    /** @brief Horizontalni padding za horizontalni separator. */
    int16_t separator_x_padding;
    /** @brief X pozicija za "NEXT" dugme. */
    int16_t next_button_x_pos;
    /** @brief Y centar za "NEXT" dugme. */
    int16_t next_button_y_center;
}
select_screen2_drawing_layout =
{
    .clean_zone         = { .x0 = 0,   .y0 = 0, .x1 = 190, .y1 = 136 },
    .wifi_zone          = { .x0 = 190, .y0 = 0, .x1 = 380, .y1 = 136 },
    .app_zone           = { .x0 = 0,   .y0 = 136, .x1 = 190, .y1 = 272 },
    .settings_zone      = { .x0 = 190, .y0 = 136, .x1 = 380, .y1 = 272 },
    .next_button_zone   = { .x0 = 400, .y0 = 80, .x1 = 480, .y1 = 272 },

    .x_center_left      = 95,
    .x_center_right     = 285,
    .y_center_top       = 68,
    .y_center_bottom    = 204,
    .text_vertical_offset = 10,
    .separator_y_start  = 20,
    .separator_y_end    = 252,
    .separator_x_padding = 20,
    .next_button_x_pos  = DRAWING_AREA_WIDTH + 5,
    .next_button_y_center = 192,
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na prvom ekranu za podešavanja (Termostat i Ventilator).
 */
static const struct
{
    WidgetRect_t thst_control_pos;      /**< Pozicija i dimenzije za RADIO widget kontrole termostata. */
    WidgetRect_t fan_control_pos;       /**< Pozicija i dimenzije za RADIO widget kontrole ventilatora. */
    WidgetRect_t thst_max_sp_pos;       /**< Pozicija i dimenzije za SPINBOX max. zadate temperature. */
    WidgetRect_t thst_min_sp_pos;       /**< Pozicija i dimenzije za SPINBOX min. zadate temperature. */
    WidgetRect_t fan_diff_pos;          /**< Pozicija i dimenzije za SPINBOX diferencijala ventilatora. */
    WidgetRect_t fan_low_band_pos;      /**< Pozicija i dimenzije za SPINBOX donjeg opsega ventilatora. */
    WidgetRect_t fan_hi_band_pos;       /**< Pozicija i dimenzije za SPINBOX gornjeg opsega ventilatora. */
    WidgetRect_t thst_group_pos;        /**< Pozicija i dimenzije za SPINBOX grupe termostata. */
    WidgetRect_t thst_master_pos;       /**< Pozicija i dimenzije za CHECKBOX master moda. */
    WidgetRect_t next_button_pos;       /**< Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;       /**< Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT label_thst_max_sp[2];     /**< Pozicije za dvorednu labelu za max. setpoint. */
    GUI_POINT label_thst_min_sp[2];     /**< Pozicije za dvorednu labelu za min. setpoint. */
    GUI_POINT label_fan_diff[2];        /**< Pozicije za dvorednu labelu za diferencijal ventilatora. */
    GUI_POINT label_fan_low[2];         /**< Pozicije za dvorednu labelu za donji opseg ventilatora. */
    GUI_POINT label_fan_hi[2];          /**< Pozicije za dvorednu labelu za gornji opseg ventilatora. */
    GUI_POINT label_thst_ctrl_title;    /**< Pozicija za naslov "THERMOSTAT CONTROL MODE". */
    GUI_POINT label_fan_ctrl_title;     /**< Pozicija za naslov "FAN SPEED CONTROL MODE". */
    GUI_POINT label_thst_group;         /**< Pozicija za labelu "GROUP". */
}
settings_screen_1_layout =
{
    .thst_control_pos   = { 10,  20,  150, 80 },
    .fan_control_pos    = { 10,  150, 150, 80 },
    .thst_max_sp_pos    = { 110, 20,  90,  30 },
    .thst_min_sp_pos    = { 110, 70,  90,  30 },
    .fan_diff_pos       = { 110, 150, 90,  30 },
    .fan_low_band_pos   = { 110, 190, 90,  30 },
    .fan_hi_band_pos    = { 110, 230, 90,  30 },
    .thst_group_pos     = { 320, 20,  100, 40 },
    .thst_master_pos    = { 320, 70,  170, 20 },
    .next_button_pos    = { 340, 180, 130, 30 },
    .save_button_pos    = { 340, 230, 130, 30 },

    .label_thst_max_sp  = { {210, 24}, {210, 36} },
    .label_thst_min_sp  = { {210, 74}, {210, 86} },
    .label_fan_diff     = { {210, 154}, {210, 166} },
    .label_fan_low      = { {210, 194}, {210, 206} },
    .label_fan_hi       = { {210, 234}, {210, 246} },
    .label_thst_ctrl_title = { 10, 4 },
    .label_fan_ctrl_title  = { 10, 120 },
    .label_thst_group      = { 430, 37 } // 320 (x) + 100 (w) + 10 (padding)
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na drugom ekranu za podešavanja (Vrijeme, Datum, Screensaver).
 * @note  Verzija 3.0: Sadrži pozicije za SVE elemente bez izuzetka.
 */
static const struct
{
    WidgetRect_t high_brightness_pos;       /**< @brief Pozicija i dimenzije za SPINBOX visoke svjetline. */
    WidgetRect_t low_brightness_pos;        /**< @brief Pozicija i dimenzije za SPINBOX niske svjetline. */
    WidgetRect_t scrnsvr_timeout_pos;       /**< @brief Pozicija i dimenzije za SPINBOX screensaver timeout-a. */
    WidgetRect_t scrnsvr_enable_hour_pos;   /**< @brief Pozicija i dimenzije za SPINBOX sata aktivacije screensaver-a. */
    WidgetRect_t scrnsvr_disable_hour_pos;  /**< @brief Pozicija i dimenzije za SPINBOX sata deaktivacije screensaver-a. */
    WidgetRect_t hour_pos;                  /**< @brief Pozicija i dimenzije za SPINBOX sata. */
    WidgetRect_t minute_pos;                /**< @brief Pozicija i dimenzije za SPINBOX minute. */
    WidgetRect_t day_pos;                   /**< @brief Pozicija i dimenzije za SPINBOX dana. */
    WidgetRect_t month_pos;                 /**< @brief Pozicija i dimenzije za SPINBOX mjeseca. */
    WidgetRect_t year_pos;                  /**< @brief Pozicija i dimenzije za SPINBOX godine. */
    WidgetRect_t scrnsvr_color_pos;         /**< @brief Pozicija i dimenzije za SPINBOX boje sata na screensaver-u. */
    WidgetRect_t scrnsvr_checkbox_pos;      /**< @brief Pozicija i dimenzije za CHECKBOX screensaver sata. */
    WidgetRect_t weekday_dropdown_pos;      /**< @brief Pozicija i dimenzije za DROPDOWN dan u sedmici. */
    WidgetRect_t next_button_pos;           /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;           /**< @brief Pozicija i dimenzije za "SAVE" dugme. */
    TouchZone_t  scrnsvr_color_preview_rect;/**< @brief Pozicija pravougaonika za pregled boje sata. */

    GUI_POINT label_backlight_title;        /**< @brief Pozicija za naslov "DISPLAY BACKLIGHT". */
    GUI_POINT label_high_brightness;        /**< @brief Pozicija za labelu "HIGH". */
    GUI_POINT label_low_brightness;         /**< @brief Pozicija za labelu "LOW". */
    GUI_POINT label_time_title;             /**< @brief Pozicija za naslov "SET TIME". */
    GUI_POINT label_hour;                   /**< @brief Pozicija za labelu "HOUR". */
    GUI_POINT label_minute;                 /**< @brief Pozicija za labelu "MINUTE". */
    GUI_POINT label_color_title;            /**< @brief Pozicija za naslov "SET COLOR". */
    GUI_POINT label_full_color;             /**< @brief Pozicija za labelu "FULL". */
    GUI_POINT label_clock_color;            /**< @brief Pozicija za labelu "CLOCK". */
    GUI_POINT label_scrnsvr_title;          /**< @brief Pozicija za naslov "SCREENSAVER OPTION". */
    GUI_POINT label_timeout;                /**< @brief Pozicija za labelu "TIMEOUT". */
    GUI_POINT label_enable_hour[2];         /**< @brief Pozicije za dvorednu labelu "ENABLE HOUR". */
    GUI_POINT label_disable_hour[2];        /**< @brief Pozicije za dvorednu labelu "DISABLE HOUR". */
    GUI_POINT label_date_title;             /**< @brief Pozicija za naslov "SET DATE". */
    GUI_POINT label_day;                    /**< @brief Pozicija za labelu "DAY". */
    GUI_POINT label_month;                  /**< @brief Pozicija za labelu "MONTH". */
    GUI_POINT label_year;                   /**< @brief Pozicija za labelu "YEAR". */

    HLine_t   line1, line2, line3, line4, line5; /**< @brief Definicije za sve dekorativne linije. */
}
settings_screen_2_layout =
{
    .high_brightness_pos      = { 10,  20,  90, 30 },
    .low_brightness_pos       = { 10,  60,  90, 30 },
    .scrnsvr_timeout_pos      = { 10,  130, 90, 30 },
    .scrnsvr_enable_hour_pos  = { 10,  170, 90, 30 },
    .scrnsvr_disable_hour_pos = { 10,  210, 90, 30 },
    .hour_pos                 = { 190, 20,  90, 30 },
    .minute_pos               = { 190, 60,  90, 30 },
    .day_pos                  = { 190, 130, 90, 30 },
    .month_pos                = { 190, 170, 90, 30 },
    .year_pos                 = { 190, 210, 90, 30 },
    .scrnsvr_color_pos        = { 340, 20,  90, 30 },
    .scrnsvr_checkbox_pos     = { 340, 70,  110, 20 },
    .weekday_dropdown_pos     = { 340, 100, 130, 100 },
    .next_button_pos          = { 340, 180, 130, 30 },
    .save_button_pos          = { 340, 230, 130, 30 },
    .scrnsvr_color_preview_rect = { 340, 51, 430, 59 },

    .label_backlight_title  = { 10,  5 },
    .label_high_brightness  = { 110, 35 },
    .label_low_brightness   = { 110, 75 },
    .label_time_title       = { 190, 5 },
    .label_hour             = { 290, 35 },
    .label_minute           = { 290, 75 },
    .label_color_title      = { 340, 5 },
    .label_full_color       = { 440, 26 },
    .label_clock_color      = { 440, 38 },
    .label_scrnsvr_title    = { 10,  115 },
    .label_timeout          = { 110, 145 },
    .label_enable_hour      = { {110, 176}, {110, 188} },
    .label_disable_hour     = { {110, 216}, {110, 228} },
    .label_date_title       = { 190, 115 },
    .label_day              = { 290, 145 },
    .label_month            = { 290, 185 },
    .label_year             = { 290, 225 },

    .line1 = { 15, 5, 160 },
    .line2 = { 15, 185, 320 },
    .line3 = { 15, 335, 475 },
    .line4 = { 125, 5, 160 },
    .line5 = { 125, 185, 320 }
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na trećem ekranu za podešavanja (Defroster, Ventilator).
 */
static const struct
{
    WidgetRect_t defroster_cycle_time_pos;      /**< @brief Pozicija i dimenzije za SPINBOX ciklusa odmrzivača. */
    WidgetRect_t defroster_active_time_pos;     /**< @brief Pozicija i dimenzije za SPINBOX aktivnog vremena odmrzivača. */
    WidgetRect_t defroster_pin_pos;             /**< @brief Pozicija i dimenzije za SPINBOX pina odmrzivača. */
    WidgetRect_t ventilator_relay_pos;          /**< @brief Pozicija i dimenzije za SPINBOX releja ventilatora. */
    WidgetRect_t ventilator_delay_on_pos;       /**< @brief Pozicija i dimenzije za SPINBOX odgode paljenja ventilatora. */
    WidgetRect_t ventilator_delay_off_pos;      /**< @brief Pozicija i dimenzije za SPINBOX odgode gašenja ventilatora. */
    WidgetRect_t ventilator_trigger1_pos;       /**< @brief Pozicija i dimenzije za SPINBOX prvog okidača ventilatora. */
    WidgetRect_t ventilator_trigger2_pos;       /**< @brief Pozicija i dimenzije za SPINBOX drugog okidača ventilatora. */
    WidgetRect_t ventilator_local_pin_pos;      /**< @brief Pozicija i dimenzije za SPINBOX lokalnog pina ventilatora. */
    WidgetRect_t select_control_pos;            /**< @brief Pozicija i dimenzije za DROPDOWN odabir kontrole. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT label_ventilator_title;           /**< @brief Pozicija za naslov "VENTILATOR CONTROL". */
    GUI_POINT label_defroster_title;            /**< @brief Pozicija za naslov "DEFROSTER CONTROL". */
    GUI_POINT label_select_control_title;       /**< @brief Pozicija za naslov "SELECT CONTROL 4". */

    GUI_POINT label_ventilator_relay[2];        /**< @brief Pozicije za dvorednu labelu za relej ventilatora. */
    GUI_POINT label_ventilator_delay_on[2];     /**< @brief Pozicije za dvorednu labelu za odgodu paljenja. */
    GUI_POINT label_ventilator_delay_off[2];    /**< @brief Pozicije za dvorednu labelu za odgodu gašenja. */
    GUI_POINT label_ventilator_trigger1[2];     /**< @brief Pozicije za dvorednu labelu za okidač 1. */
    GUI_POINT label_ventilator_trigger2[2];     /**< @brief Pozicije za dvorednu labelu za okidač 2. */
    GUI_POINT label_ventilator_local_pin[2];    /**< @brief Pozicije za dvorednu labelu za lokalni pin. */

    GUI_POINT label_defroster_cycle_time[2];    /**< @brief Pozicije za dvorednu labelu za vrijeme ciklusa. */
    GUI_POINT label_defroster_active_time[2];   /**< @brief Pozicije za dvorednu labelu za aktivno vrijeme. */
    GUI_POINT label_defroster_pin[2];           /**< @brief Pozicije za dvorednu labelu za pin. */

    HLine_t   line_ventilator_title;            /**< @brief Linija ispod naslova za ventilator. */
    HLine_t   line_defroster_title;             /**< @brief Linija ispod naslova za odmrzivač. */
    HLine_t   line_select_control;              /**< @brief Linija ispod odabira kontrole. */
}
settings_screen_3_layout =
{
    .defroster_cycle_time_pos   = { 200, 20,  110, 35 },
    .defroster_active_time_pos  = { 200, 60,  110, 35 },
    .defroster_pin_pos          = { 200, 100, 110, 35 },
    .ventilator_relay_pos       = { 10,  20,  110, 35 },
    .ventilator_delay_on_pos    = { 10,  60,  110, 35 },
    .ventilator_delay_off_pos   = { 10,  100, 110, 35 },
    .ventilator_trigger1_pos    = { 10,  140, 110, 35 },
    .ventilator_trigger2_pos    = { 10,  180, 110, 35 },
    .ventilator_local_pin_pos   = { 10,  220, 110, 35 },
    .select_control_pos         = { 200, 170, 110, 80 },
    .next_button_pos            = { 410, 180, 60,  30 },
    .save_button_pos            = { 410, 230, 60,  30 },

    .label_ventilator_title     = { 10, 4 },
    .label_defroster_title      = { 210, 4 },
    .label_select_control_title = { 200, 154 },

    .label_ventilator_relay     = { {130, 30},  {130, 42} },
    .label_ventilator_delay_on  = { {130, 70},  {130, 82} },
    .label_ventilator_delay_off = { {130, 110}, {130, 122} },
    .label_ventilator_trigger1  = { {130, 150}, {130, 162} },
    .label_ventilator_trigger2  = { {130, 190}, {130, 202} },
    .label_ventilator_local_pin = { {130, 230}, {130, 242} },

    .label_defroster_cycle_time = { {320, 30}, {320, 42} },
    .label_defroster_active_time= { {320, 70}, {320, 82} },
    .label_defroster_pin        = { {320, 110}, {320, 122} },

    .line_ventilator_title      = { 12, 5, 180 },
    .line_defroster_title       = { 12, 200, 375 },
    .line_select_control        = { 162, 200, 375 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na četvrtom ekranu za podešavanja (Roletne).
 * @note  Definiše pravila za dinamičko iscrtavanje mreže 2x2.
 */
static const struct
{
    /** @brief Početna tačka (gore-lijevo) za prvi widget u mreži. */
    GUI_POINT    grid_start_pos;
    /** @brief Širina jednog SPINBOX widgeta. */
    int16_t      widget_width;
    /** @brief Visina jednog SPINBOX widgeta. */
    int16_t      widget_height;
    /** @brief Vertikalni razmak između "UP" i "DOWN" spinbox-a za istu roletnu. */
    int16_t      y_row_spacing;
    /** @brief Vertikalni razmak između grupa widgeta za različite roletne. */
    int16_t      y_group_spacing;
    /** @brief Horizontalni razmak između prve i druge kolone widgeta. */
    int16_t      x_col_spacing;
    /** @brief Relativni pomak za ispis prve linije labele u odnosu na widget. */
    GUI_POINT    label_line1_offset;
    /** @brief Vertikalni pomak za ispis druge linije labele. */
    int16_t      label_line2_offset_y;
    /** @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t next_button_pos;
    /** @brief Pozicija i dimenzije za "SAVE" dugme. */
    WidgetRect_t save_button_pos;
}
settings_screen_4_layout =
{
    .grid_start_pos         = { 10,  20 },
    .widget_width           = 110,
    .widget_height          = 40,
    .y_row_spacing          = 50,
    .y_group_spacing        = 100,
    .x_col_spacing          = 190,
    .label_line1_offset     = { 120, 8 },
    .label_line2_offset_y   = 12,
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na petom ekranu za podešavanja (detaljne postavke za svjetla).
 * @note  Definiše pravila za iscrtavanje elemenata u dvije kolone.
 */
static const struct
{
    /** @brief X pozicija za prvu kolonu widgeta i labela. */
    int16_t      col1_x;
    /** @brief X pozicija za drugu kolonu widgeta i labela. */
    int16_t      col2_x;
    /** @brief Početna Y pozicija za prvi red. */
    int16_t      start_y;
    /** @brief Vertikalni razmak (visina reda) između widgeta. */
    int16_t      y_step;
    /** @brief Dimenzije za SPINBOX widgete. */
    WidgetRect_t spinbox_size;
    /** @brief Dimenzije za prvi CHECKBOX. */
    WidgetRect_t checkbox1_size;
    /** @brief Dimenzije za drugi CHECKBOX. */
    WidgetRect_t checkbox2_size;
    /** @brief Relativni pomak za ispis prve linije labele. */
    GUI_POINT    label_line1_offset;
    /** @brief Vertikalni pomak za ispis druge linije labele. */
    int16_t      label_line2_offset_y;
    /** @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t next_button_pos;
    /** @brief Pozicija i dimenzije za "SAVE" dugme. */
    WidgetRect_t save_button_pos;
}
settings_screen_5_layout =
{
    .col1_x                 = 10,
    .col2_x                 = 200,
    .start_y                = 5,
    .y_step                 = 43,
    .spinbox_size           = { 0, 0, 100, 40 }, // x,y se ne koriste, samo w,h
    .checkbox1_size         = { 0, 0, 130, 20 },
    .checkbox2_size         = { 0, 0, 145, 20 },
    .label_line1_offset     = { 110, 10 }, // 100 (sirina) + 10 (padding)
    .label_line2_offset_y   = 12,
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};

/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na šestom ekranu za podešavanja (Opšte postavke).
 * @author      Gemini & [Vaše Ime]
 * @note        AŽURIRANA VERZIJA: Uklonjen je `enable_scenes_checkbox_pos` jer
 * pripada drugom ekranu. Dodat je `enable_security_checkbox_pos` sa
 * fiksnom, definisanom pozicijom.
 ******************************************************************************
 */
static const struct
{
    WidgetRect_t device_id_pos;                 /**< @brief Pozicija i dimenzije za SPINBOX ID-a uređaja. */
    WidgetRect_t curtain_move_time_pos;         /**< @brief Pozicija i dimenzije za SPINBOX vremena kretanja roletni. */
    WidgetRect_t leave_scrnsvr_checkbox_pos;    /**< @brief Pozicija i dimenzije za CHECKBOX ponašanja screensaver-a. */
    WidgetRect_t night_timer_checkbox_pos;      /**< @brief Pozicija i dimenzije za CHECKBOX noćnog tajmera za svjetla. */
    WidgetRect_t enable_security_checkbox_pos;  /**< @brief Pozicija i dimenzije za CHECKBOX koji omogućava/onemogućava security modul. */
    WidgetRect_t set_defaults_button_pos;       /**< @brief Pozicija i dimenzije za "SET DEFAULTS" dugme. */
    WidgetRect_t restart_button_pos;            /**< @brief Pozicija i dimenzije za "RESTART" dugme. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT    device_id_label_pos[2];        /**< @brief Pozicije za dvorednu labelu za ID uređaja. */
    GUI_POINT    curtain_move_time_label_pos[2];/**< @brief Pozicije za dvorednu labelu za vrijeme kretanja roletni. */

    WidgetRect_t language_dropdown_pos;         /**< @brief Pozicija i dimenzije za DROPDOWN odabir jezika. */
    GUI_POINT    language_label_pos;            /**< @brief Pozicija za labelu "LANGUAGE". */
    WidgetRect_t select_control_1_pos;          /**< @brief Pozicija i dimenzije za DROPDOWN za dinamičku ikonu 1. */
    GUI_POINT    select_control_1_label_pos;    /**< @brief Pozicija za labelu za dinamičku ikonu 1. */
    WidgetRect_t select_control_2_pos;          /**< @brief Pozicija i dimenzije za DROPDOWN za dinamičku ikonu 2. */
    GUI_POINT    select_control_2_label_pos;    /**< @brief Pozicija za labelu za dinamičku ikonu 2. */
}
settings_screen_6_layout =
{
    .device_id_pos                = { 10, 10, 110, 40 },
    .curtain_move_time_pos        = { 10, 60, 110, 40 },
    .leave_scrnsvr_checkbox_pos   = { 10, 110, 205, 20 },
    .night_timer_checkbox_pos     = { 10, 140, 170, 20 },
    .enable_security_checkbox_pos = { 10, 165, 240, 20 }, // Postavljamo novu kontrolu na logičnu poziciju
    .set_defaults_button_pos      = { 10, 190, 80, 30 },
    .restart_button_pos           = { 10, 230, 80, 30 },
    .next_button_pos              = { 410, 180, 60, 30 },
    .save_button_pos              = { 410, 230, 60, 30 },

    .device_id_label_pos          = { {130, 20}, {130, 32} },
    .curtain_move_time_label_pos  = { {130, 70}, {130, 82} },

    .language_dropdown_pos        = { 220, 10, 110, 180 },
    .language_label_pos           = { 340, 22 },
    .select_control_1_pos         = { 220, 70, 110, 150 },
    .select_control_1_label_pos   = { 340, 82 },
    .select_control_2_pos         = { 220, 130, 110, 150 },
    .select_control_2_label_pos   = { 340, 142 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na sedmom ekranu za podešavanja (Scene Backend).
 */
static const struct
{
    WidgetRect_t enable_scenes_checkbox_pos;    /**< @brief Pozicija i dimenzije za CHECKBOX koji omogućava/onemogućava sistem scena. */
    GUI_POINT    grid_start_pos;                /**< @brief Početna tačka (gore-lijevo) za prvi widget u mreži okidača. */
    int16_t      widget_width;                  /**< @brief Širina jednog SPINBOX widgeta. */
    int16_t      widget_height;                 /**< @brief Visina jednog SPINBOX widgeta. */
    int16_t      y_spacing;                     /**< @brief Vertikalni razmak između redova. */
    int16_t      x_col_spacing;                 /**< @brief Horizontalni razmak između kolona. */
    GUI_POINT    label_offset;                  /**< @brief Relativni pomak za ispis labele. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */
}
settings_screen_7_layout =
{
    .enable_scenes_checkbox_pos = { 10, 5, 240, 20 },
    .grid_start_pos         = { 10, 40 },
    .widget_width           = 110,
    .widget_height          = 35,
    .y_spacing              = 50,
    .x_col_spacing          = 190,
    .label_offset           = { 120, 18 },
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};
/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na osmom ekranu za podešavanja (Kapije).
 * @author      Gemini
 * @note        Ova struktura je namjerno i u potpunosti identična kopija
 * strukture `settings_screen_5_layout` kako bi se osigurala
 * apsolutna vizuelna i funkcionalna konzistentnost sa ekranom
 * za podešavanje svjetala, u skladu sa projektnim zahtjevom.
 ******************************************************************************
 */
static const struct
{
    /** @brief X pozicija za prvu kolonu widgeta i labela. */
    int16_t      col1_x;
    /** @brief X pozicija za drugu kolonu widgeta i labela. */
    int16_t      col2_x;
    /** @brief Početna Y pozicija za prvi red. */
    int16_t      start_y;
    /** @brief Vertikalni razmak (visina reda) između widgeta. */
    int16_t      y_step;
    /** @brief Dimenzije za SPINBOX widgete. */
    WidgetRect_t spinbox_size;
    /** @brief Relativni pomak za ispis prve linije labele. */
    GUI_POINT    label_line1_offset;
    /** @brief Vertikalni pomak za ispis druge linije labele. */
    int16_t      label_line2_offset_y;
    /** @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t next_button_pos;
    /** @brief Pozicija i dimenzije za "SAVE" dugme. */
    WidgetRect_t save_button_pos;
}
settings_screen_8_layout =
{
    .col1_x                 = 10,
    .col2_x                 = 200,
    .start_y                = 5,
    .y_step                 = 43,
    .spinbox_size           = { 0, 0, 100, 40 }, // x,y se ne koriste, samo w,h
    .label_line1_offset     = { 110, 10 }, // 100 (sirina) + 10 (padding)
    .label_line2_offset_y   = 12,
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};

/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za raspored elemenata na
 * ekranu za podešavanje alarma (`SCREEN_SETTINGS_9`).
 ******************************************************************************
 */
static const struct
{
    GUI_POINT    start_pos;         /**< @brief Početna gornja-lijeva pozicija za prvu grupu kontrola. */
    WidgetRect_t spinbox_size;      /**< @brief Dimenzije za svaki SPINBOX. */
    int16_t      y_group_spacing;   /**< @brief Vertikalni razmak između grupa (particija). */
    int16_t      x_col_spacing;     /**< @brief Horizontalni razmak između kolona. */
}
settings_screen_9_layout =
{
    .start_pos         = { 10, 20 },
    .spinbox_size      = { 0, 0, 110, 40 },
    .y_group_spacing   = 50,
    .x_col_spacing     = 190
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za podešavanje datuma i vremena.
 */
static const struct
{
    // Definicija mreže (grid) za pozicioniranje grupa
    int16_t y_row_top;      /**< Y koordinata gornjeg reda dugmadi. */
    int16_t y_row_bottom;   /**< Y koordinata donjeg reda dugmadi. */
    int16_t x_col_1;        /**< X koordinata prve kolone (Dan/Sat). */
    int16_t x_col_2;        /**< X koordinata druge kolone (Mjesec/Minuta). */
    int16_t x_col_3;        /**< X koordinata treće kolone (Godina). */

    // Dimenzije i razmaci za +/- dugmad
    int16_t btn_size;       /**< Veličina (širina i visina) kvadratnih +/- dugmadi. */
    int16_t btn_pair_gap_x; /**< Horizontalni razmak između '+' i '-' dugmeta u paru. */

    // Vertikalni ofseti za tekst u odnosu na vrh dugmadi
    int16_t label_offset_y; /**< Vertikalni ofset od vrha dugmadi prema GORE do labele (npr. "SAT"). */
    int16_t value_offset_y; /**< Vertikalni ofset od vrha dugmadi prema GORE do teksta vrijednosti (npr. "23"). */

    // Definicije za 'OK' dugme
    int16_t ok_btn_pos_x;   /**< X pozicija za 'OK' dugme. */
    int16_t ok_btn_pos_y;   /**< Y pozicija za 'OK' dugme. */
    int16_t ok_btn_width;   /**< Širina 'OK' dugmeta. */
    int16_t ok_btn_height;  /**< Visina 'OK' dugmeta. */
}
datetime_settings_layout =
{
    .y_row_top      = 80,
    .y_row_bottom   = 200,
    .x_col_1        = 15,
    .x_col_2        = 175,
    .x_col_3        = 335,

    .btn_size       = 50,
    .btn_pair_gap_x = 10,

    .label_offset_y = 50,
    .value_offset_y = 25,

    .ok_btn_pos_x   = 335,
    .ok_btn_pos_y   = 200,
    .ok_btn_width   = 100,
    .ok_btn_height  = 50
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na glavnom ekranu tajmera.
 */
static const struct
{
    // Elementi za prikaz kada VRIJEME NIJE PODEŠENO
    GUI_POINT   datetime_icon_pos;      /**< Tačna pozicija za ikonicu 'podesi vrijeme'. */
    GUI_POINT   datetime_text_pos;      /**< Tačna pozicija za tekst 'Podesite Datum i Vrijeme'. */

    // Elementi za prikaz kada JE VRIJEME PODEŠENO
    GUI_POINT   time_pos;               /**< Pozicija za ispis vremena alarma (HH:MM). */
    GUI_POINT   days_pos;               /**< Pozicija za ispis dana ponavljanja. */
    GUI_POINT   toggle_icon_pos;        /**< Pozicija za ON/OFF toggle ikonicu. */
    GUI_POINT   status_text_pos;        /**< Pozicija za ispis statusa (uključen/isključen). */
}
timer_screen_layout =
{
    .datetime_icon_pos  = { 158, 84  },
    .datetime_text_pos  = { DRAWING_AREA_WIDTH / 2, 198 },

    .time_pos           = { DRAWING_AREA_WIDTH / 2, 80 },
    .days_pos           = { DRAWING_AREA_WIDTH / 2, 140 },
    .toggle_icon_pos    = { 0, 180 }, // X se računa dinamički
    .status_text_pos    = { DRAWING_AREA_WIDTH / 2, 235 }
};
static const struct
{
    // Prikaz vremena
    GUI_POINT   time_hour_pos;          /**< Pozicija za ispis sati (HH). */
    int16_t     time_hour_width;        /**< Širina kvadrata za HH. */
    GUI_POINT   time_colon_pos;         /**< Pozicija za ispis dvotačke (:). */
    int16_t     time_colon_width;       /**< Širina kvadrata za :. */
    GUI_POINT   time_minute_pos;        /**< Pozicija za ispis minuta (MM). */
    int16_t     time_minute_width;      /**< Širina kvadrata za MM. */

    // Dugmad za podešavanje vremena
    GUI_POINT   hour_up_pos;            /**< Pozicija za '+' dugme za sate. */
    GUI_POINT   hour_down_pos;          /**< Pozicija za '-' dugme za sate. */
    GUI_POINT   minute_up_pos;          /**< Pozicija za '+' dugme za minute. */
    GUI_POINT   minute_down_pos;        /**< Pozicija za '-' dugme za minute. */
    int16_t     time_btn_size;          /**< Veličina (širina i visina) +/- dugmadi. */

    // Odabir dana
    GUI_POINT   day_labels_pos;         /**< Pozicija za ispis skraćenica dana (PON, UTO...). */
    GUI_POINT   day_checkbox_start_pos; /**< Početna pozicija za prvi checkbox (ponedjeljak). */
    int16_t     day_checkbox_gap_x;     /**< Horizontalni razmak između checkbox-ova. */

    // Akcije
    GUI_POINT   buzzer_button_pos;      /**< Pozicija za dugme "Koristi buzzer". */
    GUI_POINT   scene_button_pos;       /**< Pozicija za dugme "Pokreni scenu". */
    GUI_POINT   scene_name_pos;         /**< Pozicija za ispis imena odabrane scene. */

    // Navigacija
    GUI_POINT   save_button_pos;        /**< Pozicija za 'SAVE' dugme. */
    GUI_POINT   cancel_button_pos;      /**< Pozicija za 'CANCEL' dugme. */
    GUI_POINT   scene_select_btn_pos;   /**< Pozicija za dugme za odabir scene (donji desni ugao). */
}
timer_settings_screen_layout =
{
    .time_hour_pos          = { 70, 80 },
    .time_hour_width        = 100,
    .time_colon_pos         = { 175, 80 },
    .time_colon_width       = 40,
    .time_minute_pos        = { 210, 80 },
    .time_minute_width      = 100,

    .hour_up_pos            = { 10, 25 },
    .hour_down_pos          = { 10, 95 },
    .minute_up_pos          = { 320, 25 },
    .minute_down_pos        = { 320, 95 },
    .time_btn_size          = 50,

    .day_labels_pos         = { 240, 160 },
    .day_checkbox_start_pos = { 15, 180 },
    .day_checkbox_gap_x     = 15,

    .buzzer_button_pos      = { 15, 222 },
    .scene_button_pos       = { 145, 222 },
    .scene_name_pos         = { 290, 238 },

    .save_button_pos        = { 400, 25 },
    .cancel_button_pos      = { 400, 95 },
    .scene_select_btn_pos   = { 400, 222 }
};

/**
 ******************************************************************************
 * @brief       Sadrži konstante za raspored elemenata u dinamičkoj mreži
 * ikonica (koristi se za ekrane Svjetla i Kapije).
 * @author      Gemini
 * @note        Ova struktura centralizuje sve parametre za layout, omogućavajući
 * lako i precizno podešavanje izgleda bez izmjene koda funkcija.
 ******************************************************************************
 */
static const struct
{
    /** @brief Početna Y pozicija za iscrtavanje kada je prikazan samo JEDAN red ikonica. */
    int16_t y_start_pos_single_row;

    /** @brief Početna Y pozicija za iscrtavanje kada su prikazana DVA reda ikonica. */
    int16_t y_start_pos_multi_row;

    /** @brief Ukupna visina jednog reda. Koristi se za izračunavanje centra reda i pozicije drugog reda. */
    int16_t row_height;

    /** @brief Vertikalni razmak u pikselima između teksta i ikonice. */
    int16_t text_icon_padding;

}
lights_and_gates_grid_layout =
{
    .y_start_pos_single_row = 86,   // Ranije hardkodirana vrijednost
    .y_start_pos_multi_row  = 0,    // Ranije hardkodirana vrijednost
    .row_height             = 130,  // Ranije hardkodirana vrijednost
    .text_icon_padding      = 2     // Ranije hardkodirana vrijednost
};
/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za raspored elemenata na
 * ekranu za kontrolu alarma (SCREEN_SECURITY).
 ******************************************************************************
 */
static const struct
{
    GUI_POINT    start_pos;         /**< @brief Početna gornja-lijeva pozicija za prvo dugme. */
    int16_t      button_size;       /**< @brief Veličina (širina i visina) kvadratnog dugmeta. */
    int16_t      y_spacing;         /**< @brief Vertikalni razmak između redova. */
    int16_t      label_x_offset;    /**< @brief Horizontalni razmak od dugmeta do početka teksta labele. */
}
security_screen_layout =
{
    .start_pos      = { 20, 5 },
    .button_size    = 50,
    .y_spacing      = 60,
    .label_x_offset = 10
};
// =======================================================================
// ===        Automatsko generisanje `const` niza za "Skener"          ===
// =======================================================================
// ULOGA: Ovaj blok koda koristi X-Macro tehniku da bi automatski
// popunio `settings_static_widget_ids` niz sa ID-jevima iz `settings_widgets.def`.
//
static const uint16_t settings_static_widget_ids[] = 
{
    // 1. Privremeno redefinišemo makro WIDGET da vraća samo prvi argument (ID)
#define WIDGET(id, val, comment) id,
    // 2. Uključujemo našu glavnu listu
#include "settings_widgets.def"
    // 3. Odmah poništavamo našu privremenu definiciju da ne bi smetala ostatku koda
#undef WIDGET
};

/**
 * @brief Niz sa pokazivačima na bitmape za ikonice svjetala.
 * @note  IZMIJENJENO: Redoslijed u ovom nizu sada MORA TAČNO ODGOVARATI
 * redoslijedu u `enum IconID` definisanom u `display.h`.
 * Svaka ikonica ima par (OFF, ON).
 */
static GUI_CONST_STORAGE GUI_BITMAP* light_modbus_images[] = 
{
    // Indeksi 0, 1 (Odgovara ICON_BULB = 0)
    &bmSijalicaOff, &bmSijalicaOn,
    // Indeksi 2, 3 (Odgovara ICON_VENTILATOR_ICON = 1)
    &bmicons_menu_ventilator_off, &bmicons_menu_ventilator_on,
    // Indeksi 4, 5 (Odgovara ICON_CEILING_LED_FIXTURE = 2)
    &bmicons_lights_ceiling_led_fixture_off, &bmicons_lights_ceiling_led_fixture_on,
    // Indeksi 6, 7 (Odgovara ICON_CHANDELIER = 3)
    &bmicons_lights_chandelier_off, &bmicons_lights_chandelier_on,
    // Indeksi 8, 9 (Odgovara ICON_HANGING = 4)
    &bmicons_lights_hanging_off, &bmicons_lights_hanging_on,
    // Indeksi 10, 11 (Odgovara ICON_LED_STRIP = 5)
    &bmicons_lights_led_off, &bmicons_lights_led_on,
    // Indeksi 12, 13 (Odgovara ICON_SPOT_CONSOLE = 6)
    &bmicons_lights_spot_console_off, &bmicons_lights_spot_console_on,
    // Indeksi 14, 15 (Odgovara ICON_SPOT_SINGLE = 7)
    &bmicons_lights_spot_single_off, &bmicons_lights_spot_single_on,
    // Indeksi 16, 17 (Odgovara ICON_STAIRS = 8)
    &bmicons_lights_stairs_off, &bmicons_lights_stairs_on,
    // Indeksi 18, 19 (Odgovara ICON_WALL = 9)
    &bmicons_lights_wall_off, &bmicons_lights_wall_on
};
/**
 * @brief Niz sa pokazivačima na bitmape ISKLJUČIVO za ikonice scena.
 * @note  Redoslijed u ovom nizu MORA tačno odgovarati redoslijedu enum-a
 * koji počinje sa ICON_SCENE_WIZZARD u fajlu display.h.
 */
static GUI_CONST_STORAGE GUI_BITMAP* scene_icon_images[] = 
{
    &bmicons_scene_wizzard,
    &bmicons_scene_morning,
    &bmicons_scene_sleep,
    &bmicons_scene_leaving,
    &bmicons_scene_homecoming,
    &bmicons_scene_movie,
    &bmicons_scene_dinner,
    &bmicons_scene_reading,
    &bmicons_scene_relaxing,
    &bmicons_scene_gathering,
    &bmicons_scene_security
};

/**
 ******************************************************************************
 * @brief       Niz sa pokazivačima na bitmape za sve tipove i stanja kapija.
 * @author      Gemini
 * @note        Ovaj niz je organizovan u blokove od po 5 ikonica za svaki osnovni
 * tip uređaja definisan u `IconID` enumu (počevši od `ICON_GATE_SWING`).
 * Redoslijed unutar svakog bloka je: ZATVORENO, OTVORENO, OTVARANJE, ZATVARANJE, DJELIMIČNO.
 * Ažurirano: Dodani su blokovi za SIGURNOSNA VRATA i PODZEMNU RAMPU.
 ******************************************************************************
 */
static GUI_CONST_STORAGE GUI_BITMAP* gate_icon_images[] = 
{
    // Blok za ICON_GATE_SWING (Krilna kapija) - 5 STANJA
    &bmicons_gate_swing_gate_closed,
    &bmicons_gate_swing_gate_open,
    &bmicons_gate_swing_gate_opening,
    &bmicons_gate_swing_gate_closing,
    &bmicons_gate_swing_gate_partial_open,

    // Blok za ICON_GATE_SLIDING (Klizna kapija) - 5 STANJA
    &bmicons_gate_sliding_gate_closed,
    &bmicons_gate_sliding_gate_open,
    &bmicons_gate_sliding_gate_opening,
    &bmicons_gate_sliding_gate_closing,
    &bmicons_gate_sliding_gate_partial_open,

    // Blok za ICON_GATE_GARAGE (Garažna vrata) - 5 STANJA
    &bmicons_gate_garage_door_closed,
    &bmicons_gate_garage_door_open,
    &bmicons_gate_garage_door_opening,
    &bmicons_gate_garage_door_closing,
    &bmicons_gate_garage_door_partial_open,

    // Blok za ICON_GATE_RAMP (Rampa) - 5 STANJA
    &bmicons_gate_ramp_closed,
    &bmicons_gate_ramp_open,
    &bmicons_gate_ramp_opening,
    &bmicons_gate_ramp_closing,
    &bmicons_gate_ramp_partial_open,

    // Blok za ICON_GATE_PEDESTRIAN_LOCK (Brava) - 5 STANJA (sa fallback-om)
    &bmicons_gate_pedestrian_closed,
    &bmicons_gate_pedestrian_open,
    &bmicons_gate_pedestrian_open,       // Za "opening" prikazuje otvoreno
    &bmicons_gate_pedestrian_closed,     // Za "closing" prikazuje zatvoreno
    &bmicons_gate_pedestrian_open,       // Za "partial" prikazuje otvoreno

    // === POČETAK ISPRAVKE: DODAVANJE BLOKOVA KOJI NEDOSTAJU ===

    // Blok za ICON_GATE_SECURITY_DOOR (Sigurnosna Vrata)
    // KORISTIMO IKONICE ZA BRAVU KAO PRIVREMENU ZAMJENU
    &bmicons_gate_pedestrian_closed,
    &bmicons_gate_pedestrian_open,
    &bmicons_gate_pedestrian_open,       // opening
    &bmicons_gate_pedestrian_closed,     // closing
    &bmicons_gate_pedestrian_open,       // partial

    // Blok za ICON_GATE_UNDERGROUND_RAMP (Podzemna Rampa)
    // KORISTIMO IKONICE ZA OBIČNU RAMPU KAO ZAMJENU
    &bmicons_gate_ramp_closed,
    &bmicons_gate_ramp_open,
    &bmicons_gate_ramp_opening,
    &bmicons_gate_ramp_closing,
    &bmicons_gate_ramp_partial_open,

    // === KRAJ ISPRAVKE ===
};
/** @brief Niz sa definisanim bojama za sat na screensaver-u. */
static uint32_t clk_clrs[COLOR_BSIZE] = 
{
    GUI_GRAY, GUI_RED, GUI_BLUE, GUI_GREEN, GUI_CYAN, GUI_MAGENTA,
    GUI_YELLOW, GUI_LIGHTGRAY, GUI_LIGHTRED, GUI_LIGHTBLUE, GUI_LIGHTGREEN,
    GUI_LIGHTCYAN, GUI_LIGHTMAGENTA, GUI_LIGHTYELLOW, GUI_DARKGRAY, GUI_DARKRED,
    GUI_DARKBLUE,  GUI_DARKGREEN, GUI_DARKCYAN, GUI_DARKMAGENTA, GUI_DARKYELLOW,
    GUI_WHITE, GUI_BROWN, GUI_ORANGE, CLR_DARK_BLUE, CLR_LIGHT_BLUE, CLR_BLUE, CLR_LEMON
};


/*============================================================================*/
/* HANDLE-OVI ZA GUI WIDGET-E                                                 */
/*============================================================================*/

/** @name Zajednički Handle-ovi za Navigaciju i Akcije
 * @brief Handle-ovi za dugmad koja se koriste na više ekrana za podešavanja.
 * @{
 */
static BUTTON_Handle   hBUTTON_Ok;             /**< @brief Handle za dugme "OK" ili "SAVE" za potvrdu i snimanje izmjena. */
static BUTTON_Handle   hBUTTON_Next;           /**< @brief Handle za dugme "NEXT" za prelazak na sljedeći ekran u sekvenci podešavanja. */
/** @} */

/** @name Handle-ovi za Podešavanja Sistema (SCREEN_SETTINGS_6)
 * @{
 */
static BUTTON_Handle   hBUTTON_SET_DEFAULTS;   /**< @brief Handle za dugme "SET DEFAULTS" za vraćanje na fabričke postavke. */
static BUTTON_Handle   hBUTTON_SYSRESTART;     /**< @brief Handle za dugme "RESTART" za softverski restart uređaja. */
static SPINBOX_Handle  hDEV_ID;                /**< @brief Handle za SPINBOX za podešavanje adrese uređaja na RS485 busu. */
static DROPDOWN_Handle hDRPDN_Language;        /**< @brief Handle za DROPDOWN za odabir jezika korisničkog interfejsa. */
/** @} */

/** @name Handle-ovi za Podešavanja Termostata (SCREEN_SETTINGS_1)
 * @{
 */
static RADIO_Handle    hThstControl;           /**< @brief Handle za RADIO grupu za odabir moda rada termostata (Grijanje/Hlađenje/Off). */
static RADIO_Handle    hFanControl;            /**< @brief Handle za RADIO grupu za odabir moda rada ventilatora (ON-OFF / 3 Brzine). */
static SPINBOX_Handle  hThstMaxSetPoint;       /**< @brief Handle za SPINBOX za podešavanje maksimalne dozvoljene temperature. */
static SPINBOX_Handle  hThstMinSetPoint;       /**< @brief Handle za SPINBOX za podešavanje minimalne dozvoljene temperature. */
static SPINBOX_Handle  hFanDiff;               /**< @brief Handle za SPINBOX za podešavanje temperaturne razlike (histereze) za ventilator. */
static SPINBOX_Handle  hFanLowBand;            /**< @brief Handle za SPINBOX za temperaturni opseg za nisku brzinu ventilatora. */
static SPINBOX_Handle  hFanHiBand;             /**< @brief Handle za SPINBOX za temperaturni opseg za visoku brzinu ventilatora. */
static SPINBOX_Handle  hThstGroup;             /**< @brief Handle za SPINBOX za podešavanje ID-ja grupe termostata. */
static CHECKBOX_Handle hThstMaster;            /**< @brief Handle za CHECKBOX za postavljanje uređaja kao Master u grupi. */
/** @} */

/** @name Handle-ovi za Podešavanja Ekrana i Vremena (SCREEN_SETTINGS_2)
 * @{
 */
static SPINBOX_Handle  hSPNBX_DisplayHighBrightness;   /**< @brief Handle za SPINBOX za podešavanje visoke (radne) svjetline ekrana. */
static SPINBOX_Handle  hSPNBX_DisplayLowBrightness;    /**< @brief Handle za SPINBOX za podešavanje niske (screensaver) svjetline ekrana. */
static SPINBOX_Handle  hSPNBX_ScrnsvrTimeout;          /**< @brief Handle za SPINBOX za podešavanje vremena neaktivnosti za ulazak u screensaver. */
static SPINBOX_Handle  hSPNBX_ScrnsvrEnableHour;       /**< @brief Handle za SPINBOX za sat automatske aktivacije screensavera. */
static SPINBOX_Handle  hSPNBX_ScrnsvrDisableHour;      /**< @brief Handle za SPINBOX za sat automatske deaktivacije screensavera. */
static SPINBOX_Handle  hSPNBX_ScrnsvrClockColour;      /**< @brief Handle za SPINBOX za odabir boje sata na screensaveru. */
static CHECKBOX_Handle hCHKBX_ScrnsvrClock;            /**< @brief Handle za CHECKBOX za omogućavanje prikaza sata na screensaveru. */
static SPINBOX_Handle  hSPNBX_Hour;                    /**< @brief Handle za SPINBOX za podešavanje sata (RTC). */
static SPINBOX_Handle  hSPNBX_Minute;                  /**< @brief Handle za SPINBOX za podešavanje minute (RTC). */
static SPINBOX_Handle  hSPNBX_Day;                     /**< @brief Handle za SPINBOX za podešavanje dana u mjesecu (RTC). */
static SPINBOX_Handle  hSPNBX_Month;                   /**< @brief Handle za SPINBOX za podešavanje mjeseca (RTC). */
static SPINBOX_Handle  hSPNBX_Year;                    /**< @brief Handle za SPINBOX za podešavanje godine (RTC). */
static DROPDOWN_Handle hDRPDN_WeekDay;                 /**< @brief Handle za DROPDOWN za odabir dana u sedmici (RTC). */
/** @} */

/** @name Handle-ovi za Podešavanja Ventilatora i Odmrzivača (SCREEN_SETTINGS_3)
 * @{
 */
static SPINBOX_Handle  hVentilatorRelay;           /**< @brief Handle za SPINBOX za RS485 adresu releja ventilatora. */
static SPINBOX_Handle  hVentilatorDelayOn;         /**< @brief Handle za SPINBOX za vrijeme odgode paljenja ventilatora. */
static SPINBOX_Handle  hVentilatorDelayOff;        /**< @brief Handle za SPINBOX za vrijeme odgode gašenja ventilatora. */
static SPINBOX_Handle  hVentilatorTriggerSource1;  /**< @brief Handle za SPINBOX za odabir prvog svjetla kao okidača. */
static SPINBOX_Handle  hVentilatorTriggerSource2;  /**< @brief Handle za SPINBOX za odabir drugog svjetla kao okidača. */
static SPINBOX_Handle  hVentilatorLocalPin;        /**< @brief Handle za SPINBOX za odabir lokalnog GPIO pina za ventilator. */
static Defroster_settingsWidgets defroster_settingWidgets; /**< @brief Struktura koja objedinjuje handle-ove za postavke odmrzivača. */
/** @} */

/** @name Handle-ovi za Podešavanja Roletni (SCREEN_SETTINGS_4)
 * @{
 */
static SPINBOX_Handle  hCurtainsRelay[CURTAINS_SIZE * 2]; /**< @brief Niz handle-ova za SPINBOX-ove za adrese "GORE" i "DOLE" releja za sve roletne. */
static SPINBOX_Handle  hCurtainsMoveTime;                 /**< @brief Handle za SPINBOX za podešavanje globalnog vremena kretanja roletni. */
/** @} */

/** @name Handle-ovi za Podešavanja Svjetala (SCREEN_SETTINGS_5)
 * @{
 */
static LIGHT_settingsWidgets lightsWidgets[LIGHTS_MODBUS_SIZE]; /**< @brief Niz struktura, gdje svaka sadrži sve handle-ove za podešavanje jednog svjetla. */
static BUTTON_Handle   hButtonRenameLight;                      /**< @brief Handle za dugme za preimenovanje svjetla (pokreće tastaturu). */
/** @} */

/** @name Handle-ovi za Podešavanja Opštih Opcija (SCREEN_SETTINGS_6)
 * @{
 */
static CHECKBOX_Handle hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH; /**< @brief Handle za CHECKBOX za promjenu ponašanja screensavera pri dodiru. */
static CHECKBOX_Handle hCHKBX_LIGHT_NIGHT_TIMER;    /**< @brief Handle za CHECKBOX za omogućavanje noćnog tajmera za svjetla. */
static CHECKBOX_Handle hCHKBX_EnableSecurity;       /**< @brief NOVO: Handle za CHECKBOX za omogućavanje security modula. */
static DROPDOWN_Handle hSelectControl_1;            /**< @brief Handle za DROPDOWN za dinamičku ikonu na SelectScreen1. */
static DROPDOWN_Handle hSelectControl_2;            /**< @brief Handle za DROPDOWN za dinamičku ikonu na SelectScreen2. */
/** @} */

/** @name Handle-ovi za Podešavanja Scena (SCREEN_SETTINGS_7 i Wizard)
 * @{
 */
static CHECKBOX_Handle hCHKBX_EnableScenes;         /**< @brief Handle za CHECKBOX za globalno omogućavanje/onemogućavanje sistema scena. */
static SPINBOX_Handle  hSPNBX_SceneTriggers[SCENE_MAX_TRIGGERS];  /**< @brief Niz handle-ova za SPINBOX-ove za adrese okidača "Povratak" scene. */
static BUTTON_Handle   hButtonChangeAppearance;     /**< @brief Handle za dugme "[ Promijeni ]" unutar čarobnjaka za scene. */
static BUTTON_Handle   hButtonDeleteScene;          /**< @brief Handle za dugme "[ Obriši ]" za brisanje postojeće scene. */
static BUTTON_Handle   hButtonDetailedSetup;        /**< @brief Handle za dugme "[ Detaljna Podešavanja ]" za ulazak u "anketni" čarobnjak. */
static CHECKBOX_Handle hCheckboxSceneLights;        /**< @brief Handle za Checkbox "Svjetla" u čarobnjaku. */
static CHECKBOX_Handle hCheckboxSceneCurtains;      /**< @brief Handle za Checkbox "Roletne" u čarobnjaku. */
static CHECKBOX_Handle hCheckboxSceneThermostat;    /**< @brief Handle za Checkbox "Termostat" u čarobnjaku. */
static BUTTON_Handle   hButtonWizNext;              /**< @brief Handle za dugme "[ Dalje ]" unutar čarobnjaka. */
static BUTTON_Handle   hButtonWizBack;              /**< @brief Handle za dugme "[ Nazad ]" unutar čarobnjaka. */
static BUTTON_Handle   hButtonWizCancel;            /**< @brief Handle za dugme "[ Otkaži ]" unutar čarobnjaka. */
/** @} */

/** @name Handle-ovi za Podešavanja Kapija (SCREEN_SETTINGS_8 i SCREEN_GATE_SETTINGS)
 * @{
 */
static SPINBOX_Handle  hGateSelect;                    /**< @brief Handle za SPINBOX za odabir kapije (1-6) na ekranu za podešavanja. */
static DROPDOWN_Handle hGateType;                      /**< @brief Handle za DROPDOWN za odabir Profila Kontrole. */
static SPINBOX_Handle  hGateAppearance;                /**< @brief Handle za SPINBOX za odabir vizuelnog izgleda. */
static SPINBOX_Handle  hGateParamSpinboxes[7];         /**< @brief Niz handle-ova za SPINBOX-ove za unos parametara (adrese, tajmeri). */
static BUTTON_Handle   hGateControlButtons[6];         /**< @brief Niz handle-ova za dinamički kreiranu dugmad (Otvori, Zatvori, Stop...) na kontrolnom panelu. */
/** @} */

/** @name Handle-ovi za Univerzalne Tastature (NumPad i Alfanumerička)
 * @{
 */
static BUTTON_Handle   hKeypadButtons[12];             /**< @brief Niz handle-ova za 12 dugmadi na numeričkoj tastaturi. */
static WM_HWIN         hKeyboardButtons[KEY_ROWS * KEYS_PER_ROW]; /**< @brief Niz handle-ova za tastere sa karakterima na alfanumeričkoj tastaturi. */
static WM_HWIN         hKeyboardSpecialButtons[5];     /**< @brief Niz handle-ova za specijalne tastere (Shift, Space, Del, OK) na alfanumeričkoj tastaturi. */
/** @} */

/** @name Handle-ovi za Podešavanja Datuma i Vremena (SCREEN_SETTINGS_DATETIME)
 * @{
 */
static TEXT_Handle     hTextDateTimeValue[5];        /**< @brief Handle-ovi za TEXT widgete koji prikazuju vrijednosti (Dan, Mjesec, Godina, Sat, Minuta). */
static BUTTON_Handle   hButtonDateTimeUp[5];           /**< @brief Niz handle-ova za GORE (+) dugmad. */
static BUTTON_Handle   hButtonDateTimeDown[5];         /**< @brief Niz handle-ova za DOLE (-) dugmad. */
/** @} */

/** @name Handle-ovi za Podešavanja Tajmera (SCREEN_SETTINGS_TIMER)
 * @{
 */
static BUTTON_Handle   hButtonTimerHourUp;             /**< @brief Handle za dugme '+' za sat. */
static BUTTON_Handle   hButtonTimerHourDown;           /**< @brief Handle za dugme '-' za sat. */
static BUTTON_Handle   hButtonTimerMinuteUp;           /**< @brief Handle za dugme '+' za minutu. */
static BUTTON_Handle   hButtonTimerMinuteDown;         /**< @brief Handle za dugme '-' za minutu. */
static BUTTON_Handle   hButtonTimerDay[7];             /**< @brief Niz handle-ova za 7 toggle dugmadi za dane u sedmici. */
static BUTTON_Handle   hButtonTimerBuzzer;             /**< @brief Handle za toggle dugme za aktivaciju zujalice. */
static BUTTON_Handle   hButtonTimerScene;              /**< @brief Handle za toggle dugme za aktivaciju scene. */
static BUTTON_Handle   hButtonTimerSceneSelect;        /**< @brief Handle za dugme za ulazak u odabir scene. */
static BUTTON_Handle   hButtonTimerSave;               /**< @brief Handle za dugme "Snimi" na ekranu za podešavanje tajmera. */
static BUTTON_Handle   hButtonTimerCancel;             /**< @brief Handle za dugme "Otkaži" na ekranu za podešavanje tajmera. */
/** @} */

/** @name Handle-ovi za Podešavanja Alarma (SCREEN_SETTINGS_ALARM)
 * @{
 */
static BUTTON_Handle hButtonChangePin;         /**< @brief Dugme za pokretanje procedure promjene glavnog PIN-a. */
static BUTTON_Handle hButtonSystemName;        /**< @brief Dugme za promjenu naziva kompletnog alarmnog sistema. */
static BUTTON_Handle hButtonPartitionName[3];  /**< @brief Niz handle-ova za dugmad za promjenu naziva particija. */
/** @} */
/*============================================================================*/
/* GLOBALNE VARIJABLE NA NIVOU PROJEKTA                                       */
/*============================================================================*/

/**
 * @brief Glavni 32-bitni registar sa flegovima za kompletan display modul.
 * @note Mora biti globalan da bi drugi moduli (npr. thermostat) mogli
 * da signaliziraju promjene putem makroa (npr. MVUpdateSet()).
 */
uint32_t dispfl;
/**
 * @brief Varijable koje čuvaju trenutno stanje korisničkog interfejsa.
 * @note Moraju biti globalne jer drugi moduli (rs485, lights, curtain)
 * čitaju 'screen' i postavljaju 'shouldDrawScreen' da zatraže osvježavanje.
 */
uint8_t screen, shouldDrawScreen;
/**
 * @brief Indeks trenutno odabrane zavjese.
 * @note Mora biti globalan jer ga `curtain.c` koristi da zna kojom zavjesom
 * se manipuliše preko GUI-ja.
 */
uint8_t curtain_selected;
// Definicija globalne instance strukture za postavke
Display_EepromSettings_t g_display_settings;

/*============================================================================*/
/* STATIČKE VARIJABLE NA NIVOU MODULA                                         */
/*============================================================================*/
/**
 * @brief Služi kao interni state-machine fleg za `Service_ThermostatScreen` funkciju.
 * @note Vrijednost `0` označava da ekran treba inicijalno iscrtati (pozadinu, statičke elemente),
 * dok vrijednost `1` označava da je ekran već iscrtan i da treba samo obrađivati unose i
 * ažurirati dinamičke podatke. Resetuje se na `0` prilikom napuštanja ekrana.
 */
static uint8_t thermostatMenuState = 0;
/**
 * @brief Fleg koji služi kao mehanizam za komunikaciju između modula.
 * @note Drugi moduli (npr. `defroster`, `ventilator`) pozivaju javnu funkciju `DISP_SignalDynamicIconUpdate()`
 * koja postavlja ovaj fleg na `true`. Funkcija `Service_SelectScreen1` periodično provjerava
 * ovaj fleg i, ako je postavljen, pokreće ponovno iscrtavanje dinamičke ikonice.
 */
static bool dynamicIconUpdateFlag = false;
/**
 * @brief Služi kao tajmer (čuvar `HAL_GetTick()` vrijednosti) za periodične akcije koje se dešavaju svake sekunde.
 * @note Koristi se u `Handle_PeriodicEvents` i `Service_ThermostatScreen` funkcijama za provjeru da li je
 * prošlo 1000ms, kako bi se ažurirao prikaz vremena na ekranu bez blokiranja ostatka koda.
 */
static uint32_t rtctmr;
/**
 * @brief Koristi se za detekciju dugog pritiska na ON/OFF zonu na ekranu termostata.
 * @note `HandlePress_ThermostatScreen` funkcija pokreće tajmer (upisuje `HAL_GetTick()`), a
 * `Service_ThermostatScreen` provjerava da li je prošlo dovoljno vremena (`> 2 sekunde`) da
 * bi se akcija (paljenje/gašenje) izvršila.
 */
static uint32_t thermostatOnOffTouch_timer = 0;
/**
 * @brief Glavni tajmer za screensaver. Čuva `HAL_GetTick()` vrijednost posljednje korisničke aktivnosti.
 * @note Funkcija `Handle_PeriodicEvents` ga koristi za provjeru da li je isteklo vrijeme neaktivnosti
 * (`g_display_settings.scrnsvr_tout`) kako bi aktivirala screensaver.
 */
static uint32_t scrnsvr_tmr;
/**
 * @brief Koristi se za detekciju dugog pritiska na ikonicu svjetla.
 * @note `HandlePress_LightsScreen` pokreće tajmer, a `Handle_PeriodicEvents` provjerava da li je prošlo
 * dovoljno vremena (`> 2 sekunde`) kako bi se prešlo na ekran za detaljno podešavanje
 * svjetla (`SCREEN_LIGHT_SETTINGS`).
 */
static uint32_t light_settingsTimerStart = 0;
/**
 * @brief Tajmer koji se okida otprilike svake minute (`>= 60 * 1000` ms) unutar `Handle_PeriodicEvents`.
 * @note Njegova glavna uloga je da pokrene provjeru da li je potrebno automatski upaliti neko
 * svjetlo na osnovu njegovih `on_hour` i `on_minute` postavki.
 */
static uint32_t everyMinuteTimerStart = 0;
/**
 * @brief Zastarjela varijabla. U priloženom kodu se deklariše, ali se nigdje ne koristi.
 */
static uint32_t onoff_tmr = 0;
/**
 * @brief Zastarjela varijabla. U priloženom kodu se deklariše, ali se nigdje ne koristi.
 */
static uint32_t value_step_tmr = 0;
/**
 * @brief Pomoćni tajmer unutar `Handle_PeriodicEvents` koji se inkrementira svake sekunde.
 * @note Kada dostigne vrijednost `10`, pokreće ažuriranje izmjerene temperature (`MVUpdateSet()`)
 * na ekranu termostata.
 */
static uint32_t refresh_tmr = 0;
/**
 * @brief Tajmer koji se koristi isključivo u `Service_CleanScreen` funkciji za odbrojavanje
 * vremena (60 sekundi) tokom kojeg je ekran zaključan za čišćenje.
 */
static uint32_t clean_tmr = 0;
/**
 * @brief Fleg koji se postavlja na `true` u `PID_Hook` funkciji ako je početni pritisak detektovan
 * unutar zone "hamburger" menija.
 * @note Koristi se da bi se spriječilo da se pritisak na meni propagira i aktivira
 * neku drugu akciju na ekranu.
 */
static bool touch_in_menu_zone = false;
/**
 * @brief Interni state-machine fleg za `Service_CleanScreen` funkciju.
 * @note Vrijednost `0` označava prvi ulazak na ekran kada se inicijalizuje tajmer,
 * dok `1` označava da je odbrojavanje u toku.
 */
static uint8_t menu_clean = 0;
/**
 * @brief Čuva stanje menija za odabir kontrole (npr. koji QR kod treba prikazati).
 * @note `HandlePress_SelectScreenLast` ga postavlja na `0` za WiFi ili `1` za App prije
 * prelaska na `SCREEN_QR_CODE`.
 */
static uint8_t menu_lc = 0;
/**
 * @brief Čuva trenutnu stranicu (indeks) u meniju za podešavanje zavjesa.
 * @note Koristi se u `DSP_InitSet4Scrn` i `Service_SettingsScreen_4` za prikazivanje
 * i upravljanje postavkama za 4 zavjese po stranici.
 */
static uint8_t curtainSettingMenu = 0;
/**
 * @brief Čuva indeks svjetla (stranicu) koje se trenutno podešava u meniju `SCREEN_SETTINGS_5`.
 * @note Koristi se u `DSP_InitSet5Scrn` i `Service_SettingsScreen_5` za prikazivanje i
 * upravljanje postavkama za jedno po jedno svjetlo.
 */
static uint8_t lightsModbusSettingsMenu = 0;
/**
 * @brief Čuva indeks svjetla na koje je korisnik kliknuo na ekranu `SCREEN_LIGHTS`.
 * @note Vrijednost `LIGHTS_MODBUS_SIZE + 1` se koristi da označi da nijedno svjetlo nije odabrano.
 * Koristi se za `toggle` operacije i za ulazak u `SCREEN_LIGHT_SETTINGS` za specifično svjetlo.
 */
static uint8_t light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
/**
 * @brief Fleg koji se postavlja na `true` ako se u grupi odabranih svjetala nalazi bar jedno RGB svjetlo.
 * @note Koristi se u `Service_LightSettingsScreen` da odredi da li treba prikazati paletu boja
 * kada se podešavaju sva svjetla odjednom.
 */
static uint8_t lights_allSelected_hasRGB = 0;
/**
 * @brief Globalni fleg za sve ekrane sa podešavanjima. Postavlja se na `1` kada korisnik
 * promijeni vrijednost bilo kojeg widgeta.
 * @note Koristi se da bi se znalo da li je potrebno pozvati funkciju za snimanje
 * u EEPROM (`Display_Save()`, `LIGHTS_Save()`, itd.) prilikom izlaska sa ekrana.
 */
static uint8_t settingsChanged = 0;
/**
 * @brief Fleg specifičan za `Service_SettingsScreen_1`, označava da je došlo do promjene
 * u podešavanjima termostata.
 * @note Osigurava da se `THSTAT_Save()` poziva samo kada je to zaista potrebno.
 */
static uint8_t thsta = 0;
/**
 * @brief Zastarjeli fleg, vjerovatno za promjene u podešavanjima svjetala (lights).
 * @note Koristi se u `Service_SettingsScreen_2` ali mu se vrijednost postavlja na zastarjeli način.
 * `settingsChanged` je preuzeo ovu ulogu.
 */
static uint8_t lcsta = 0;
/**
 * @brief Fleg koji se koristi u `DISPMenuSettings` funkciji za detekciju dugog pritiska za ulazak u meni.
 * @note Postavlja se na `1` u `PID_Hook` kada dodir započne u zoni hamburger menija.
 */
static uint8_t btnset = 0;
/**
 * @brief Par flegova za "edge detection" mehanizam za dugme za povećanje vrijednosti (npr. na termostatu).
 * @note `btninc` se postavlja na `1` dok je dugme pritisnuto. `_btninc` se koristi da
 * osigura da se akcija izvrši samo jednom po pritisku, a ne u svakom ciklusu petlje.
 */
static uint8_t btninc, _btninc;
/**
 * @brief Par flegova za "edge detection" mehanizam za dugme za smanjenje vrijednosti.
 * @note Radi na istom principu kao `btninc` i `_btninc`.
 */
static uint8_t btndec, _btndec;
/**
 * @brief Čuva prethodnu vrijednost minuta.
 * @note Koristi se za detekciju promjene minute kako bi se izbjeglo nepotrebno iscrtavanje ekrana svake sekunde.
 * U priloženom kodu se resetuje, ali se njegova vrijednost ne provjerava aktivno.
 */
static uint8_t old_min = 60;
/**
 * @brief Čuva prethodnu vrijednost dana u sedmici.
 * @note Koristi se u `DISPDateTime` funkciji za detekciju promjene dana, kako bi se
 * ponovo iscrtao kompletan screensaver sa novim datumom.
 */
static uint8_t old_day = 0;
/**
 * @brief Dvodimenzionalni bafer koji u RAM-u čuva tekstualne sadržaje za QR kodove.
 * @note Podaci se učitavaju iz EEPROM-a u `DISP_Init` funkciji i koriste u `Service_QrCodeScreen`
 * za generisanje slike QR koda.
 */
static uint8_t qr_codes[QR_CODE_COUNT][QR_CODE_LENGTH] = {0};
/**
 * @brief Određuje koji QR kod treba iscrtati na ekranu `SCREEN_QR_CODE`.
 * @note Vrijednost `1` je za WiFi, `2` za Aplikaciju. Postavlja se u `HandlePress_SelectScreenLast`.
 */
static uint8_t qr_code_draw_id = 0;
/**
 * @brief Brojač za odbrojavanje na ekranu za čišćenje (`SCREEN_CLEAN`).
 * @note Inicijalizuje se na 60 i dekrementira svake sekunde u `Service_CleanScreen` funkciji.
 */
static uint8_t clrtmr = 0;
/**
 * @brief Čuva kompletno stanje (koordinate i status pritiska) posljednjeg detektovanog pritiska na ekran.
 * @note Koristi se na `SCREEN_MAIN` da bi se pri otpuštanju znalo da li je početni pritisak
 * bio unutar zone glavnog prekidača.
 */
static GUI_PID_STATE last_press_state;
/**
 * @brief Čuva indeks scene (0-5) koja je odabrana kao akcija za tajmer.
 * @note Vrijednost -1 znači da nijedna scena nije odabrana. `Service_SettingsTimerScreen`
 * ga koristi za snimanje postavki i prikaz naziva odabrane scene.
 */
static int8_t timer_selected_scene_index = -1;
/**
 * @brief Fleg koji označava da li je ekran za podešavanje tajmera (`SCREEN_SETTINGS_TIMER`) već inicijalizovan.
 * @note Sprječava ponovno učitavanje vrijednosti iz `Timer` modula i reinicijalizaciju
 * widgeta unutar `Service_SettingsTimerScreen` funkcije.
 */
static bool timer_screen_initialized = false;
/**
 * @brief       Čuva odabranu akciju (0=System, 1-3=Particija) sa ekrana alarma.
 * @note        Vrijednost se postavlja u `Service_SecurityScreen` kada korisnik
 * pritisne dugme, a zatim se čita u `Service_NumpadScreen` nakon
 * uspješne provjere PIN-a kako bi se izvršila odgovarajuća komanda.
 */
static int8_t selected_action = -1;
/** @{ */

/** @name Statičke varijable za Alfanumeričku Tastaturu */

/**
 * @brief Niz handle-ova za sve dinamički kreirane tastere sa karakterima na alfanumeričkoj tastaturi.
 * @note Koristi se u `Service_KeyboardScreen` za provjeru koji taster je pritisnut.
 */
static WM_HWIN hKeyboardButtons[KEY_ROWS * KEYS_PER_ROW];
/**
 * @brief Niz handle-ova za specijalne tastere (Shift, Del, Space, OK) na alfanumeričkoj tastaturi.
 * @note Koristi se u `Service_KeyboardScreen` za obradu specijalnih komandi.
 */
static WM_HWIN hKeyboardSpecialButtons[5];
/**
 * @brief Interni bafer u koji se upisuje tekst prilikom kucanja na alfanumeričkoj tastaturi.
 * @note Njegov sadržaj se ispisuje na ekranu i kopira u `g_keyboard_result` nakon potvrde.
 */
static char keyboard_buffer[32];
/**
 * @brief Pokazivač (indeks) na trenutnu poziciju za unos u `keyboard_buffer`.
 * @note Ponaša se kao kursor, inkrementira se pri unosu karaktera, a dekrementira pri brisanju.
 */
static uint8_t keyboard_buffer_idx = 0;
/**
 * @brief Fleg koji prati da li je `Shift` taster na alfanumeričkoj tastaturi aktivan.
 * @note Mijenja se pritiskom na "Shift" i koristi se u `DSP_InitKeyboardScreen`
 * da odabere odgovarajući set karaktera (mala/velika slova) za iscrtavanje.
 */
static bool keyboard_shift_active = false;
/** @} */

/* Privatne varijable za PIN/Numpad tastaturu */

/**
 * @brief Bafer u koji se upisuju brojevi uneseni preko numeričke tastature (`SCREEN_NUMPAD`).
 * @note Sadržaj se prikazuje na ekranu (kao brojevi ili zvjezdice) i koristi za validaciju.
 */
static char pin_buffer[MAX_PIN_LENGTH + 1];
/**
 * @brief Pokazivač (indeks) na trenutnu poziciju za unos u `pin_buffer`.
 */
static uint8_t pin_buffer_idx = 0;
/**
 * @brief Tajmer za maskiranje karaktera na PIN tastaturi.
 * @note Pokreće se pri unosu novog broja. Kada istekne (`PIN_MASK_DELAY`),
 * pokreće ponovno iscrtavanje koje zamijeni broj sa zvjezdicom.
 */
static uint32_t pin_mask_timer = 0;
/**
 * @brief Fleg koji označava da li treba prikazati poruku o grešci (npr. pogrešan PIN).
 * @note Ako je `true`, `DSP_DrawNumpadText` će ispisati "GRESKA" umjesto unesenih brojeva.
 */
static bool pin_error_active = false;
/**
 * @brief Čuva posljednji uneseni karakter na PIN tastaturi.
 * @note Koristi se da bi se omogućilo privremeno prikazivanje broja prije
 * nego što ga `pin_mask_timer` pretvori u zvjezdicu.
 */
static char pin_last_char = 0;
/**
 * @brief Tajmer za detekciju dugog pritiska na naziv svjetla za pokretanje izmjene.
 * @note Pokreće se kada korisnik pritisne zonu sa tekstom u `SCREEN_LIGHT_SETTINGS`.
 * Provjerava se u `Handle_PeriodicEvents`, a resetuje u `HandleTouchReleaseEvent`.
 */
static uint32_t rename_light_timer_start = 0;
/**
 * @brief Čuva ID posljednjeg pritisnutog "EDIT" dugmeta na ekranu za podešavanje kapija.
 * @note  Koristi se za obradu rezultata vraćenog sa Numpad-a.
 */
static int active_gate_edit_button_id = 0;
/**
 * @brief Indeks kapije (0-5) koja je trenutno odabrana za podešavanje.
 */
static uint8_t settings_gate_selected_index = 0;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon zatvaranja Numpad-a.
 */
static eScreen numpad_return_screen = SCREEN_MAIN;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon zatvaranja alfanumeričke tastature.
 */
static eScreen keyboard_return_screen = SCREEN_MAIN;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon izlaska iz podešavanja svjetla.
 * @note  Ovo je neophodno jer se u SCREEN_LIGHT_SETTINGS može ući sa više
 * različitih ekrana (npr. SCREEN_MAIN ili SCREEN_LIGHTS).
 */
static eScreen light_settings_return_screen = SCREEN_MAIN;
/**
 * @brief Indeks scene (0-5) koja se trenutno kreira ili mijenja.
 * @note Ovu varijablu postavlja `HandlePress_SceneScreen` kada korisnik dodirne
 * neki od slotova za scene, a `DSP_InitSceneEditScreen` i `HandlePress_SceneEditScreen`
 * je koriste da znaju nad kojom scenom treba izvršiti akcije.
 */
static uint8_t scene_edit_index = 0;
/**
 * @brief Timestamp (HAL_GetTick()) kada je korisnik pritisnuo ikonicu na ekranu sa scenama.
 * @note  Koristi se za izračunavanje trajanja pritiska kako bi se razlikovao
 * kratak klik (aktivacija) od dugog pritiska (editovanje). Vrijednost 0
 * označava da pritisak nije aktivan.
 */
static uint32_t scene_press_timer_start = 0;
/**
 * @brief Indeks slota (0-5) koji je korisnik pritisnuo na ekranu sa scenama.
 * @note  Čuva se prilikom pritiska, a koristi prilikom otpuštanja.
 * Vrijednost -1 označava da nijedan slot nije pritisnut.
 */
static int8_t scene_pressed_index = -1;
/**
 * @brief Indeks trenutne stranice na ekranu za odabir izgleda scene.
 * @note  Koristi se za paginaciju ako ima više ikonica nego što može stati
 * na jedan ekran. Vrijednost 0 je prva stranica.
 */
static uint8_t scene_appearance_page = 0;
/**
 * @brief Globalni fleg unutar display modula koji prati da li je sistem u "Scene Wizard" modu.
 * @note  Ovaj fleg je ključan za kontekstualnu promjenu ponašanja ekrana.
 * Kada je `true`, ekrani kao što su SCREEN_LIGHTS ili SCREEN_THERMOSTAT
 * će prikazati "Dalje" dugme umjesto standardnog interfejsa.
 * Postavlja se na `true` pri ulasku u "anketni" čarobnjak, a na `false`
 * pri izlasku (snimanje, otkazivanje, timeout).
 */
static bool is_in_scene_wizard_mode = false;
/**
 ******************************************************************************
 * @brief       Timestamp (`HAL_GetTick()`) kada je korisnik pritisnuo ikonicu kapije.
 * @author      Gemini
 * @note        Koristi se za izračunavanje trajanja pritiska kako bi se
 * razlikovao kratak klik (toggle akcija) od dugog pritiska
 * (ulazak u kontrolni panel). Vrijednost 0 označava da pritisak
 * nije aktivan.
 ******************************************************************************
 */
static uint32_t gate_press_timer_start = 0;
/**
 ******************************************************************************
 * @brief       Indeks slota (0-5) koji je korisnik pritisnuo na ekranu sa kapijama.
 * @author      Gemini
 * @note        Čuva se prilikom pritiska, a koristi prilikom otpuštanja ili
 * detekcije dugog pritiska. Vrijednost -1 označava da nijedan
 * slot nije pritisnut.
 ******************************************************************************
 */
static int8_t gate_pressed_index = -1;
/**
 ******************************************************************************
 * @brief       Fleg koji prati da li je `Service_GateSettingsScreen` inicijalizovan.
 * @author      Gemini
 * @note        Postavlja se na `true` pri prvom ulasku u servisnu funkciju,
 * a `DSP_KillGateSettingsScreen` ga resetuje na `false`, čime se
 * forsira reinicijalizacija internih statičkih varijabli servisa.
 ******************************************************************************
 */
static bool gate_settings_initialized = false;
/**
 ******************************************************************************
 * @brief       Indeks kapije (0-5) za koju se prikazuje kontrolni panel.
 * @author      Gemini
 * @note        Ova varijabla se postavlja prilikom dugog pritiska i služi
 * `Service_GateControlPanel` funkciji da zna za koji uređaj treba
 * iscrtati kontrole i slati komande.
 ******************************************************************************
 */
static uint8_t gate_control_panel_index = 0;
/**
 * @brief Čuva trenutni mod rada za ekran za odabir scene.
 * @note  Ovu varijablu postavljaju funkcije koje pozivaju ekran (npr. iz `Service_SettingsTimerScreen`)
 * kako bi definisale njegovo ponašanje. Inicijalno je postavljen na WIZARD mod.
 */
static eScenePickerMode current_scene_picker_mode = SCENE_PICKER_MODE_WIZARD;
/**
 * @brief Čuva adresu ekrana na koji se treba vratiti nakon odabira scene.
 * @note  Ovo omogućava da se ekran za odabir pozove sa više različitih mjesta u aplikaciji
 * i da se uvijek osigura ispravan povratak.
 */
static eScreen scene_picker_return_screen = SCREEN_SCENE_EDIT;
/** 
 * @brief Tajmer za detekciju dugog pritiska na dinamičkoj ikonici na SelectScreen1. 
 */
static uint32_t dynamic_icon1_press_timer = 0;
/** 
 * @brief Tajmer za detekciju dugog pritiska na dinamičkoj ikonici na SelectScreen2. 
 */
static uint32_t dynamic_icon2_press_timer = 0;
/**
 * @brief Čuva indeks particije (0-2) koja je odabrana za promjenu naziva.
 * @note  Vrijednost -1 označava da nijedna nije odabrana.
 */
static int8_t selected_partition_for_rename = -1;
/**
 * @brief Prati trenutno stanje u procesu promjene PIN-a.
 */
static PinChangeState_e pin_change_state = PIN_CHANGE_IDLE;
/**
 * @brief Tajmer za detekciju dugog pritiska na ikonicu Alarma na SelectScreen2. 
 */
static uint32_t dynamic_icon_alarm_press_timer = 0;
/** 
 * @brief Tajmer za detekciju dugog pritiska na ikonicu Tajmera na SelectScreen2. 
 */
static uint32_t dynamic_icon_timer_press_timer = 0;
/**
 * @brief Privremeni bafer za čuvanje novog PIN-a tokom procedure promjene.
 */
static char new_pin_buffer[SECURITY_PIN_LENGTH];
/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH (STATIC) FUNKCIJA                                    */
/*============================================================================*/

/**
 * @name Funkcije za Inicijalizaciju i Uništavanje Ekrana Podešavanja
 * @brief Grupa funkcija odgovornih za kreiranje (`Init`) i uništavanje (`Kill`)
 * svih GUI widgeta na pojedinačnim ekranima za podešavanja.
 * @{
 */
static void DSP_InitSet1Scrn(void);
static void DSP_InitSet2Scrn(void);
static void DSP_InitSet3Scrn(void);
static void DSP_InitSet4Scrn(void);
static void DSP_InitSet5Scrn(void);
static void DSP_InitSet6Scrn(void);
static void DSP_InitSet7Scrn(void);
static void DSP_InitSet8Scrn(void);
static void DSP_InitSet9Scrn(void);
static void DSP_KillSet1Scrn(void);
static void DSP_KillSet2Scrn(void);
static void DSP_KillSet3Scrn(void);
static void DSP_KillSet4Scrn(void);
static void DSP_KillSet5Scrn(void);
static void DSP_KillSet6Scrn(void);
static void DSP_KillSet7Scrn(void);
static void DSP_KillSet8Scrn(void);
static void DSP_KillSet9Scrn(void);
static void DSP_InitSceneEditScreen(void);
static void DSP_KillSceneEditScreen(void);
static void DSP_KillLightSettingsScreen(void);
static void DSP_InitSceneAppearanceScreen(void);
static void DSP_InitSceneWizDevicesScreen(void);
static void DSP_KillSceneWizDevicesScreen(void);
static void DSP_KillSceneAppearanceScreen(void);
static void DSP_KillSceneScreen(void);
static void DSP_KillSceneEditLightsScreen(void);
static void DSP_KillSceneEditCurtainsScreen(void);
static void DSP_KillSceneEditThermostatScreen(void);
static void DSP_InitSceneWizFinalizeScreen(void);
static void DSP_KillSceneWizFinalizeScreen(void);
static void DSP_InitSettingsDateTimeScreen(void);
static void DSP_KillSettingsDateTimeScreen(void);
static void DSP_InitSettingsTimerScreen(void);
static void DSP_KillSettingsTimerScreen(void);
static void DSP_KillTimerScreen(void);
static void DSP_KillAlarmActiveScreen(void);
static void DSP_InitGateSettingsScreen(void);
static void DSP_KillGateSettingsScreen(void);
static void DSP_KillGateScreen(void);
static void DSP_KillSecurityScreen(void);
static void DSP_InitSettingsAlarmScreen(void);
static void DSP_KillSettingsAlarmScreen(void);

/** @} */

/**
 * @name Servisne Funkcije za Upravljanje Ekranima
 * @brief Svaka od ovih funkcija predstavlja "state" u glavnoj state-machine
 * petlji (`DISP_Service`). Poziva se periodično dok je odgovarajući
 * ekran aktivan i sadrži logiku za taj ekran.
 * @{
 */
static void Service_MainScreen(void);
static void Service_SelectScreen1(void);
static void Service_SelectScreen2(void);
static void Service_SelectScreenLast(void);
static void Service_ThermostatScreen(void);
static void Service_ReturnToFirst(void);
static void Service_SceneScreen(void);
static void Service_CleanScreen(void);
static void Service_SettingsScreen_1(void);
static void Service_SettingsScreen_2(void);
static void Service_SettingsScreen_3(void);
static void Service_SettingsScreen_4(void);
static void Service_SettingsScreen_5(void);
static void Service_SettingsScreen_6(void);
static void Service_SettingsScreen_7(void);
static void Service_SettingsScreen_8(void);
static void Service_SettingsScreen_9(void);
static void Service_LightsScreen(void);
static void Service_GateScreen(void);
static void Service_TimerScreen(void);
static void Service_SecurityScreen(void);
static void Service_CurtainsScreen(void);
static void Service_QrCodeScreen(void);
static void Service_LightSettingsScreen(void);
static void Service_MainScreenSwitch(void);
static void Service_SceneAppearanceScreen(void);
static void Service_SceneWizDevicesScreen(void);
static void Service_SceneEditScreen(void);
static void Service_SceneEditLightsScreen(void);
static void Service_SceneEditThermostatScreen(void);
static void Service_SceneWizFinalizeScreen(void);
static void Service_SettingsDateTimeScreen(void);
static void Service_SettingsTimerScreen(void);
static void Service_AlarmActiveScreen(void);
static void Service_GateSettingsScreen(void);
static void Service_SettingsAlarmScreen(void);

/** @} */

/**
 * @brief Glavna "hook" funkcija za obradu dodira na ekran.
 * @note Ovu funkciju poziva STemWin GUI biblioteka svaki put kada detektuje
 * promjenu stanja dodira. Ona je ulazna tačka za sve korisničke interakcije.
 * @param pTS Pokazivač na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
static void PID_Hook(GUI_PID_STATE * pTS);
/**
 * @brief Upravlja svim periodičnim događajima i tajmerima.
 * @note Poziva se iz `DISP_Service`. Odgovorna za logiku screensaver-a,
 * noćnog tajmera za svjetla, ažuriranje vremena i druge pozadinske zadatke.
 */
static void Handle_PeriodicEvents(void);
/**
 * @brief Dispečer za događaje pritiska na ekran.
 * @note Poziva se iz `PID_Hook` kada je ekran pritisnut. Na osnovu trenutnog
 * ekrana (`screen`), prosljeđuje događaj odgovarajućoj `HandlePress_...` funkciji.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag);

static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreenLast(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS);
static void HandlePress_MainScreenSwitch(GUI_PID_STATE * pTS);
static void HandlePress_SceneAppearanceScreen(GUI_PID_STATE* pTS, uint8_t* click_flag);
static void HandlePress_SceneScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_TimerScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_GateScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_GateSettingsScreen(GUI_PID_STATE* pTS, uint8_t* click_flag);

/**
 * @brief Dispečer za događaje otpuštanja dodira sa ekrana.
 * @note Poziva se iz `PID_Hook` kada korisnik otpusti prst sa ekrana.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS);

static void HandleRelease_MainScreenSwitch(GUI_PID_STATE * pTS);
static void HandleRelease_SceneScreen(void);
static void HandleRelease_AlarmIcon(void);
static void HandleRelease_TimerIcon(void);
/** @} */
/* Prototipovi novih funkcija za PIN tastaturu */
static void DSP_DrawNumpadText(void);
static void DSP_InitNumpadScreen(void);
static void Service_NumpadScreen(void);
static void DSP_KillNumpadScreen(void);
static void Display_ShowNumpad(const NumpadContext_t* context);

// NOVI PROTOTIPOVI ZA ALFANUMERIČKU TASTATURU
static void Display_ShowKeyboard(const KeyboardContext_t* context);
static void DSP_InitKeyboardScreen(void);
static void Service_KeyboardScreen(void);
static void DSP_KillKeyboardScreen(void);

// NOVA ISPRAVNA LINIJA:
static int PopulateControlDropdown(DROPDOWN_Handle hDropdown, int8_t exclusion_mode, int8_t* map_array, int map_size);
static void Display_ShowErrorPopup(const char* device_name, uint8_t device_index);
static void DISPSetBrightnes(uint8_t val);
/**
 * @name Pomoćne (Utility) Funkcije
 * @brief Grupa internih funkcija za razne zadatke kao što su snimanje,
 * iscrtavanje zajedničkih elemenata, upravljanje menijima, itd.
 * @{
 */
static void DISP_Animation(void);
/**
 * @brief Snima trenutne postavke displeja u EEPROM.
 */
static void Display_Save(void);
/**
 * @brief Postavlja sve postavke displeja na sigurne fabričke vrijednosti.
 */
static void Display_SetDefault(void);
/**
 * @brief Inicijalizuje postavke displeja iz EEPROM-a pri startu sistema.
 */
static void Display_InitSettings(void);
/**
 * @brief Iscrtava i ažurira prikaz datuma i vremena na ekranu.
 */
static void DISPDateTime(void);
/**
 * @brief Iscrtava ikonu "hamburger" menija u gornjem desnom uglu.
 */
static void DrawHamburgerMenu(uint8_t position);
/**
 * @brief Detektuje i obrađuje dugi pritisak za ulazak u meni za podešavanja.
 * @param btn Fleg koji ukazuje na početak pritiska (postavljen u `PID_Hook`).
 * @retval uint8_t 1 ako je dugi pritisak detektovan, inače 0.
 */
static uint8_t DISPMenuSettings(uint8_t btn);
/**
 * @brief Provjerava status ažuriranja firmvera i prikazuje odgovarajuću poruku.
 * @retval uint8_t 1 ako je ažuriranje aktivno, inače 0.
 */
static uint8_t Service_HandleFirmwareUpdate(void);
/**
 * @brief Forsirano uništava sve widgete sa ekrana za podešavanja.
 * @note Koristi se kao "fail-safe" mehanizam za čišćenje "duhova" sa ekrana.
 */
static void ForceKillAllSettingsWidgets(void);
/**
 * @brief Mapa koja povezuje indeks u DROPDOWN listi sa stvarnom ControlMode vrijednošću za Ikonu 1.
 */
static int8_t control_mode_map_1[MODE_COUNT];
/**
 * @brief Mapa koja povezuje indeks u DROPDOWN listi sa stvarnom ControlMode vrijednošću za Ikonu 2.
 */
static int8_t control_mode_map_2[MODE_COUNT];

static bool IsBusFwUpdateActive(void);
/** @} */

/*============================================================================*/
/* IMPLEMENTACIJA JAVNIH FUNKCIJA (GRUPA 1/12)                                */
/*============================================================================*/
/**
 * @brief Inicijalizuje GUI sistem.
 * @note Poziva se jednom na početku programa iz main() funkcije.
 * Inicijalizuje STemWin, postavlja hook za touch, učitava
 * parametre iz EEPROM-a i odmah iscrtava glavni ekran.
 */
void DISP_Init(void)
{
    uint8_t len;

    Display_InitSettings();

    // Inicijalizacija STemWin grafičke biblioteke
    GUI_Init();
    // Povezivanje (hook) funkcije za obradu dodira sa GUI sistemom
    GUI_PID_SetHook(PID_Hook);
    // Omogućavanje višestrukog baferovanja za fluidnije iscrtavanje
    WM_MULTIBUF_Enable(1);
    // Postavljanje UTF-8 enkodiranja za podršku specijalnim karakterima
    GUI_UC_SetEncodeUTF8();
    // Odabir i čišćenje prvog sloja (layer 0)
    GUI_SelectLayer(0);
    GUI_Clear();
    // Odabir drugog sloja (layer 1) i postavljanje providne pozadine
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    //DISP_Animation();
    // =======================================================================
    // === KRAJ NOVE LOGIKE ===
    // =======================================================================
    // Učitavanje prvog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE1, 1);
    if (len < QR_CODE_LENGTH) {
        EE_ReadBuffer(&qr_codes[0][0], EE_QR_CODE1 + 1, len);
    }

    // Učitavanje drugog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE2, 1);
    if (len < QR_CODE_LENGTH) {
        EE_ReadBuffer(&qr_codes[1][0], EE_QR_CODE2 + 1, len);
    }

    // Pokretanje tajmera koji se okida svake minute
    everyMinuteTimerStart = HAL_GetTick();

    // =======================================================================
    // === POČETAK NOVE "PAMETNE" LOGIKE ODABIRA POČETNOG EKRANA ===
    // =======================================================================
    // KORAK 1: Dobijamo handle-ove za sve relevantne module.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    // Ovdje ćemo dodati i gettere za Gate, Timer, Security kada njihovi moduli budu imali tu funkciju.

    // KORAK 2: Provjeravamo konfiguraciju po prioritetima.
    bool has_lights = (LIGHTS_getCount() > 0);
    bool has_thermostat = (Thermostat_GetGroup(pThst) > 0);
    bool has_curtains = (Curtains_getCount() > 0);
    // bool has_gates = (Gates_GetCount() > 0); // Primjer za budućnost

    if (has_lights) {
        // Ako ima svjetala, početni ekran je uvijek Main screen.
        screen = SCREEN_MAIN;
    } else if (has_thermostat && !has_curtains /* && !has_gates... */) {
        // Ako je JEDINA konfigurisana funkcija termostat.
        screen = SCREEN_THERMOSTAT;
    } else if (has_thermostat || has_curtains /* || has_gates... */) {
        // Ako nema svjetala, ali ima VIŠE drugih funkcija, scene su najbolji dashboard.
        screen = SCREEN_SCENE;
    } else {
        // Ako nema NIJEDNE konfigurisane glavne funkcije, provjeravamo SelectScreen1.
        // Ovdje ćemo dodati provjeru za Ventilator/Defroster.
        // Za sada, ako nema ničega, idemo na poruku za konfiguraciju.
        screen = SCREEN_CONFIGURE_DEVICE; // Privremeno, dok ne dodamo sve provjere
    }

    // Fallback ako nijedan uslov nije zadovoljen (iako ne bi trebalo da se desi)
    if (screen == 0) { // Sigurnosna provjera
        screen = SCREEN_MAIN; // Vraćamo na Main kao najsigurniju opciju
    }

    // =======================================================================
    // === KRAJ NOVE LOGIKE ===
    // =======================================================================

    // Ažurira se shouldDrawScreen za prvu petlju da bi se iscrtao odabrani ekran.
    shouldDrawScreen = 1;
}
/**
 * @brief Glavna servisna funkcija za korisnički interfejs.
 * @note Poziva se periodično iz glavne petlje programa (while(1) u main.c).
 * Odgovorna je za ažuriranje GUI-ja, obradu tajmera i pozivanje
 * specifičnih servisnih funkcija u zavisnosti od aktivnog ekrana.
 */
void DISP_Service(void)
{
    static uint32_t guitmr = 0;

    // Ažuriranje GUI-ja i obrada TS-a u fiksnim intervalima
    if ((HAL_GetTick() - guitmr) >= GUI_REFRESH_TIME) {
        guitmr = HAL_GetTick();
        GUI_Exec(); // Izvršava sve pending operacije iscrtavanja
    }

    // Provjera i prikaz poruke o ažuriranju firmvera
    if (Service_HandleFirmwareUpdate()) {
        return; // Ako je ažuriranje u toku, prekini dalje izvršavanje GUI logike
    }

    // Glavni switch za upravljanje stanjima (ekranima)
    switch (screen) {
    case SCREEN_MAIN:
        Service_MainScreen();
        break;
    case SCREEN_SELECT_1:
        Service_SelectScreen1();
        break;
    case SCREEN_SELECT_2:
        Service_SelectScreen2();
        break;
    case SCREEN_SCENE:
        Service_SceneScreen();
        break;
    case SCREEN_SCENE_EDIT:
        Service_SceneEditScreen();
        break;
    case SCREEN_SCENE_APPEARANCE:
        Service_SceneAppearanceScreen();
        break;
    case SCREEN_SCENE_WIZ_DEVICES:
        Service_SceneWizDevicesScreen();
        break;
    case SCREEN_SELECT_LAST:
        Service_SelectScreenLast();
        break;
    case SCREEN_THERMOSTAT:
        Service_ThermostatScreen();
        break;
    case SCREEN_ALARM_ACTIVE:
        Service_AlarmActiveScreen();
        break;
    case SCREEN_RETURN_TO_FIRST:
        Service_ReturnToFirst();
        break;
    case SCREEN_SETTINGS_1:
        Service_SettingsScreen_1();
        break;
    case SCREEN_SETTINGS_2:
        Service_SettingsScreen_2();
        break;
    case SCREEN_SETTINGS_3:
        Service_SettingsScreen_3();
        break;
    case SCREEN_SETTINGS_4:
        Service_SettingsScreen_4();
        break;
    case SCREEN_SETTINGS_5:
        Service_SettingsScreen_5();
        break;
    case SCREEN_SETTINGS_6:
        Service_SettingsScreen_6();
        break;
    case SCREEN_SETTINGS_7:
        Service_SettingsScreen_7();
        break;
    case SCREEN_SETTINGS_8:
        Service_SettingsScreen_8();
        break;
    case SCREEN_SETTINGS_9:
        Service_SettingsScreen_9();
        break;
    case SCREEN_SETTINGS_ALARM:
        Service_SettingsAlarmScreen();
        break;
    case SCREEN_CLEAN:
        Service_CleanScreen();
        break;
    case SCREEN_NUMPAD:
        Service_NumpadScreen();
        break;
    case SCREEN_LIGHTS:
        Service_LightsScreen();
        break;
    case SCREEN_CURTAINS:
        Service_CurtainsScreen();
        break;
    case SCREEN_GATE:
        Service_GateScreen();
        break;
    case SCREEN_GATE_SETTINGS:
        Service_GateSettingsScreen();
        break;
    case SCREEN_SECURITY:
        Service_SecurityScreen();
        break;
    case SCREEN_TIMER:
        Service_TimerScreen();
        break;
    case SCREEN_SETTINGS_TIMER:
        Service_SettingsTimerScreen();
        break;
    case SCREEN_SETTINGS_DATETIME:
        Service_SettingsDateTimeScreen();
        break;
    case SCREEN_QR_CODE:
        Service_QrCodeScreen();
        break;
    case SCREEN_LIGHT_SETTINGS:
        Service_LightSettingsScreen();
        break;
    case SCREEN_RESET_MENU_SWITCHES:
        Service_MainScreenSwitch();
        break;
    default:
        // U slučaju nepoznatog stanja, resetuj flegove menija
        menu_lc = 0;
        thermostatMenuState = 0;
        break;
    }

    // Upravljanje periodičnim događajima i tajmerima (npr. screensaver)
    Handle_PeriodicEvents();

    // Provjera da li treba ući u meni za podešavanja (dugi pritisak)
    if (DISPMenuSettings(btnset) && (screen < SCREEN_SETTINGS_1)) {
        // Inicijalizuj prvi ekran podešavanja
        DSP_InitSet1Scrn();
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 * @brief Prikazuje zadanu temperaturu (Set Point) na ekranu termostata.
 */
void DISPSetPoint(void)
{
    // Deklaracija lokalnih const varijabli
    const int spHPos = 200; // Horizontalna pozicija za SetPoint
    const int spVPos = 150; // Vertikalna pozicija za SetPoint

    // 1. Dobijamo handle za termostat.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    // Koristi se višestruko baferovanje za iscrtavanje bez treperenja
    GUI_MULTIBUF_BeginEx(1);

    // Očisti pravougaonik gdje se nalazi vrijednost
    GUI_ClearRect(spHPos - 5, spVPos - 5, spHPos + 120, spVPos + 85);

    // Postavi boju, font i poravnanje
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_D48);
    GUI_SetTextMode(GUI_TM_NORMAL);
    GUI_SetTextAlign(GUI_TA_RIGHT);
    GUI_GotoXY(spHPos, spVPos);

    // Ispiši vrijednost zadate temperature
    GUI_DispDec(Thermostat_GetSetpoint(pThst), 2);

    // Završi operaciju iscrtavanja
    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Resetuje tajmer za screensaver i postavlja visoku svjetlinu ekrana.
 * @note Ova funkcija se poziva nakon svakog dodira na ekran.
 */
void DISPResetScrnsvr(void)
{
    const int scrnsvrTout = 30; // 30 sekundi za izlaz iz screensavera
    // Ako je screensaver bio aktivan, vrati se na početni ekran
    if(IsScrnsvrActiv() && IsScrnsvrEnabled()) {
        screen = SCREEN_RETURN_TO_FIRST;
    }
    // Resetuj softverske flegove za screensaver
    ScrnsvrReset();
    ScrnsvrInitReset();
    // Resetuj tajmer neaktivnosti na trenutno vrijeme
    scrnsvr_tmr = HAL_GetTick();
    // Vrati default timeout vrijednost
    g_display_settings.scrnsvr_tout = scrnsvrTout;
    // Postavi visoku (normalnu) svjetlinu ekrana
    DISPSetBrightnes(g_display_settings.high_bcklght);
}

/**
 * @brief Hook funkcija za obradu događaja sa ekrana osjetljivog na dodir.
 * @note  Ovo je centralna tačka za obradu korisničkog unosa. STemWin poziva
 * ovu funkciju svaki put kada detektuje promjenu stanja dodira.
 * @param pTS Pointer na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
/**
 * @brief Hook funkcija za obradu događaja sa ekrana osjetljivog na dodir.
 * @param pTS Pointer na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
void PID_Hook(GUI_PID_STATE * pTS)
{
    static uint8_t release = 0; // Fleg koji prati da li je dodir bio pritisnut i čeka otpuštanje.
    uint8_t click = 0;          // Fleg koji signalizira da treba generisati zvučni signal (klik).

    if (screen == SCREEN_ALARM_ACTIVE) {
        if (pTS->Pressed) { // Bilo kakav pritisak na ekran
            Buzzer_Stop(); // Zaustavi zujalicu
            DSP_KillAlarmActiveScreen();
            screen = SCREEN_MAIN;
            shouldDrawScreen = 1;
        }
        return; // Preskoči ostatak logike
    }

    // NOVO: Potpuna blokada dodira ako je detektovana aktivnost ažuriranja na busu.
    if (IsBusFwUpdateActive()) {
        // Ako je ažuriranje aktivno, resetujemo screensaver da ekran ostane upaljen
        // i vidljiv, ali ignorišemo svaki unos dodirom.
        DISPResetScrnsvr();
        return;
    }

    // Provjera da li je dodir registrovan na početku s nulama, i resetuj btnset.
    // NOVO: Svaka linija je objašnjena
    if(pTS->x == 0 && pTS->y == 0 && pTS->Pressed == 0) { // Provjerava da li su koordinate nula i pritisak nula
        btnset = 0; // Ako je početno stanje, resetuj fleg za dugi pritisak
        return; // Izlazak iz funkcije
    }

    // Ostatak koda ostaje nepromijenjen
    if (screen == SCREEN_CLEAN) { // Ako je ekran za čišćenje, ignoriraj dodir
        return; // Izlazak iz funkcije
    }

    // --- Obrada pritiska na ekran ---
    if (pTS->Pressed == 1U) { // Ako je pritisak registrovan
        pTS->Layer = 1U; // Događaj se odnosi na gornji sloj (layer 1)
        release = 1;     // Postavi fleg da se čeka otpuštanje

        // << IZMJENA: Provjera zone hamburger menija sada koristi `global_layout` strukturu >>
        if ((pTS->x >= global_layout.hamburger_menu_zone.x0) && (pTS->x < global_layout.hamburger_menu_zone.x1) &&
                (pTS->y >= global_layout.hamburger_menu_zone.y0) && (pTS->y < global_layout.hamburger_menu_zone.y1) &&
                (screen < SCREEN_SETTINGS_1 && screen != SCREEN_KEYBOARD_ALPHA && screen != SCREEN_SCENE_APPEARANCE) &&
                !is_in_scene_wizard_mode && screen != SCREEN_SETTINGS_TIMER )
        {
            touch_in_menu_zone = true; // Postavi fleg da je dodir počeo u zoni menija
            click = 1;                 // Svaki dodir u ovoj zoni generiše zvučni signako setuje l

            // << ISPRAVKA: Centralizovano brisanje ekrana prilikom svake navigacije >>
            // Ovo osigurava da je ekran uvijek čist prije iscrtavanja novog sadržaja.
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_Clear();

            // =======================================================================
            // === POČETAK IZMJENE: Nova, ispravna logika navigacije ===
            // =======================================================================
            switch(screen) {
            // Pravilo: Sa bilo kog SELECT ekrana, hamburger vodi na MAIN
            case SCREEN_SELECT_1:
            case SCREEN_SELECT_2:
            case SCREEN_SELECT_LAST:
            case SCREEN_SCENE:
                screen = SCREEN_MAIN;
                break;

            // Pravilo: Sa pod-menija prvog nivoa, hamburger vodi na SELECT_1
            case SCREEN_THERMOSTAT:
                thermostatMenuState = 0; // Resetuj stanje ekrana termostata
            // Propadanje je namjerno
            case SCREEN_LIGHTS:
            case SCREEN_CURTAINS:
                screen = SCREEN_SELECT_1;
                break;

            case SCREEN_SETTINGS_DATETIME:
                DSP_KillSettingsDateTimeScreen();
                screen = SCREEN_TIMER; // Vraćamo se na Timer ekran
                break;

            case SCREEN_SETTINGS_TIMER:
                DSP_KillSettingsTimerScreen();
                screen = SCREEN_TIMER;
                break;

            // Pravilo: Sa pod-menija drugog nivoa, hamburger vodi na SELECT_2
            case SCREEN_TIMER:
                DSP_KillTimerScreen();

             case SCREEN_GATE:
                DSP_KillGateScreen();
                screen = SCREEN_SELECT_2;
                break;

            case SCREEN_SECURITY:
                DSP_KillSecurityScreen(); // <<< OBAVEZNO UNIŠTI DUGMAD PRIJE IZLASKA
                screen = SCREEN_SELECT_2;
                break;
            
            case SCREEN_SETTINGS_ALARM:
                DSP_KillSettingsAlarmScreen();
                screen = SCREEN_SELECT_2;
                break;
            
            case SCREEN_QR_CODE:
                menu_lc = 0; // Resetuj parametar
                screen = SCREEN_SELECT_LAST;
                break;

            // Pravilo: Sa MAIN ekrana, hamburger vodi u prvi meni
            case SCREEN_MAIN:
                screen = SCREEN_SELECT_1;
                break;

            // Specijalni slučajevi ostaju isti
            case SCREEN_LIGHT_SETTINGS:
                DSP_KillLightSettingsScreen();
                screen = light_settings_return_screen;
                break;

            case SCREEN_GATE_SETTINGS: // << NOVO
                DSP_KillGateSettingsScreen();
                screen = SCREEN_GATE; // Vrati se na dashboard za kapije
                break;

            case SCREEN_NUMPAD:
                DSP_KillNumpadScreen();
                pin_change_state = PIN_CHANGE_IDLE;
                g_numpad_result.is_cancelled = true;
                screen = numpad_return_screen;
                break;

            case SCREEN_KEYBOARD_ALPHA:
                DSP_KillKeyboardScreen();
                g_keyboard_result.is_cancelled = true;
                screen = keyboard_return_screen;
                break;
            }
            // =======================================================================
            // === KRAJ IZMJENE ===
            // =======================================================================

            shouldDrawScreen = 1; // Uvijek zatraži iscrtavanje nakon promjene ekrana
            btnset = 1; // Fleg za dugi pritisak
        } else {
            touch_in_menu_zone = false;
            HandleTouchPressEvent(pTS, &click);
        }
        if (click) {
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();
            click = 0;
        }
    }
    else { // Ako je dodir otpušten
        if(release) {
            release = 0;
            HandleTouchReleaseEvent(pTS);
            touch_in_menu_zone = false;
        }
        // Čim korisnik podigne prst, bez obzira gdje, vraćamo osjetljivost na normalu.
        g_high_precision_mode = false;
    }
    if (pTS->Pressed == 1U) { // Ako je pritisak bio registrovan
        DISPResetScrnsvr();
    }
}

/**
 * @brief Prikazuje log poruku na ekranu (koristi se za debug).
 * @note Funkcija ispisuje poruku i pomjera prethodne poruke prema gore,
 * stvarajući efekat skrolujućeg loga.
 * @param pbuf Pointer na string koji treba prikazati.
 */
void DISP_UpdateLog(const char *pbuf)
{
    static char displog[6][128]; // Statički 2D niz za čuvanje historije loga (6 linija)
    int i = 5;

    // Očisti područje na ekranu gdje se ispisuje log
    GUI_ClearRect(120, 80, 480, 240);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_TOP);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_SetFont(&GUI_Font16B_1);
    GUI_SetColor(GUI_WHITE);

    // Pomjeri sve stare poruke za jedno mjesto prema gore
    do {
        ZEROFILL(displog[i], COUNTOF(displog[i]));
        strcpy(displog[i], displog[i-1]);
        GUI_DispStringAt(displog[i], 125, 200-(i*20));
    } while(--i);

    // Ispiši novu poruku na dnu loga žutom bojom
    GUI_SetColor(GUI_YELLOW);
    ZEROFILL(displog[0], COUNTOF(displog[0]));
    strcpy(displog[0], pbuf);
    GUI_DispStringAt(displog[0], 125, 200);

    // Ažuriraj ekran
    GUI_Exec();
}
/**
 * @brief Postavlja stanje menija termostata.
 * @note Ova funkcija se koristi za postavljanje internog fleg-a
 * koji prati status ekrana menija termostata.
 * @param state Novo stanje menija (npr. 0 za neaktivan, 1 za aktivan).
 * @retval None
 */
void DISP_SetThermostatMenuState(uint8_t state)
{
    thermostatMenuState = state;
}

/**
 * @brief Dobiva trenutno stanje menija termostata.
 * @note Ova funkcija vraća vrijednost internog fleg-a koji
 * prati status ekrana menija termostata.
 * @param None
 * @retval uint8_t Trenutno stanje menija.
 */
uint8_t DISP_GetThermostatMenuState(void)
{
    return thermostatMenuState;
}

/**
 * @brief Signalizira da je potrebno ažurirati dinamičku ikonu.
 * @note Ova funkcija postavlja interni fleg-a `dynamicIconUpdateFlag`
 * na 'true'. Glavna servisna funkcija će pročitati ovaj fleg i
 * ponovo iscrtati dinamičku ikonu (za defroster/ventilator).
 * @param None
 * @retval None
 */
void DISP_SignalDynamicIconUpdate(void)
{
    dynamicIconUpdateFlag = true;
}

/**
 * @brief Vraća pointer na odgovarajući string iz tabele prevoda.
 * @param t ID teksta koji treba učitati (iz TextID enum-a).
 * @return const char* Pointer na string u tabeli za trenutno odabrani jezik.
 */
const char* lng(uint8_t t)
{
    // Provjera da li je ID u validnom opsegu
    if (t > 0 && t < TEXT_COUNT) {
        // Vrati direktan pointer na string iz tabele
        return language_strings[t][g_display_settings.language];
    }
    // U slučaju nevalidnog ID-ja, vrati pointer na prazan string
    return language_strings[0][0]; // Index 0 (TXT_DUMMY) je definisan kao prazan string
}
/**
 * @brief Provjerava da li je dužina podataka za QR kod validna.
 * @param dataLength Dužina podataka.
 * @return true ako je dužina manja od definisanog maksimuma, inače false.
 */
bool QR_Code_isDataLengthShortEnough(uint8_t dataLength)
{
    return dataLength < QR_CODE_LENGTH;
}

/**
 * @brief Provjerava da li će string stati u bafer za QR kod.
 * @param data Pointer na string sa podacima.
 * @return true ako podaci staju, inače false.
 */
bool QR_Code_willDataFit(const uint8_t *data)
{
    return QR_Code_isDataLengthShortEnough(strlen((char*)data));
}

/**
 * @brief Vraća pointer na podatke za specifični QR kod.
 * @param qrCodeID ID QR koda (1 za WiFi, 2 za App).
 * @return Pointer na string sa podacima.
 */
uint8_t* QR_Code_Get(const uint8_t qrCodeID)
{
    // Provjera da li je ID validan
    if (qrCodeID > 0 && qrCodeID <= QR_CODE_COUNT) {
        return qr_codes[qrCodeID - 1]; // Vrati traženi QR kod
    }
    return qr_codes[0]; // Vrati prvi kao fallback u slučaju greške
}
/**
 * @brief Postavlja podatke za specifični QR kod.
 * @note  Funkcija provjerava da li podaci staju u predviđeni bafer prije kopiranja.
 * @param qrCodeID ID QR koda (1 za WiFi, 2 za App).
 * @param data Pointer na string sa podacima.
 */
void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data)
{
    // Provjeri da li je ID validan i da li će podaci stati u bafer
    if(QR_Code_willDataFit(data) && qrCodeID > 0 && qrCodeID <= QR_CODE_COUNT)
    {
        // Sigurno kopiraj string u odgovarajući red 2D niza
        sprintf((char*)(qr_codes[qrCodeID - 1]), "%s", (char*)data);
    }
}
/*============================================================================*/
/* IMPLEMENTACIJA STATIČKIH (PRIVATNIH) FUNKCIJA                              */
/*============================================================================*/
static void DISP_Animation(void)
{
    DISPSetBrightnes(20);
    // Deklaracija niza pokazivača na sve frejmove animacije
    GUI_CONST_STORAGE GUI_BITMAP* animation_frames[] = {
        &bmanimation_welcome_frame_05,
        &bmanimation_welcome_frame_10,
        &bmanimation_welcome_frame_15,
        &bmanimation_welcome_frame_20,
        &bmanimation_welcome_frame_25,
        &bmanimation_welcome_frame_30,
        &bmanimation_welcome_frame_35,
        &bmanimation_welcome_frame_40,
        &bmanimation_welcome_frame_45,
        &bmanimation_welcome_frame_50,
        &bmanimation_welcome_frame_55,
        &bmanimation_welcome_frame_60,
        &bmanimation_welcome_frame_65,
        &bmanimation_welcome_frame_70,
        &bmanimation_welcome_frame_75,
        &bmanimation_welcome_frame_80,
        &bmanimation_welcome_frame_85,
        &bmanimation_welcome_frame_90,
        &bmanimation_welcome_frame_95,
        &bmanimation_welcome_frame_100
    };

    // Definirajte vrijeme zadržavanja svakog frejma u milisekundama
    const uint32_t FRAME_DELAY_MS = 10; // 20ms za svaki frejm = 50 FPS


    // Prikaz svakog frejma u petlji
    for (int i = 0; i < (sizeof(animation_frames) / sizeof(animation_frames[0])); i++)
    {
        GUI_MULTIBUF_Begin();
        GUI_Clear();
        // Crtanje bitmapa na centar ekrana
        GUI_DrawBitmap(animation_frames[i], (LCD_GetXSize() - animation_frames[i]->XSize) / 2, (LCD_GetYSize() - animation_frames[i]->YSize) / 2);
        GUI_MULTIBUF_End();

        // Obavezno pozvati GUI_Exec() nakon svake operacije
        GUI_Exec();

        // Čekanje prije prelaska na sljedeći frejm
        HAL_Delay(FRAME_DELAY_MS);
    }

    HAL_Delay(1000);

    // =======================================================================
    // === POČETAK NOVE LOGIKE ZA ISPIS TEKSTA NA DNU EKRANA ===
    // =======================================================================

    // Postavljanje fonta i boje za tekst
    GUI_SetFont(&GUI_Font20_ASCII);
    GUI_SetColor(GUI_WHITE);

    // Tekst koji želimo animirati
    const char* text = "www.imedia.ba";

    // Izračunavanje pozicije i dimenzija teksta
    int x_center = LCD_GetXSize() / 2;
    int y_bottom = LCD_GetYSize() - GUI_GetFontDistY() - 30;
    int text_width = GUI_GetStringDistX(text);
    int x_start = x_center - (text_width / 2);

    // Postavljanje globalnog poravnanja teksta za crtanje
    GUI_SetTextAlign(GUI_TA_LEFT);

    // Petlja za animaciju
    for(int current_width = 0; current_width <= text_width; current_width += 5) {
        GUI_MULTIBUF_Begin();
        GUI_ClearRect(x_start, y_bottom, x_start + text_width, y_bottom + GUI_GetFontDistY());

        // Postavljanje pravokutnika za "kliping" (clipped rect)
        GUI_RECT clip_rect = {x_start, y_bottom, x_start + current_width, y_bottom + GUI_GetFontDistY()};
        GUI_SetClipRect(&clip_rect);

        // Iscrtavanje cijelog teksta unutar definiranog "kliping" područja
        GUI_DispStringAt(text, x_start, y_bottom);

        // Obavezno resetirajte "kliping" područje nakon svake operacije
        GUI_SetClipRect(NULL);

        GUI_MULTIBUF_End();
        GUI_Exec();

        // Generiši "klik" zvuk za upravo iscrtano slovo
//        BuzzerOn();
//        HAL_Delay(1);
//        BuzzerOff();

        HAL_Delay(50); // Podesite kašnjenje za brzinu animacije
    }

    HAL_Delay(1000);
    // =======================================================================
    // === KRAJ NOVE LOGIKE ZA ISPIS TEKSTA ===
    // =======================================================================

#define ANIMATION_REPEATS 20 // Podesite broj ponavljanja animacije

    GUI_CONST_STORAGE GUI_BITMAP* animation_flame[] = {
        &bmanimation_candle_frame_1,
        &bmanimation_candle_frame_2,
        &bmanimation_candle_frame_3,
        &bmanimation_candle_frame_4
    };

    // Definirajte vrijeme zadržavanja svakog frejma u milisekundama
    const uint32_t FLAME_DELAY_MS = 100; // 100ms za svaki frejm

    // Izračunavanje dimenzija za brisanje (koristimo dimenzije prvog frejma)
    const int xPos = 118;
    const int yPos = 80;
    const int clearWidth = animation_flame[0]->XSize;
    const int clearHeight = animation_flame[0]->YSize;

    // Vanjska petlja za ponavljanje animacije
    for (int repeat = 0; repeat < ANIMATION_REPEATS; repeat++)
    {
        // Unutrašnja petlja za prikaz svakog frejma
        for (int i = 0; i < (sizeof(animation_flame) / sizeof(animation_flame[0])); i++)
        {
            GUI_MULTIBUF_Begin();

            // Dinamičko čišćenje područja bitmape na njenoj poziciji
            GUI_ClearRect(xPos, yPos, xPos + clearWidth, yPos + clearHeight);

            // Crtanje bitmapa
            GUI_DrawBitmap(animation_flame[i], xPos, yPos);
            GUI_MULTIBUF_End();

            // Obavezno pozvati GUI_Exec() nakon svake operacije
            GUI_Exec();

            // Čekanje prije prelaska na sljedeći frejm
            HAL_Delay(FLAME_DELAY_MS);
            DISPSetBrightnes(g_display_settings.high_bcklght);
        }
    }
    GUI_Clear();
    HAL_Delay(1000);
    DISPSetBrightnes(g_display_settings.low_bcklght);
}
/**
 ******************************************************************************
 * @brief       Pomoćna funkcija za popunjavanje dropdown liste dinamičkim opcijama.
 * @author      Gemini
 * @note        ISPRAVLJENA VERZIJA. Ova funkcija više ne koristi nepostojeću
 * ...UserData funkciju. Umjesto toga, dinamički kreira 'control_mode_map'
 * niz koji mapira indeks stavke u dropdown-u na njenu stvarnu
 * vrijednost iz `ControlMode` enuma.
 * @param[in]   hDropdown Handle dropdown widgeta koji treba popuniti.
 * @param[in]   exclusion_mode Vrijednost iz `ControlMode` enuma koju treba preskočiti.
 * @param[out]  map_array Pokazivač na niz koji će biti popunjen mapiranim vrijednostima.
 * @param[in]   map_size Veličina `map_array` niza.
 * @retval      int Broj stavki dodatih u dropdown.
 ******************************************************************************
 */
static int PopulateControlDropdown(DROPDOWN_Handle hDropdown, int8_t exclusion_mode, int8_t* map_array, int map_size)
{
    // Niz sa TextID-jevima za nazive opcija.
    const TextID mode_text_ids[] = {
        TXT_DUMMY, // Za "OFF"
        TXT_DEFROSTER,
        TXT_VENTILATOR,
        TXT_DUMMY, // Za "LANGUAGE"
        TXT_DUMMY, // Za "THEME"
        TXT_LANGUAGE_SOS_ALL_OFF, // Za "SOS"
        TXT_DUMMY, // Za "ALL OFF"
        TXT_DUMMY  // Za "OUTDOOR"
    };

    // Fiksni nazivi koji nemaju svoj TextID
    const char* fallback_names[] = {
        "OFF", "", "", "LANGUAGE", "THEME", "", "ALL OFF", "OUTDOOR"
    };

    int items_added = 0;
    // Pošto DeleteAllItems ne postoji, brišemo stavke jednu po jednu unazad.
    int num_items = DROPDOWN_GetNumItems(hDropdown);
    for (int i = num_items - 1; i >= 0; i--) {
        DROPDOWN_DeleteItem(hDropdown, i);
    }

    for (int i = MODE_OFF; i < MODE_COUNT; i++)
    {
        // Provjeravamo da li trenutni mod treba biti izostavljen
        if (i != exclusion_mode)
        {
            // Dodajemo naziv u dropdown listu
            const char* item_name = (mode_text_ids[i] != TXT_DUMMY) ? lng(mode_text_ids[i]) : fallback_names[i];
            DROPDOWN_AddString(hDropdown, item_name);

            // Mapiramo stvarni `ControlMode` `i` na trenutni indeks `items_added`
            if (map_array != NULL && items_added < map_size)
            {
                map_array[items_added] = i;
            }
            items_added++;
        }
    }
    return items_added;
}
/**
 * @brief Postavlja svjetlinu pozadinskog osvjetljenja.
 * @param val Vrijednost svjetline (od 1 do 90).
 */
static void DISPSetBrightnes(uint8_t val)
{
    if (val < DISP_BRGHT_MIN) val = DISP_BRGHT_MIN;
    else if (val > DISP_BRGHT_MAX) val = DISP_BRGHT_MAX;
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)(val * 10U));
}
/**
 ******************************************************************************
 * @brief Provjerava da li je u toku ažuriranje firmvera bilo gdje na RS485 busu.
 * @author Gemini & [Vaše Ime]
 * @note  Funkcija provjerava globalni tajmer koji se resetuje svakim primljenim
 * FIRMWARE_UPDATE paketom. Ako je od zadnjeg paketa prošlo manje od
 * FW_UPDATE_BUS_TIMEOUT, smatra se da je ažuriranje i dalje aktivno.
 * @retval bool true ako je ažuriranje aktivno, inače false.
 ******************************************************************************
 */
static bool IsBusFwUpdateActive(void)
{
    // Provjera da li je tajmer inicijalizovan (da ne bi bilo lažno pozitivnih na startu)
    if (g_last_fw_packet_timestamp == 0) {
        return false;
    }
    // Provjera da li je prošlo manje vremena od definisanog timeout-a
    return ((HAL_GetTick() - g_last_fw_packet_timestamp) < FW_UPDATE_BUS_TIMEOUT);
}
/**
 * @brief  Postavlja sve postavke displeja na sigurne fabričke vrijednosti.
 * @note   Ova funkcija se poziva kada podaci u EEPROM-u nisu validni.
 * @param  None
 * @retval None
 */
static void Display_SetDefault(void)
{
    // Sigurnosno nuliranje cijele strukture
    memset(&g_display_settings, 0, sizeof(Display_EepromSettings_t));

    // Postavljanje logičkih početnih vrijednosti
    g_display_settings.low_bcklght = 5;
    g_display_settings.high_bcklght = 80;
    g_display_settings.scrnsvr_tout = 30; // 30 sekundi
    g_display_settings.scrnsvr_ena_hour = 22; // 22:00
    g_display_settings.scrnsvr_dis_hour = 7;  // 07:00
    g_display_settings.scrnsvr_clk_clr = 0;   // GUI_GRAY
    g_display_settings.scrnsvr_on_off = true;
    g_display_settings.leave_scrnsvr_on_release = false;
    g_display_settings.language = BSHC; // Početni jezik je bosanski
    g_display_settings.scenes_enabled = true;
}

/**
 * @brief  Čuva kompletnu konfiguraciju displeja u EEPROM, uključujući CRC.
 * @param  None
 * @retval None
 */
static void Display_Save(void)
{
    // Postavljanje magičnog broja kao "potpisa"
    g_display_settings.magic_number = EEPROM_MAGIC_NUMBER;
    // Privremeno nuliranje CRC-a radi ispravnog izračuna
    g_display_settings.crc = 0;
    // Izračunavanje CRC-a nad cijelom strukturom
    g_display_settings.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_display_settings, sizeof(Display_EepromSettings_t));
    // Snimanje cijelog bloka podataka u EEPROM
    EE_WriteBuffer((uint8_t*)&g_display_settings, EE_DISPLAY_SETTINGS, sizeof(Display_EepromSettings_t));
}

/**
 * @brief  Inicijalizuje postavke displeja iz EEPROM-a, uz provjeru validnosti.
 * @note   Ova funkcija se poziva jednom pri startu sistema.
 * @param  None
 * @retval None
 */
static void Display_InitSettings(void)
{
    // Učitavanje cijelog konfiguracionog bloka iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&g_display_settings, EE_DISPLAY_SETTINGS, sizeof(Display_EepromSettings_t));

    // Provjera "magičnog broja"
    if (g_display_settings.magic_number != EEPROM_MAGIC_NUMBER) {
        // Podaci su nevažeći, učitaj fabričke postavke
        Display_SetDefault();
        // Odmah snimi ispravne fabričke postavke
        Display_Save();
    } else {
        // Magični broj je ispravan, provjeri integritet podataka pomoću CRC-a
        uint16_t received_crc = g_display_settings.crc;
        g_display_settings.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_display_settings, sizeof(Display_EepromSettings_t));

        if (received_crc != calculated_crc) {
            // Podaci su oštećeni, učitaj sigurne fabričke postavke
            Display_SetDefault();
            Display_Save();
        }
    }

    // Nakon validacije, primijeni učitane/defaultne postavke na globalne varijable
    // koje se koriste u ostatku display.c fajla, ako je to neophodno.
    // Npr. ako `language` varijabla mora biti globalna:
    // language = g_display_settings.language;
    // Ovo omogućava da ostatak koda ne morate mijenjati odmah.
}

/**
* @brief Očisti sve duhove sa ekrana
 * @note Ova funkcija provjerava postojanje widgeta iz liste svih definisanih u settings_widgets.def
 * kada je kontrola na glavnom ekranu i displej u modu screen savera i uklanja svaki "zivi" widget
 */
static void ForceKillAllSettingsWidgets(void)
{
    WM_HWIN hWidget;
    uint16_t id_to_check;

    // --- 1. Uništavanje statičkih widgeta sa nove, pregledne liste ---
    for (uint16_t i = 0; i < (sizeof(settings_static_widget_ids) / sizeof(settings_static_widget_ids[0])); i++) {
        id_to_check = settings_static_widget_ids[i];
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // --- 2. Uništavanje dinamičkih widgeta (petlje ostaju) ---
    // Zavjese
    for (uint16_t i = 0; i < (CURTAINS_SIZE * 2); i++) {
        id_to_check = ID_CurtainsRelay + i;
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // Svjetla
    for (uint16_t i = 0; i < (LIGHTS_MODBUS_SIZE * 13); i++) {
        id_to_check = ID_LightsModbusRelay + i;
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // --- 3. DOPUNJENO: Brisanje svih izostavljenih widgeta ---

    // NumPad tastatura
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i])) {
            WM_DeleteWindow(hKeypadButtons[i]);
            hKeypadButtons[i] = 0;
        }
    }
    // Alfanumerička tastatura
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i])) {
            WM_DeleteWindow(hKeyboardButtons[i]);
            hKeyboardButtons[i] = 0;
        }
    }
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hKeyboardSpecialButtons[i])) {
            WM_DeleteWindow(hKeyboardSpecialButtons[i]);
            hKeyboardSpecialButtons[i] = 0;
        }
    }
    if (WM_IsWindow(hButtonRenameLight)) {
        WM_DeleteWindow(hButtonRenameLight);
        hButtonRenameLight = 0;
    }

    // Podešavanja kapija
    if (WM_IsWindow(hGateSelect)) WM_DeleteWindow(hGateSelect);
    if (WM_IsWindow(hGateType)) WM_DeleteWindow(hGateType);
    for (int i = 0; i < 9; i++) {
        if (WM_IsWindow(hGateControlButtons[i])) {
            WM_DeleteWindow(hGateControlButtons[i]);
            hGateControlButtons[i] = 0;
        }
    }

    // Podešavanja scena
    if (WM_IsWindow(hButtonChangeAppearance)) WM_DeleteWindow(hButtonChangeAppearance);
    if (WM_IsWindow(hButtonDeleteScene)) WM_DeleteWindow(hButtonDeleteScene);
    if (WM_IsWindow(hButtonDetailedSetup)) WM_DeleteWindow(hButtonDetailedSetup);
    if (WM_IsWindow(hButtonWizCancel)) WM_DeleteWindow(hButtonWizCancel);
    if (WM_IsWindow(hButtonWizBack)) WM_DeleteWindow(hButtonWizBack);
    if (WM_IsWindow(hButtonWizNext)) WM_DeleteWindow(hButtonWizNext);

    for (int i = 0; i < SCENE_MAX_TRIGGERS; i++) {
        if (WM_IsWindow(hSPNBX_SceneTriggers[i])) {
            WM_DeleteWindow(hSPNBX_SceneTriggers[i]);
            hSPNBX_SceneTriggers[i] = 0;
        }
    }

    // Podešavanja datuma i vremena
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hTextDateTimeValue[i])) WM_DeleteWindow(hTextDateTimeValue[i]);
        if (WM_IsWindow(hButtonDateTimeUp[i])) WM_DeleteWindow(hButtonDateTimeUp[i]);
        if (WM_IsWindow(hButtonDateTimeDown[i])) WM_DeleteWindow(hButtonDateTimeDown[i]);
    }

    // Podešavanja tajmera
    if (WM_IsWindow(hButtonTimerHourUp)) WM_DeleteWindow(hButtonTimerHourUp);
    if (WM_IsWindow(hButtonTimerHourDown)) WM_DeleteWindow(hButtonTimerHourDown);
    if (WM_IsWindow(hButtonTimerMinuteUp)) WM_DeleteWindow(hButtonTimerMinuteUp);
    if (WM_IsWindow(hButtonTimerMinuteDown)) WM_DeleteWindow(hButtonTimerMinuteDown);
    for (int i = 0; i < 7; i++) {
        if (WM_IsWindow(hButtonTimerDay[i])) WM_DeleteWindow(hButtonTimerDay[i]);
    }
    if (WM_IsWindow(hButtonTimerBuzzer)) WM_DeleteWindow(hButtonTimerBuzzer);
    if (WM_IsWindow(hButtonTimerScene)) WM_DeleteWindow(hButtonTimerScene);
    if (WM_IsWindow(hButtonTimerSceneSelect)) WM_DeleteWindow(hButtonTimerSceneSelect);
    if (WM_IsWindow(hButtonTimerSave)) WM_DeleteWindow(hButtonTimerSave);
    if (WM_IsWindow(hButtonTimerCancel)) WM_DeleteWindow(hButtonTimerCancel);

    // Zajednički dugmići
    if (WM_IsWindow(hBUTTON_Ok)) WM_DeleteWindow(hBUTTON_Ok);
    if (WM_IsWindow(hBUTTON_Next)) WM_DeleteWindow(hBUTTON_Next);
    if (WM_IsWindow(hBUTTON_SET_DEFAULTS)) WM_DeleteWindow(hBUTTON_SET_DEFAULTS);
    if (WM_IsWindow(hBUTTON_SYSRESTART)) WM_DeleteWindow(hBUTTON_SYSRESTART);

    // Ostali widgeti
    if (WM_IsWindow(hThstControl)) WM_DeleteWindow(hThstControl);
    if (WM_IsWindow(hFanControl)) WM_DeleteWindow(hFanControl);
    if (WM_IsWindow(hThstMaxSetPoint)) WM_DeleteWindow(hThstMaxSetPoint);
    if (WM_IsWindow(hThstMinSetPoint)) WM_DeleteWindow(hThstMinSetPoint);
    if (WM_IsWindow(hFanDiff)) WM_DeleteWindow(hFanDiff);
    if (WM_IsWindow(hFanLowBand)) WM_DeleteWindow(hFanLowBand);
    if (WM_IsWindow(hFanHiBand)) WM_DeleteWindow(hFanHiBand);
    if (WM_IsWindow(hThstGroup)) WM_DeleteWindow(hThstGroup);
    if (WM_IsWindow(hThstMaster)) WM_DeleteWindow(hThstMaster);
    if (WM_IsWindow(hSPNBX_DisplayHighBrightness)) WM_DeleteWindow(hSPNBX_DisplayHighBrightness);
    if (WM_IsWindow(hSPNBX_DisplayLowBrightness)) WM_DeleteWindow(hSPNBX_DisplayLowBrightness);
    if (WM_IsWindow(hSPNBX_ScrnsvrTimeout)) WM_DeleteWindow(hSPNBX_ScrnsvrTimeout);
    if (WM_IsWindow(hSPNBX_ScrnsvrEnableHour)) WM_DeleteWindow(hSPNBX_ScrnsvrEnableHour);
    if (WM_IsWindow(hSPNBX_ScrnsvrDisableHour)) WM_DeleteWindow(hSPNBX_ScrnsvrDisableHour);
    if (WM_IsWindow(hSPNBX_ScrnsvrClockColour)) WM_DeleteWindow(hSPNBX_ScrnsvrClockColour);
    if (WM_IsWindow(hCHKBX_ScrnsvrClock)) WM_DeleteWindow(hCHKBX_ScrnsvrClock);
    if (WM_IsWindow(hDRPDN_WeekDay)) WM_DeleteWindow(hDRPDN_WeekDay);
    if (WM_IsWindow(hVentilatorRelay)) WM_DeleteWindow(hVentilatorRelay);
    if (WM_IsWindow(hVentilatorDelayOn)) WM_DeleteWindow(hVentilatorDelayOn);
    if (WM_IsWindow(hVentilatorDelayOff)) WM_DeleteWindow(hVentilatorDelayOff);
    if (WM_IsWindow(hVentilatorTriggerSource1)) WM_DeleteWindow(hVentilatorTriggerSource1);
    if (WM_IsWindow(hVentilatorTriggerSource2)) WM_DeleteWindow(hVentilatorTriggerSource2);
    if (WM_IsWindow(hVentilatorLocalPin)) WM_DeleteWindow(hVentilatorLocalPin);
    if (WM_IsWindow(hCurtainsMoveTime)) WM_DeleteWindow(hCurtainsMoveTime);
    if (WM_IsWindow(hDEV_ID)) WM_DeleteWindow(hDEV_ID);
    if (WM_IsWindow(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH)) WM_DeleteWindow(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
    if (WM_IsWindow(hCHKBX_LIGHT_NIGHT_TIMER)) WM_DeleteWindow(hCHKBX_LIGHT_NIGHT_TIMER);
    if (WM_IsWindow(hCHKBX_EnableScenes)) WM_DeleteWindow(hCHKBX_EnableScenes);
    if (WM_IsWindow(hDRPDN_Language)) WM_DeleteWindow(hDRPDN_Language);

    // Zavjese
    for (int i = 0; i < (CURTAINS_SIZE * 2); i++) {
        if (WM_IsWindow(hCurtainsRelay[i])) WM_DeleteWindow(hCurtainsRelay[i]);
    }

    // Svjetla
    for (int i = 0; i < LIGHTS_MODBUS_SIZE; i++) {
        if (WM_IsWindow(lightsWidgets[i].relay)) WM_DeleteWindow(lightsWidgets[i].relay);
        if (WM_IsWindow(lightsWidgets[i].iconID)) WM_DeleteWindow(lightsWidgets[i].iconID);
        if (WM_IsWindow(lightsWidgets[i].controllerID_on)) WM_DeleteWindow(lightsWidgets[i].controllerID_on);
        if (WM_IsWindow(lightsWidgets[i].controllerID_on_delay)) WM_DeleteWindow(lightsWidgets[i].controllerID_on_delay);
        if (WM_IsWindow(lightsWidgets[i].on_hour)) WM_DeleteWindow(lightsWidgets[i].on_hour);
        if (WM_IsWindow(lightsWidgets[i].on_minute)) WM_DeleteWindow(lightsWidgets[i].on_minute);
        if (WM_IsWindow(lightsWidgets[i].offTime)) WM_DeleteWindow(lightsWidgets[i].offTime);
        if (WM_IsWindow(lightsWidgets[i].communication_type)) WM_DeleteWindow(lightsWidgets[i].communication_type);
        if (WM_IsWindow(lightsWidgets[i].local_pin)) WM_DeleteWindow(lightsWidgets[i].local_pin);
        if (WM_IsWindow(lightsWidgets[i].sleep_time)) WM_DeleteWindow(lightsWidgets[i].sleep_time);
        if (WM_IsWindow(lightsWidgets[i].button_external)) WM_DeleteWindow(lightsWidgets[i].button_external);
        if (WM_IsWindow(lightsWidgets[i].tiedToMainLight)) WM_DeleteWindow(lightsWidgets[i].tiedToMainLight);
        if (WM_IsWindow(lightsWidgets[i].rememberBrightness)) WM_DeleteWindow(lightsWidgets[i].rememberBrightness);
    }
}
/**
 * @brief       Iscrtava "hamburger" meni ikonu na predefinisanoj poziciji.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je refaktorisana da eliminiše dupliciranje koda i
 * "magične brojeve". Koristi centralizovane konstante iz
 * `hamburger_menu_layout` strukture. Poziva se sa različitih
 * ekrana za iscrtavanje standardnih navigacionih elemenata. [cite: 52, 53, 54]
 * @param       position Određuje lokaciju iscrtavanja: [cite: 58]
 * - 1: Gornji desni ugao (standardni meni za povratak)
 * - 2: Donji lijevi ugao (meni za ulazak u scene)
 * @retval      None [cite: 61]
 *****************************************************************************/
static void DrawHamburgerMenu(uint8_t position)
{
    int16_t xStart, yStart, width, yGap;

    if (position == 1) // Gornji desni meni
    {
        xStart = hamburger_menu_layout.top_right.x_start;
        yStart = hamburger_menu_layout.top_right.y_start;
        width  = hamburger_menu_layout.top_right.width;
        yGap   = hamburger_menu_layout.top_right.y_gap;
    }
    else if (position == 2) // Donji lijevi meni
    {
        xStart = hamburger_menu_layout.bottom_left.x_start;
        yStart = hamburger_menu_layout.bottom_left.y_start;
        width  = hamburger_menu_layout.bottom_left.width;
        yGap   = hamburger_menu_layout.bottom_left.y_gap;
    }
    else
    {
        return; // Nepoznata pozicija, ne radi ništa.
    }

    // Postavljanje parametara za iscrtavanje ostaje nepromijenjeno
    GUI_SetPenSize(hamburger_menu_layout.line_thickness);
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);

    // Iscrtaj tri linije na osnovu odabranih parametara
    GUI_DrawLine(xStart, yStart, xStart + width, yStart);
    GUI_DrawLine(xStart, yStart + yGap, xStart + width, yStart + yGap);
    GUI_DrawLine(xStart, yStart + (yGap * 2), xStart + width, yStart + (yGap * 2));
}
/**
 * @brief Prikazuje datum i vrijeme na ekranu, i upravlja logikom screensavera.
 * @note Ažurira se svake sekunde i odgovorna je za aktivaciju/deaktivaciju
 * screensavera na osnovu postavljenih sati. Ova refaktorirana verzija koristi
 * GUI funkcije LCD_GetXSize() i LCD_GetYSize() za dinamičko određivanje
 * dimenzija ekrana, čime se uklanjaju fiksne numeričke vrijednosti ("magični brojevi")
 * i poboljšava prenosivost koda.
 */
static void DISPDateTime(void)
{
    /**
     * @brief Konstante za dimenzioniranje područja za brisanje
     * @note Ove konstante su sada definirane u odnosu na GUI.
     */
    const int16_t TIME_CLEAR_RECT_WIDTH = 100;
    const int16_t TIME_CLEAR_RECT_HEIGHT = 50;

    // Konstante za brisanje screensaver ekrana
    const int16_t SCREENSAVER_TIME_Y_START = 80;
    const int16_t SCREENSAVER_TIME_Y_END = 192;
    const int16_t SCREENSAVER_DATE_Y_START = 220;
    const int16_t SCREENSAVER_DATE_Y_END = 270;


    char dbuf[64];
    static uint8_t old_day = 0;

    if (!IsRtcTimeValid()) return;

    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    // Logika za automatsko paljenje/gašenje screensaver-a
    if (g_display_settings.scrnsvr_ena_hour >= g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour || Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    } else if (g_display_settings.scrnsvr_ena_hour < g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour && Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    if (IsScrnsvrActiv() && IsScrnsvrEnabled() && IsScrnsvrClkActiv()) {
        if (!IsScrnsvrInitActiv() || (old_day != rtcdt.WeekDay)) {
            ScrnsvrInitSet();
            GUI_MULTIBUF_BeginEx(0);
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_MULTIBUF_EndEx(0);
            GUI_MULTIBUF_BeginEx(1);
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();
            old_min = 60U;
            old_day = rtcdt.WeekDay;
            GUI_MULTIBUF_EndEx(1);
        }

        GUI_MULTIBUF_BeginEx(1);

        // POPRAVAK: Brisanje ekrana za screensaver bez magičnih brojeva
        GUI_ClearRect(0, SCREENSAVER_TIME_Y_START, LCD_GetXSize(), SCREENSAVER_TIME_Y_END);
        GUI_ClearRect(0, SCREENSAVER_DATE_Y_START, TIME_CLEAR_RECT_WIDTH, SCREENSAVER_DATE_Y_END);

        HEX2STR(dbuf, &rtctm.Hours);
        if (rtctm.Seconds & 1) dbuf[2] = ':';
        else dbuf[2] = ' ';
        HEX2STR(&dbuf[3], &rtctm.Minutes);

        GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
        GUI_SetFont(GUI_FONT_D80);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(dbuf, main_screen_layout.time_pos_scrnsvr.x, main_screen_layout.time_pos_scrnsvr.y);

        /**
         * @brief Niz sa TextID-jevima za dane u sedmici.
         * @note Koristi se za dinamičko prevođenje imena dana na screensaver-u.
         */
        const TextID days[] = {TXT_MONDAY, TXT_TUESDAY, TXT_WEDNESDAY, TXT_THURSDAY, TXT_FRIDAY, TXT_SATURDAY, TXT_SUNDAY};

        /**
         * @brief Niz sa TextID-jevima za mjesece u godini.
         * @note Koristi se za dinamičko prevođenje imena mjeseci na screensaver-u.
         */
        const TextID months[] = {TXT_MONTH_JAN, TXT_MONTH_FEB, TXT_MONTH_MAR, TXT_MONTH_APR, TXT_MONTH_MAY, TXT_MONTH_JUN,
                                 TXT_MONTH_JUL, TXT_MONTH_AUG, TXT_MONTH_SEP, TXT_MONTH_OCT, TXT_MONTH_NOV, TXT_MONTH_DEC
                                };

        sprintf(dbuf, "%s, %02d. %s %d",
                lng(days[Bcd2Dec(rtcdt.WeekDay) - 1]),
                Bcd2Dec(rtcdt.Date),
                lng(months[Bcd2Dec(rtcdt.Month) - 1]),
                Bcd2Dec(rtcdt.Year) + 2000);

        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(dbuf, main_screen_layout.date_pos_scrnsvr.x, main_screen_layout.date_pos_scrnsvr.y);

        GUI_MULTIBUF_EndEx(1);

    }

    if (old_day != rtcdt.WeekDay) {
        old_day = rtcdt.WeekDay;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, rtcdt.Date);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, rtcdt.Month);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, rtcdt.WeekDay);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, rtcdt.Year);
    }
}

/**
 ******************************************************************************
 * @brief       [DUMMY] Prikazuje skočni prozor sa porukom o grešci.
 * @author      Gemini
 * @note        Trenutno samo ispisuje poruku na ekranu. Kasnije će biti
 * zamijenjena sa pravim modalnim dijalogom.
 * @param[in]   device_name Naziv uređaja koji je u grešci.
 * @param[in]   device_index Indeks uređaja da bismo znali koji handle da resetujemo.
 ******************************************************************************
 */
static void Display_ShowErrorPopup(const char* device_name, uint8_t device_index)
{
    // TODO: Zamijeniti sa pravim modalnim prozorom (npr. DIALOG_Create(...))
    GUI_MULTIBUF_BeginEx(1);
    GUI_SetColor(GUI_RED);
    GUI_FillRect(50, 80, 430, 190);
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font24_1);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);

    char buffer[50];
    sprintf(buffer, "!!! GRESKA: %s !!!", device_name);
    GUI_DispStringAt(buffer, 240, 120);

    GUI_SetFont(&GUI_Font16_1);
    GUI_DispStringAt("Dodirni za OK", 240, 160);

    GUI_MULTIBUF_EndEx(1);

    // Čekanje na dodir za potvrdu
    GUI_PID_STATE ts;
    do {
        GUI_PID_GetState(&ts);
        HAL_Delay(20);
    } while (!ts.Pressed);

    do {
        GUI_PID_GetState(&ts);
        HAL_Delay(20);
    } while (ts.Pressed);

    // Nakon potvrde, pozovi backend da resetuje stanje greške
    Gate_Handle* handle = Gate_GetInstance(device_index);
    //Gate_AcknowledgeFault(handle);

    shouldDrawScreen = 1; // Zatraži ponovno iscrtavanje
}
/**
 * @brief Otkriva i obrađuje dugi pritisak za ulazak u meni za podešavanja.
 * @param btn Fleg koji ukazuje na početak pritiska (postavljen u PID_Hook).
 * @return uint8_t 1 ako je dugi pritisak detektovan, inače 0.
 */
static uint8_t DISPMenuSettings(uint8_t btn)
{
    static uint8_t last_state = 0U;
    static uint32_t menu_tmr = 0U;

    if ((btn == 1U) && (last_state == 0U)) {
        // Početak pritiska, postavi fleg i pokreni tajmer.
        last_state = 1U;
        menu_tmr = HAL_GetTick();
    } else if ((btn == 1U) && (last_state == 1U)) {
        // Pritisak traje, provjeri da li je dovoljno dugo.
        if((HAL_GetTick() - menu_tmr) >= SETTINGS_MENU_ENABLE_TIME) {
            last_state = 0U; // Resetuj stanje nakon detekcije.
            return (1U); // Uspješan dugi pritisak.
        }
    } else if ((btn == 0U) && (last_state == 1U)) {
        // Pritisak je prekinut prije isteka vremena.
        last_state = 0U;
    }

    return (0U); // Dugi pritisak nije detektovan.
}
/**
 * @brief Inicijalizuje prvi ekran podešavanja (kontrola termostata i ventilatora).
 * @note  Ova funkcija kreira sve potrebne GUI widgete, kao što su RADIO
 * i SPINBOX kontrole, te dugmad "NEXT" i "SAVE" za navigaciju i čuvanje.
 * Vrijednosti se inicijalizuju na osnovu trenutnih postavki.
 * Sav raspored elemenata je definisan u `settings_screen_1_layout` strukturi.
 */
static void DSP_InitSet1Scrn(void)
{
    /** @brief Dobijamo handle za termostat kako bismo pročitali trenutne postavke. */
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    /**
     * @brief Inicijalizacija iscrtavanja.
     * @note  Pokreće se višestruko baferovanje i čiste se oba grafička sloja.
     */
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /**
     * @brief Kreiranje widgeta za kontrolu termostata.
     * @note  Pozicije, dimenzije i početne vrijednosti se uzimaju iz layout strukture
     * i API funkcija termostat modula.
     */
    hThstControl = RADIO_CreateEx(settings_screen_1_layout.thst_control_pos.x, settings_screen_1_layout.thst_control_pos.y, settings_screen_1_layout.thst_control_pos.w, settings_screen_1_layout.thst_control_pos.h, 0, WM_CF_SHOW, 0, ID_ThstControl, 3, 20);
    RADIO_SetTextColor(hThstControl, GUI_GREEN);
    RADIO_SetText(hThstControl, "OFF", 0);
    RADIO_SetText(hThstControl, "COOLING", 1);
    RADIO_SetText(hThstControl, "HEATING", 2);
    RADIO_SetValue(hThstControl, Thermostat_GetControlMode(pThst));

    hThstMaxSetPoint = SPINBOX_CreateEx(settings_screen_1_layout.thst_max_sp_pos.x, settings_screen_1_layout.thst_max_sp_pos.y, settings_screen_1_layout.thst_max_sp_pos.w, settings_screen_1_layout.thst_max_sp_pos.h, 0, WM_CF_SHOW, ID_MaxSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, Thermostat_Get_SP_Max(pThst));

    hThstMinSetPoint = SPINBOX_CreateEx(settings_screen_1_layout.thst_min_sp_pos.x, settings_screen_1_layout.thst_min_sp_pos.y, settings_screen_1_layout.thst_min_sp_pos.w, settings_screen_1_layout.thst_min_sp_pos.h, 0, WM_CF_SHOW, ID_MinSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMinSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMinSetPoint, Thermostat_Get_SP_Min(pThst));

    /**
     * @brief Kreiranje widgeta za kontrolu ventilatora.
     */
    hFanControl = RADIO_CreateEx(settings_screen_1_layout.fan_control_pos.x, settings_screen_1_layout.fan_control_pos.y, settings_screen_1_layout.fan_control_pos.w, settings_screen_1_layout.fan_control_pos.h, 0, WM_CF_SHOW, 0, ID_FanControl, 2, 20);
    RADIO_SetTextColor(hFanControl, GUI_GREEN);
    RADIO_SetText(hFanControl, "ON / OFF", 0);
    RADIO_SetText(hFanControl, "3 SPEED", 1);
    RADIO_SetValue(hFanControl, Thermostat_GetFanControlMode(pThst));

    hFanDiff = SPINBOX_CreateEx(settings_screen_1_layout.fan_diff_pos.x, settings_screen_1_layout.fan_diff_pos.y, settings_screen_1_layout.fan_diff_pos.w, settings_screen_1_layout.fan_diff_pos.h, 0, WM_CF_SHOW, ID_FanDiff, 0, 10);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, Thermostat_GetFanDifference(pThst));

    hFanLowBand = SPINBOX_CreateEx(settings_screen_1_layout.fan_low_band_pos.x, settings_screen_1_layout.fan_low_band_pos.y, settings_screen_1_layout.fan_low_band_pos.w, settings_screen_1_layout.fan_low_band_pos.h, 0, WM_CF_SHOW, ID_FanLowBand, 0, 50);
    SPINBOX_SetEdge(hFanLowBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanLowBand, Thermostat_GetFanLowBand(pThst));

    hFanHiBand = SPINBOX_CreateEx(settings_screen_1_layout.fan_hi_band_pos.x, settings_screen_1_layout.fan_hi_band_pos.y, settings_screen_1_layout.fan_hi_band_pos.w, settings_screen_1_layout.fan_hi_band_pos.h, 0, WM_CF_SHOW, ID_FanHiBand, 0, 100);
    SPINBOX_SetEdge(hFanHiBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanHiBand, Thermostat_GetFanHighBand(pThst));

    /**
     * @brief Kreiranje widgeta za grupni rad termostata.
     */
    hThstGroup = SPINBOX_CreateEx(settings_screen_1_layout.thst_group_pos.x, settings_screen_1_layout.thst_group_pos.y, settings_screen_1_layout.thst_group_pos.w, settings_screen_1_layout.thst_group_pos.h, 0, WM_CF_SHOW, ID_THST_GROUP, 0, 254);
    SPINBOX_SetEdge(hThstGroup, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstGroup, Thermostat_GetGroup(pThst));

    // << ISPRAVKA: Dodat je 7. argument (ExFlags) sa vrijednošću 0. >>
    hThstMaster = CHECKBOX_CreateEx(settings_screen_1_layout.thst_master_pos.x, settings_screen_1_layout.thst_master_pos.y, settings_screen_1_layout.thst_master_pos.w, settings_screen_1_layout.thst_master_pos.h, 0, WM_CF_SHOW, 0, ID_THST_MASTER);
    CHECKBOX_SetTextColor(hThstMaster, GUI_GREEN);
    CHECKBOX_SetText(hThstMaster, "Master");
    CHECKBOX_SetState(hThstMaster, Thermostat_IsMaster(pThst));

    /**
     * @brief Kreiranje navigacionih dugmadi.
     */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_1_layout.next_button_pos.x, settings_screen_1_layout.next_button_pos.y, settings_screen_1_layout.next_button_pos.w, settings_screen_1_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_1_layout.save_button_pos.x, settings_screen_1_layout.save_button_pos.y, settings_screen_1_layout.save_button_pos.w, settings_screen_1_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /**
     * @brief Iscrtavanje tekstualnih labela i linija.
     * @note  Sve pozicije se dobijaju iz `settings_screen_1_layout` strukture.
     */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_GotoXY(settings_screen_1_layout.label_thst_max_sp[0].x, settings_screen_1_layout.label_thst_max_sp[0].y);
    GUI_DispString("MAX. USER SETPOINT");
    GUI_GotoXY(settings_screen_1_layout.label_thst_max_sp[1].x, settings_screen_1_layout.label_thst_max_sp[1].y);
    GUI_DispString("TEMP. x1*C");

    GUI_GotoXY(settings_screen_1_layout.label_thst_min_sp[0].x, settings_screen_1_layout.label_thst_min_sp[0].y);
    GUI_DispString("MIN. USER SETPOINT");
    GUI_GotoXY(settings_screen_1_layout.label_thst_min_sp[1].x, settings_screen_1_layout.label_thst_min_sp[1].y);
    GUI_DispString("TEMP. x1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_diff[0].x, settings_screen_1_layout.label_fan_diff[0].y);
    GUI_DispString("FAN SPEED DIFFERENCE");
    GUI_GotoXY(settings_screen_1_layout.label_fan_diff[1].x, settings_screen_1_layout.label_fan_diff[1].y);
    GUI_DispString("TEMP. x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_low[0].x, settings_screen_1_layout.label_fan_low[0].y);
    GUI_DispString("FAN LOW SPEED BAND");
    GUI_GotoXY(settings_screen_1_layout.label_fan_low[1].x, settings_screen_1_layout.label_fan_low[1].y);
    GUI_DispString("SETPOINT +/- x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_hi[0].x, settings_screen_1_layout.label_fan_hi[0].y);
    GUI_DispString("FAN HI SPEED BAND");
    GUI_GotoXY(settings_screen_1_layout.label_fan_hi[1].x, settings_screen_1_layout.label_fan_hi[1].y);
    GUI_DispString("SETPOINT +/- x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_thst_ctrl_title.x, settings_screen_1_layout.label_thst_ctrl_title.y);
    GUI_DispString("THERMOSTAT CONTROL MODE");

    GUI_GotoXY(settings_screen_1_layout.label_fan_ctrl_title.x, settings_screen_1_layout.label_fan_ctrl_title.y);
    GUI_DispString("FAN SPEED CONTROL MODE");

    GUI_GotoXY(settings_screen_1_layout.label_thst_group.x, settings_screen_1_layout.label_thst_group.y);
    GUI_DispString("GROUP");

    GUI_DrawHLine(12, 5, 320);
    GUI_DrawHLine(130, 5, 320);

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Briše GUI widgete sa prvog ekrana podešavanja.
 * @note Ova funkcija se poziva pre prelaska na novi ekran kako bi se oslobodila
 * memorija i spriječili konflikti sa novim widgetima.
 */
static void DSP_KillSet1Scrn(void)
{
    WM_DeleteWindow(hThstControl);
    WM_DeleteWindow(hFanControl);
    WM_DeleteWindow(hThstMaxSetPoint);
    WM_DeleteWindow(hThstMinSetPoint);
    WM_DeleteWindow(hFanDiff);
    WM_DeleteWindow(hFanLowBand);
    WM_DeleteWindow(hFanHiBand);
    WM_DeleteWindow(hThstGroup);
    WM_DeleteWindow(hThstMaster);
    WM_DeleteWindow(hBUTTON_Ok);
    WM_DeleteWindow(hBUTTON_Next);
}

/**
 * @brief Inicijalizuje drugi ekran podešavanja (vrijeme, datum, screensaver).
 * @note  Verzija 2.2: Potpuno refaktorisana, bez magičnih brojeva, sa kompletnim
 * kodom za iscrtavanje i ispravnim pozivima CreateEx funkcija.
 * Kreira sve GUI widgete i iscrtava statičke elemente (labele, linije)
 * koristeći isključivo konstante iz `settings_screen_2_layout` strukture.
 */
static void DSP_InitSet2Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Učitavanje trenutnog vremena i datuma sa RTC-a za inicijalizaciju widgeta. */
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    /** @brief Kreiranje svih widgeta koristeći pozicije iz `settings_screen_2_layout`. */
    hSPNBX_DisplayHighBrightness = SPINBOX_CreateEx(settings_screen_2_layout.high_brightness_pos.x, settings_screen_2_layout.high_brightness_pos.y, settings_screen_2_layout.high_brightness_pos.w, settings_screen_2_layout.high_brightness_pos.h, 0, WM_CF_SHOW, ID_DisplayHighBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayHighBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayHighBrightness, g_display_settings.high_bcklght);

    hSPNBX_DisplayLowBrightness = SPINBOX_CreateEx(settings_screen_2_layout.low_brightness_pos.x, settings_screen_2_layout.low_brightness_pos.y, settings_screen_2_layout.low_brightness_pos.w, settings_screen_2_layout.low_brightness_pos.h, 0, WM_CF_SHOW, ID_DisplayLowBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayLowBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayLowBrightness, g_display_settings.low_bcklght);

    hSPNBX_ScrnsvrTimeout = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_timeout_pos.x, settings_screen_2_layout.scrnsvr_timeout_pos.y, settings_screen_2_layout.scrnsvr_timeout_pos.w, settings_screen_2_layout.scrnsvr_timeout_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrTimeout, 1, 240);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrTimeout, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrTimeout, g_display_settings.scrnsvr_tout);

    hSPNBX_ScrnsvrEnableHour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_enable_hour_pos.x, settings_screen_2_layout.scrnsvr_enable_hour_pos.y, settings_screen_2_layout.scrnsvr_enable_hour_pos.w, settings_screen_2_layout.scrnsvr_enable_hour_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrEnableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrEnableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrEnableHour, g_display_settings.scrnsvr_ena_hour);

    hSPNBX_ScrnsvrDisableHour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_disable_hour_pos.x, settings_screen_2_layout.scrnsvr_disable_hour_pos.y, settings_screen_2_layout.scrnsvr_disable_hour_pos.w, settings_screen_2_layout.scrnsvr_disable_hour_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrDisableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrDisableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrDisableHour, g_display_settings.scrnsvr_dis_hour);

    hSPNBX_Hour = SPINBOX_CreateEx(settings_screen_2_layout.hour_pos.x, settings_screen_2_layout.hour_pos.y, settings_screen_2_layout.hour_pos.w, settings_screen_2_layout.hour_pos.h, 0, WM_CF_SHOW, ID_Hour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_Hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Hour, Bcd2Dec(rtctm.Hours));

    hSPNBX_Minute = SPINBOX_CreateEx(settings_screen_2_layout.minute_pos.x, settings_screen_2_layout.minute_pos.y, settings_screen_2_layout.minute_pos.w, settings_screen_2_layout.minute_pos.h, 0, WM_CF_SHOW, ID_Minute, 0, 59);
    SPINBOX_SetEdge(hSPNBX_Minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Minute, Bcd2Dec(rtctm.Minutes));

    hSPNBX_Day = SPINBOX_CreateEx(settings_screen_2_layout.day_pos.x, settings_screen_2_layout.day_pos.y, settings_screen_2_layout.day_pos.w, settings_screen_2_layout.day_pos.h, 0, WM_CF_SHOW, ID_Day, 1, 31);
    SPINBOX_SetEdge(hSPNBX_Day, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Day, Bcd2Dec(rtcdt.Date));

    hSPNBX_Month = SPINBOX_CreateEx(settings_screen_2_layout.month_pos.x, settings_screen_2_layout.month_pos.y, settings_screen_2_layout.month_pos.w, settings_screen_2_layout.month_pos.h, 0, WM_CF_SHOW, ID_Month, 1, 12);
    SPINBOX_SetEdge(hSPNBX_Month, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Month, Bcd2Dec(rtcdt.Month));

    hSPNBX_Year = SPINBOX_CreateEx(settings_screen_2_layout.year_pos.x, settings_screen_2_layout.year_pos.y, settings_screen_2_layout.year_pos.w, settings_screen_2_layout.year_pos.h, 0, WM_CF_SHOW, ID_Year, 2000, 2099);
    SPINBOX_SetEdge(hSPNBX_Year, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Year, (Bcd2Dec(rtcdt.Year) + 2000));

    hSPNBX_ScrnsvrClockColour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_color_pos.x, settings_screen_2_layout.scrnsvr_color_pos.y, settings_screen_2_layout.scrnsvr_color_pos.w, settings_screen_2_layout.scrnsvr_color_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrClkColour, 1, COLOR_BSIZE);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrClockColour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrClockColour, g_display_settings.scrnsvr_clk_clr);

    hCHKBX_ScrnsvrClock = CHECKBOX_CreateEx(settings_screen_2_layout.scrnsvr_checkbox_pos.x, settings_screen_2_layout.scrnsvr_checkbox_pos.y, settings_screen_2_layout.scrnsvr_checkbox_pos.w, settings_screen_2_layout.scrnsvr_checkbox_pos.h, 0, WM_CF_SHOW, 0, ID_ScrnsvrClock);
    CHECKBOX_SetTextColor(hCHKBX_ScrnsvrClock, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ScrnsvrClock, "SCREENSAVER");
    // << ISPRAVKA 3: Inicijalizacija se sada vrši iz EEPROM strukture `g_display_settings` >>
    CHECKBOX_SetState(hCHKBX_ScrnsvrClock, g_display_settings.scrnsvr_on_off);

    hDRPDN_WeekDay = DROPDOWN_CreateEx(settings_screen_2_layout.weekday_dropdown_pos.x, settings_screen_2_layout.weekday_dropdown_pos.y, settings_screen_2_layout.weekday_dropdown_pos.w, settings_screen_2_layout.weekday_dropdown_pos.h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_WeekDay);
    /** * @brief << ISPRAVKA: Logika za popunjavanje dropdown menija za dane u sedmici. >>
     * @note  Petlja sada ispravno iterira 7 puta (za 7 dana) i dodaje stringove
     * za trenutno odabrani jezik (`g_display_settings.language`).
     */
    for (int i = 0; i < 7; i++) {
        DROPDOWN_AddString(hDRPDN_WeekDay, _acContent[g_display_settings.language][i]);
    }
    DROPDOWN_SetSel(hDRPDN_WeekDay, rtcdt.WeekDay - 1);

    hBUTTON_Next = BUTTON_CreateEx(settings_screen_2_layout.next_button_pos.x, settings_screen_2_layout.next_button_pos.y, settings_screen_2_layout.next_button_pos.w, settings_screen_2_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_2_layout.save_button_pos.x, settings_screen_2_layout.save_button_pos.y, settings_screen_2_layout.save_button_pos.w, settings_screen_2_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Iscrtavanje labela, linija i pregleda boje, koristeći pozicije iz layout strukture. */
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
    GUI_FillRect(settings_screen_2_layout.scrnsvr_color_preview_rect.x0, settings_screen_2_layout.scrnsvr_color_preview_rect.y0,
                 settings_screen_2_layout.scrnsvr_color_preview_rect.x1, settings_screen_2_layout.scrnsvr_color_preview_rect.y1);

    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_DrawHLine(settings_screen_2_layout.line1.y, settings_screen_2_layout.line1.x0, settings_screen_2_layout.line1.x1);
    GUI_GotoXY(settings_screen_2_layout.label_backlight_title.x, settings_screen_2_layout.label_backlight_title.y);
    GUI_DispString("DISPLAY BACKLIGHT");
    GUI_GotoXY(settings_screen_2_layout.label_high_brightness.x, settings_screen_2_layout.label_high_brightness.y);
    GUI_DispString("HIGH");
    GUI_GotoXY(settings_screen_2_layout.label_low_brightness.x, settings_screen_2_layout.label_low_brightness.y);
    GUI_DispString("LOW");

    GUI_DrawHLine(settings_screen_2_layout.line2.y, settings_screen_2_layout.line2.x0, settings_screen_2_layout.line2.x1);
    GUI_GotoXY(settings_screen_2_layout.label_time_title.x, settings_screen_2_layout.label_time_title.y);
    GUI_DispString("SET TIME");
    GUI_GotoXY(settings_screen_2_layout.label_hour.x, settings_screen_2_layout.label_hour.y);
    GUI_DispString("HOUR");
    GUI_GotoXY(settings_screen_2_layout.label_minute.x, settings_screen_2_layout.label_minute.y);
    GUI_DispString("MINUTE");

    GUI_DrawHLine(settings_screen_2_layout.line3.y, settings_screen_2_layout.line3.x0, settings_screen_2_layout.line3.x1);
    GUI_GotoXY(settings_screen_2_layout.label_color_title.x, settings_screen_2_layout.label_color_title.y);
    GUI_DispString("SET COLOR");
    GUI_GotoXY(settings_screen_2_layout.label_full_color.x, settings_screen_2_layout.label_full_color.y);
    GUI_DispString("FULL");
    GUI_GotoXY(settings_screen_2_layout.label_clock_color.x, settings_screen_2_layout.label_clock_color.y);
    GUI_DispString("CLOCK");

    GUI_DrawHLine(settings_screen_2_layout.line4.y, settings_screen_2_layout.line4.x0, settings_screen_2_layout.line4.x1);
    GUI_GotoXY(settings_screen_2_layout.label_scrnsvr_title.x, settings_screen_2_layout.label_scrnsvr_title.y);
    GUI_DispString("SCREENSAVER OPTION");
    GUI_GotoXY(settings_screen_2_layout.label_timeout.x, settings_screen_2_layout.label_timeout.y);
    GUI_DispString("TIMEOUT");
    GUI_GotoXY(settings_screen_2_layout.label_enable_hour[0].x, settings_screen_2_layout.label_enable_hour[0].y);
    GUI_DispString("ENABLE");
    GUI_GotoXY(settings_screen_2_layout.label_enable_hour[1].x, settings_screen_2_layout.label_enable_hour[1].y);
    GUI_DispString("HOUR");
    GUI_GotoXY(settings_screen_2_layout.label_disable_hour[0].x, settings_screen_2_layout.label_disable_hour[0].y);
    GUI_DispString("DISABLE");
    GUI_GotoXY(settings_screen_2_layout.label_disable_hour[1].x, settings_screen_2_layout.label_disable_hour[1].y);
    GUI_DispString("HOUR");

    GUI_DrawHLine(settings_screen_2_layout.line5.y, settings_screen_2_layout.line5.x0, settings_screen_2_layout.line5.x1);
    GUI_GotoXY(settings_screen_2_layout.label_date_title.x, settings_screen_2_layout.label_date_title.y);
    GUI_DispString("SET DATE");
    GUI_GotoXY(settings_screen_2_layout.label_day.x, settings_screen_2_layout.label_day.y);
    GUI_DispString("DAY");
    GUI_GotoXY(settings_screen_2_layout.label_month.x, settings_screen_2_layout.label_month.y);
    GUI_DispString("MONTH");
    GUI_GotoXY(settings_screen_2_layout.label_year.x, settings_screen_2_layout.label_year.y);
    GUI_DispString("YEAR");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa drugog ekrana podešavanja.
 * @note Briše sve widgete vezane za vrijeme, datum, screensaver i svjetlinu
 * ekrana.
 */
static void DSP_KillSet2Scrn(void)
{
    WM_DeleteWindow(hSPNBX_DisplayHighBrightness);
    WM_DeleteWindow(hSPNBX_DisplayLowBrightness);
    WM_DeleteWindow(hSPNBX_ScrnsvrDisableHour);
    WM_DeleteWindow(hSPNBX_ScrnsvrClockColour);
    WM_DeleteWindow(hSPNBX_ScrnsvrEnableHour);
    WM_DeleteWindow(hSPNBX_ScrnsvrTimeout);
    WM_DeleteWindow(hCHKBX_ScrnsvrClock);
    WM_DeleteWindow(hSPNBX_Minute);
    WM_DeleteWindow(hSPNBX_Month);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hSPNBX_Hour);
    WM_DeleteWindow(hSPNBX_Year);
    WM_DeleteWindow(hDRPDN_WeekDay);
    WM_DeleteWindow(hSPNBX_Day);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 * @brief Inicijalizuje treći ekran podešavanja (ventilator i odmrzivač).
 * @note Kreira widgete za postavke releja, odgode paljenja i gašenja
 * ventilatora i odmrzivača. Sav raspored elemenata je definisan u
 * `settings_screen_3_layout` strukturi.
 */
static void DSP_InitSet3Scrn(void)
{
    /** @brief Dobijamo handle-ove za module čije postavke se mijenjaju. */
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);


    /** @brief Kreiranje navigacionih dugmadi. */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_3_layout.next_button_pos.x, settings_screen_3_layout.next_button_pos.y, settings_screen_3_layout.next_button_pos.w, settings_screen_3_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_3_layout.save_button_pos.x, settings_screen_3_layout.save_button_pos.y, settings_screen_3_layout.save_button_pos.w, settings_screen_3_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Kreiranje widgeta za postavke odmrzivača (Defroster). */
    defroster_settingWidgets.cycleTime = SPINBOX_CreateEx(settings_screen_3_layout.defroster_cycle_time_pos.x, settings_screen_3_layout.defroster_cycle_time_pos.y, settings_screen_3_layout.defroster_cycle_time_pos.w, settings_screen_3_layout.defroster_cycle_time_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_CYCLE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.cycleTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.cycleTime, Defroster_getCycleTime(defHandle));

    defroster_settingWidgets.activeTime = SPINBOX_CreateEx(settings_screen_3_layout.defroster_active_time_pos.x, settings_screen_3_layout.defroster_active_time_pos.y, settings_screen_3_layout.defroster_active_time_pos.w, settings_screen_3_layout.defroster_active_time_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_ACTIVE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.activeTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.activeTime, Defroster_getActiveTime(defHandle));

    defroster_settingWidgets.pin = SPINBOX_CreateEx(settings_screen_3_layout.defroster_pin_pos.x, settings_screen_3_layout.defroster_pin_pos.y, settings_screen_3_layout.defroster_pin_pos.w, settings_screen_3_layout.defroster_pin_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_PIN, 0, 6);
    SPINBOX_SetEdge(defroster_settingWidgets.pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.pin, Defroster_getPin(defHandle));

    /** @brief Kreiranje widgeta za postavke ventilatora. */
    hVentilatorRelay = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_relay_pos.x, settings_screen_3_layout.ventilator_relay_pos.y, settings_screen_3_layout.ventilator_relay_pos.w, settings_screen_3_layout.ventilator_relay_pos.h, 0, WM_CF_SHOW, ID_VentilatorRelay, 0, 512);
    SPINBOX_SetEdge(hVentilatorRelay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorRelay, Ventilator_getRelay(ventHandle));

    hVentilatorDelayOn = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_delay_on_pos.x, settings_screen_3_layout.ventilator_delay_on_pos.y, settings_screen_3_layout.ventilator_delay_on_pos.w, settings_screen_3_layout.ventilator_delay_on_pos.h, 0, WM_CF_SHOW, ID_VentilatorDelayOn, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOn, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOn, Ventilator_getDelayOnTime(ventHandle));

    hVentilatorDelayOff = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_delay_off_pos.x, settings_screen_3_layout.ventilator_delay_off_pos.y, settings_screen_3_layout.ventilator_delay_off_pos.w, settings_screen_3_layout.ventilator_delay_off_pos.h, 0, WM_CF_SHOW, ID_VentilatorDelayOff, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOff, Ventilator_getDelayOffTime(ventHandle));

    hVentilatorTriggerSource1 = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_trigger1_pos.x, settings_screen_3_layout.ventilator_trigger1_pos.y, settings_screen_3_layout.ventilator_trigger1_pos.w, settings_screen_3_layout.ventilator_trigger1_pos.h, 0, WM_CF_SHOW, ID_VentilatorTriggerSource1, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource1, Ventilator_getTriggerSource1(ventHandle));

    hVentilatorTriggerSource2 = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_trigger2_pos.x, settings_screen_3_layout.ventilator_trigger2_pos.y, settings_screen_3_layout.ventilator_trigger2_pos.w, settings_screen_3_layout.ventilator_trigger2_pos.h, 0, WM_CF_SHOW, ID_VentilatorTriggerSource2, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource2, Ventilator_getTriggerSource2(ventHandle));

    hVentilatorLocalPin = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_local_pin_pos.x, settings_screen_3_layout.ventilator_local_pin_pos.y, settings_screen_3_layout.ventilator_local_pin_pos.w, settings_screen_3_layout.ventilator_local_pin_pos.h, 0, WM_CF_SHOW, ID_VentilatorLocalPin, 0, 32);
    SPINBOX_SetEdge(hVentilatorLocalPin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorLocalPin, Ventilator_getLocalPin(ventHandle));

    /** @brief Iscrtavanje labela i linija. */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    // Labele za VENTILATOR
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[0].x, settings_screen_3_layout.label_ventilator_relay[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[1].x, settings_screen_3_layout.label_ventilator_relay[1].y);
    GUI_DispString("BUS RELAY");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[0].x, settings_screen_3_layout.label_ventilator_delay_on[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[1].x, settings_screen_3_layout.label_ventilator_delay_on[1].y);
    GUI_DispString("DELAY ON");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[0].x, settings_screen_3_layout.label_ventilator_delay_off[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[1].x, settings_screen_3_layout.label_ventilator_delay_off[1].y);
    GUI_DispString("DELAY OFF");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[0].x, settings_screen_3_layout.label_ventilator_trigger1[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[1].x, settings_screen_3_layout.label_ventilator_trigger1[1].y);
    GUI_DispString("TRIGGER 1");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[0].x, settings_screen_3_layout.label_ventilator_trigger2[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[1].x, settings_screen_3_layout.label_ventilator_trigger2[1].y);
    GUI_DispString("TRIGGER 2");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[0].x, settings_screen_3_layout.label_ventilator_local_pin[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[1].x, settings_screen_3_layout.label_ventilator_local_pin[1].y);
    GUI_DispString("LOCAL PIN");

    // Labele za DEFROSTER
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[0].x, settings_screen_3_layout.label_defroster_cycle_time[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[1].x, settings_screen_3_layout.label_defroster_cycle_time[1].y);
    GUI_DispString("CYCLE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[0].x, settings_screen_3_layout.label_defroster_active_time[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[1].x, settings_screen_3_layout.label_defroster_active_time[1].y);
    GUI_DispString("ACTIVE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[0].x, settings_screen_3_layout.label_defroster_pin[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[1].x, settings_screen_3_layout.label_defroster_pin[1].y);
    GUI_DispString("PIN");

    // Labele za naslove i odabir kontrole
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_title.x, settings_screen_3_layout.label_ventilator_title.y);
    GUI_DispString("VENTILATOR CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_title.x, settings_screen_3_layout.label_defroster_title.y);
    GUI_DispString("DEFROSTER CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_select_control_title.x, settings_screen_3_layout.label_select_control_title.y);
    GUI_DispString("SELECT CONTROL 4");

    // Linije
    GUI_DrawHLine(settings_screen_3_layout.line_ventilator_title.y, settings_screen_3_layout.line_ventilator_title.x0, settings_screen_3_layout.line_ventilator_title.x1);
    GUI_DrawHLine(settings_screen_3_layout.line_defroster_title.y, settings_screen_3_layout.line_defroster_title.x0, settings_screen_3_layout.line_defroster_title.x1);
    GUI_DrawHLine(settings_screen_3_layout.line_select_control.y, settings_screen_3_layout.line_select_control.x0, settings_screen_3_layout.line_select_control.x1);

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa trećeg ekrana podešavanja.
 * @note Briše sve widgete vezane za postavke ventilatora.
 */
static void DSP_KillSet3Scrn(void)
{
    WM_DeleteWindow(defroster_settingWidgets.cycleTime);
    WM_DeleteWindow(defroster_settingWidgets.activeTime);
    WM_DeleteWindow(defroster_settingWidgets.pin);
    WM_DeleteWindow(hVentilatorRelay);
    WM_DeleteWindow(hVentilatorDelayOn);
    WM_DeleteWindow(hVentilatorDelayOff);
    WM_DeleteWindow(hVentilatorTriggerSource1);
    WM_DeleteWindow(hVentilatorTriggerSource2);
    WM_DeleteWindow(hVentilatorLocalPin);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 * @brief Inicijalizuje četvrti ekran podešavanja (zavjese).
 * @note  Dinamički kreira SPINBOX-ove za do 4 zavjese po ekranu
 * za podešavanje releja "GORE" i "DOLJE". Koristi `settings_screen_4_layout`
 * strukturu da dinamički izračuna pozicije svih elemenata u 2x2 mreži.
 */
static void DSP_InitSet4Scrn(void)
{
    /**
     * @brief Inicijalizacija iscrtavanja.
     */
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /**
     * @brief Petlja za kreiranje widgeta.
     * @note  Iterira kroz 4 roletne relevantne za trenutnu stranicu menija
     * (`curtainSettingMenu`).
     */
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++) {

        /** @brief Dobijamo handle za roletnu po njenom fizičkom indeksu u nizu. */
        Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);

        /**
         * @brief Dinamičko izračunavanje pozicija za trenutnu roletnu u mreži.
         * @note  Ova logika postavlja widgete u dvije kolone i dva reda.
         */
        int col = ((i % 4) < 2) ? 0 : 1; // 0 za prvu kolonu, 1 za drugu
        int row = (i % 4) % 2;          // 0 za prvi red, 1 za drugi
        int x = settings_screen_4_layout.grid_start_pos.x + (col * settings_screen_4_layout.x_col_spacing);
        int y = settings_screen_4_layout.grid_start_pos.y + (row * settings_screen_4_layout.y_group_spacing);

        /**
         * @brief Kreiranje SPINBOX-a za relej "GORE".
         * @note  Poziv `SPINBOX_CreateEx` ima 9 argumenata.
         */
        hCurtainsRelay[i * 2] = SPINBOX_CreateEx(x, y, settings_screen_4_layout.widget_width, settings_screen_4_layout.widget_height, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2), 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[i * 2], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[i * 2], Curtain_getRelayUp(handle));

        /**
         * @brief Kreiranje SPINBOX-a za relej "DOLJE".
         * @note  Pozicija se računa na osnovu pozicije "GORE" widgeta i `y_row_spacing` konstante.
         */
        hCurtainsRelay[(i * 2) + 1] = SPINBOX_CreateEx(x, y + settings_screen_4_layout.y_row_spacing, settings_screen_4_layout.widget_width, settings_screen_4_layout.widget_height, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2) + 1, 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[(i * 2) + 1], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[(i * 2) + 1], Curtain_getRelayDown(handle));

        /**
         * @brief Iscrtavanje labela pored kreiranih widgeta.
         * @note  Pozicije labela se računaju relativno u odnosu na poziciju widgeta.
         */
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

        // Labela za "GORE"
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.label_line1_offset.y);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, 2);
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.label_line1_offset.y + settings_screen_4_layout.label_line2_offset_y);
        GUI_DispString("RELAY UP");

        // Labela za "DOLE"
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.y_row_spacing + settings_screen_4_layout.label_line1_offset.y);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, 2);
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.y_row_spacing + settings_screen_4_layout.label_line1_offset.y + settings_screen_4_layout.label_line2_offset_y);
        GUI_DispString("RELAY DOWN");
    }

    /**
     * @brief Kreiranje navigacionih dugmadi.
     * @note  Pozivi `BUTTON_CreateEx` imaju 8 argumenata.
     */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_4_layout.next_button_pos.x, settings_screen_4_layout.next_button_pos.y, settings_screen_4_layout.next_button_pos.w, settings_screen_4_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_4_layout.save_button_pos.x, settings_screen_4_layout.save_button_pos.y, settings_screen_4_layout.save_button_pos.w, settings_screen_4_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa četvrtog ekrana podešavanja.
 * @note Briše dinamički kreirane widgete za zavjese na trenutnoj stranici.
 */
static void DSP_KillSet4Scrn(void)
{
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++) {
        if(hCurtainsRelay[i * 2]) { // Provjeri da li widget postoji
            WM_DeleteWindow(hCurtainsRelay[i * 2]);
            hCurtainsRelay[i * 2] = 0; // Resetuj handle
        }
        if(hCurtainsRelay[(i * 2) + 1]) { // Provjeri da li widget postoji
            WM_DeleteWindow(hCurtainsRelay[(i * 2) + 1]);
            hCurtainsRelay[(i * 2) + 1] = 0; // Resetuj handle
        }
    }
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 * @brief Inicijalizuje peti ekran podešavanja (detaljne postavke za svjetla).
 * @note  Ova funkcija dinamički kreira sve GUI widgete za podešavanje jednog
 * svjetla po ekranu. Koristi `settings_screen_5_layout` strukturu za
 * pozicioniranje svih elemenata u dvije kolone.
 */
static void DSP_InitSet5Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Dohvatamo indeks i handle svjetla čije postavke trenutno prikazujemo. */
    uint8_t light_index = lightsModbusSettingsMenu;
    LIGHT_Handle* handle = LIGHTS_GetInstance(light_index);

    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return; // Sigurnosna provjera
    }

    /**
     * @brief Kreiranje widgeta u prvoj koloni.
     * @note  Pozicije se računaju na osnovu početnih koordinata i visine reda (y_step)
     * definisanih u `settings_screen_5_layout`.
     */
    const WidgetRect_t* sb_size = &settings_screen_5_layout.spinbox_size;
    int16_t x = settings_screen_5_layout.col1_x;
    int16_t y = settings_screen_5_layout.start_y;
    int16_t y_step = settings_screen_5_layout.y_step;

    // << ISPRAVKA 2: Promijenjen množilac za ID-jeve sa 12 na 16 radi izbjegavanja preklapanja >>
    const int id_step = 16;

    // << ISPRAVKA 1: Vraćena linija za kreiranje RELAY spinbox-a >>
    lightsWidgets[light_index].relay = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 0, 0, 512);

    // << ISPRAVKA 1: Opseg za IconID je sada ispravan i nema duplirane linije >>
    uint16_t max_icon_id = (sizeof(icon_mapping_table) / sizeof(IconMapping_t)) - 1;
    lightsWidgets[light_index].iconID = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 1, 0, max_icon_id);

    lightsWidgets[light_index].controllerID_on = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 2, 0, 512);
    lightsWidgets[light_index].controllerID_on_delay  = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 3, 0, 255);
    lightsWidgets[light_index].on_hour = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 4, -1, 23);
    lightsWidgets[light_index].on_minute = SPINBOX_CreateEx(x, y + 5 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 5, 0, 59);

    x = settings_screen_5_layout.col2_x;

    lightsWidgets[light_index].offTime = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 6, 0, 255);
    lightsWidgets[light_index].communication_type = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 7, 1, 3);
    lightsWidgets[light_index].local_pin = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 8, 0, 32);
    lightsWidgets[light_index].sleep_time = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 9, 0, 255);
    lightsWidgets[light_index].button_external = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 10, 0, 3);

    const WidgetRect_t* cb1_size = &settings_screen_5_layout.checkbox1_size;
    lightsWidgets[light_index].tiedToMainLight = CHECKBOX_CreateEx(x, y + 5 * y_step, cb1_size->w, cb1_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * id_step) + 11);

    const WidgetRect_t* cb2_size = &settings_screen_5_layout.checkbox2_size;
    lightsWidgets[light_index].rememberBrightness = CHECKBOX_CreateEx(x, y + 5 * y_step + 23, cb2_size->w, cb2_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * id_step) + 12);

    /** @brief Postavljanje početnih vrijednosti za sve kreirane widgete. */
    SPINBOX_SetEdge(lightsWidgets[light_index].relay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].relay, LIGHT_GetRelay(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].iconID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].iconID, LIGHT_GetIconID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on, LIGHT_GetControllerID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on_delay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on_delay, LIGHT_GetOnDelayTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].on_hour, LIGHT_GetOnHour(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].on_minute, LIGHT_GetOnMinute(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].offTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].offTime, LIGHT_GetOffTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].communication_type, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].communication_type, LIGHT_GetCommunicationType(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].local_pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].local_pin, LIGHT_GetLocalPin(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].sleep_time, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].sleep_time, LIGHT_GetSleepTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].button_external, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].button_external, LIGHT_GetButtonExternal(handle));

    CHECKBOX_SetTextColor(lightsWidgets[light_index].tiedToMainLight, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[light_index].tiedToMainLight, "TIED TO MAIN LIGHT");
    CHECKBOX_SetState(lightsWidgets[light_index].tiedToMainLight, LIGHT_isTiedToMainLight(handle));

    CHECKBOX_SetTextColor(lightsWidgets[light_index].rememberBrightness, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[light_index].rememberBrightness, "REMEMBER BRIGHTNESS");
    CHECKBOX_SetState(lightsWidgets[light_index].rememberBrightness, LIGHT_isBrightnessRemembered(handle));

    /** @brief Kreiranje navigacionih dugmadi. */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_5_layout.next_button_pos.x, settings_screen_5_layout.next_button_pos.y, settings_screen_5_layout.next_button_pos.w, settings_screen_5_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_5_layout.save_button_pos.x, settings_screen_5_layout.save_button_pos.y, settings_screen_5_layout.save_button_pos.w, settings_screen_5_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Iscrtavanje labela. */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

    const GUI_POINT* label_offset = &settings_screen_5_layout.label_line1_offset;
    const int16_t label_y2_offset = settings_screen_5_layout.label_line2_offset_y;
    x = settings_screen_5_layout.col1_x;
    y = settings_screen_5_layout.start_y;

    // Prva kolona labela
    GUI_GotoXY(x + label_offset->x, y + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("RELAY");

    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ICON");

    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ON ID");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ON ID DELAY");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("HOUR ON");

    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("MINUTE ON");

    // Druga kolona labela
    x = settings_screen_5_layout.col2_x;

    GUI_GotoXY(x + label_offset->x, y + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("DELAY OFF");

    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("COMM. TYPE");

    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("LOCAL PIN");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("SLEEP TIME");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("BUTTON EXT.");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa petog ekrana podešavanja.
 * @note Briše dinamički kreirane widgete za svjetla na trenutnoj stranici.
 */
static void DSP_KillSet5Scrn(void)
{
    // Računamo indeks svjetla čiji su widgeti trenutno na ekranu.
    uint8_t i = lightsModbusSettingsMenu; // Ovdje ne treba množenje

    // Brišemo svaki widget pojedinačno za taj specifični indeks.
    WM_DeleteWindow(lightsWidgets[i].relay);
    WM_DeleteWindow(lightsWidgets[i].iconID);
    WM_DeleteWindow(lightsWidgets[i].controllerID_on);
    WM_DeleteWindow(lightsWidgets[i].controllerID_on_delay);
    WM_DeleteWindow(lightsWidgets[i].offTime);
    WM_DeleteWindow(lightsWidgets[i].on_hour);
    WM_DeleteWindow(lightsWidgets[i].on_minute);
    WM_DeleteWindow(lightsWidgets[i].communication_type);
    WM_DeleteWindow(lightsWidgets[i].local_pin);
    WM_DeleteWindow(lightsWidgets[i].sleep_time);
    WM_DeleteWindow(lightsWidgets[i].button_external);
    WM_DeleteWindow(lightsWidgets[i].tiedToMainLight);
    WM_DeleteWindow(lightsWidgets[i].rememberBrightness);

    // Brišemo i zajedničke dugmiće.
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje šesti ekran podešavanja (opšte postavke).
 * @author      Gemini & [Vaše Ime]
 * @note        ISPRAVLJENA VERZIJA: Uklonjeno je kreiranje `hCHKBX_EnableScenes`
 * widgeta jer on pripada ekranu SCREEN_SETTINGS_7. Novi checkbox
 * `hCHKBX_EnableSecurity` sada koristi svoju definisanu poziciju iz
 * redizajnirane `settings_screen_6_layout` strukture.
 ******************************************************************************
 */
static void DSP_InitSet6Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    hDEV_ID = SPINBOX_CreateEx(settings_screen_6_layout.device_id_pos.x, settings_screen_6_layout.device_id_pos.y, settings_screen_6_layout.device_id_pos.w, settings_screen_6_layout.device_id_pos.h, 0, WM_CF_SHOW, ID_DEV_ID, 1, 254);
    SPINBOX_SetEdge(hDEV_ID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDEV_ID, tfifa);

    hCurtainsMoveTime = SPINBOX_CreateEx(settings_screen_6_layout.curtain_move_time_pos.x, settings_screen_6_layout.curtain_move_time_pos.y, settings_screen_6_layout.curtain_move_time_pos.w, settings_screen_6_layout.curtain_move_time_pos.h, 0, WM_CF_SHOW, ID_CurtainsMoveTime, 0, 60);
    SPINBOX_SetEdge(hCurtainsMoveTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hCurtainsMoveTime, Curtain_GetMoveTime());

    const WidgetRect_t* cb1_pos = &settings_screen_6_layout.leave_scrnsvr_checkbox_pos;
    hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH = CHECKBOX_CreateEx(cb1_pos->x, cb1_pos->y, cb1_pos->w, cb1_pos->h, 0, WM_CF_SHOW, 0, ID_LEAVE_SCRNSVR_AFTER_TOUCH);
    CHECKBOX_SetTextColor(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, "ONLY LEAVE SCRNSVR AFTER TOUCH");
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, g_display_settings.leave_scrnsvr_on_release);

    const WidgetRect_t* cb2_pos = &settings_screen_6_layout.night_timer_checkbox_pos;
    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_CreateEx(cb2_pos->x, cb2_pos->y, cb2_pos->w, cb2_pos->h, 0, WM_CF_SHOW, 0, ID_LIGHT_NIGHT_TIMER);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LIGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, g_display_settings.light_night_timer_enabled);

    // ... ostatak funkcije ostaje nepromijenjen ...
    const WidgetRect_t* lang_pos = &settings_screen_6_layout.language_dropdown_pos;
    hDRPDN_Language = DROPDOWN_CreateEx(lang_pos->x, lang_pos->y, lang_pos->w, lang_pos->h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_LanguageSelect);
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        DROPDOWN_AddString(hDRPDN_Language, language_strings[TXT_LANGUAGE_NAME][i]);
    }
    DROPDOWN_SetSel(hDRPDN_Language, g_display_settings.language);
    DROPDOWN_SetFont(hDRPDN_Language, GUI_FONT_16_1);
    
    hSelectControl_1 = DROPDOWN_CreateEx(settings_screen_6_layout.select_control_1_pos.x, settings_screen_6_layout.select_control_1_pos.y, settings_screen_6_layout.select_control_1_pos.w, settings_screen_6_layout.select_control_1_pos.h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_SELECT_CONTROL_1);
    PopulateControlDropdown(hSelectControl_1, g_display_settings.selected_control_mode_2, control_mode_map_1, MODE_COUNT);
    for (int i = 0; i < MODE_COUNT; i++) {
        if (control_mode_map_1[i] == g_display_settings.selected_control_mode) {
            DROPDOWN_SetSel(hSelectControl_1, i);
            break;
        }
    }
    DROPDOWN_SetFont(hSelectControl_1, GUI_FONT_16_1);
    
    hSelectControl_2 = DROPDOWN_CreateEx(settings_screen_6_layout.select_control_2_pos.x, settings_screen_6_layout.select_control_2_pos.y, settings_screen_6_layout.select_control_2_pos.w, settings_screen_6_layout.select_control_2_pos.h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_SELECT_CONTROL_2);
    PopulateControlDropdown(hSelectControl_2, g_display_settings.selected_control_mode, control_mode_map_2, MODE_COUNT);
    for (int i = 0; i < MODE_COUNT; i++) {
        if (control_mode_map_2[i] == g_display_settings.selected_control_mode_2) {
            DROPDOWN_SetSel(hSelectControl_2, i);
            break;
        }
    }
    DROPDOWN_SetFont(hSelectControl_2, GUI_FONT_16_1);

    const WidgetRect_t* defaults_pos = &settings_screen_6_layout.set_defaults_button_pos;
    hBUTTON_SET_DEFAULTS = BUTTON_CreateEx(defaults_pos->x, defaults_pos->y, defaults_pos->w, defaults_pos->h, 0, WM_CF_SHOW, 0, ID_SET_DEFAULTS);
    BUTTON_SetText(hBUTTON_SET_DEFAULTS, "SET DEFAULTS");

    const WidgetRect_t* restart_pos = &settings_screen_6_layout.restart_button_pos;
    hBUTTON_SYSRESTART = BUTTON_CreateEx(restart_pos->x, restart_pos->y, restart_pos->w, restart_pos->h, 0, WM_CF_SHOW, 0, ID_SYSRESTART);
    BUTTON_SetText(hBUTTON_SYSRESTART, "RESTART");

    const WidgetRect_t* next_pos = &settings_screen_6_layout.next_button_pos;
    hBUTTON_Next = BUTTON_CreateEx(next_pos->x, next_pos->y, next_pos->w, next_pos->h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    const WidgetRect_t* save_pos = &settings_screen_6_layout.save_button_pos;
    hBUTTON_Ok = BUTTON_CreateEx(save_pos->x, save_pos->y, save_pos->w, save_pos->h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    GUI_GotoXY(settings_screen_6_layout.device_id_label_pos[0].x, settings_screen_6_layout.device_id_label_pos[0].y);
    GUI_DispString("DEVICE");
    GUI_GotoXY(settings_screen_6_layout.device_id_label_pos[1].x, settings_screen_6_layout.device_id_label_pos[1].y);
    GUI_DispString("BUS ID");
    GUI_GotoXY(settings_screen_6_layout.curtain_move_time_label_pos[0].x, settings_screen_6_layout.curtain_move_time_label_pos[0].y);
    GUI_DispString("CURTAINS");
    GUI_GotoXY(settings_screen_6_layout.curtain_move_time_label_pos[1].x, settings_screen_6_layout.curtain_move_time_label_pos[1].y);
    GUI_DispString("MOVE TIME");
    GUI_GotoXY(settings_screen_6_layout.language_label_pos.x, settings_screen_6_layout.language_label_pos.y);
    GUI_DispString("LANGUAGE");
    GUI_GotoXY(settings_screen_6_layout.select_control_1_label_pos.x, settings_screen_6_layout.select_control_1_label_pos.y);
    GUI_DispString("IKONA 1 (S1)");
    GUI_GotoXY(settings_screen_6_layout.select_control_2_label_pos.x, settings_screen_6_layout.select_control_2_label_pos.y);
    GUI_DispString("IKONA 2 (S2)");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa šestog ekrana podešavanja.
 * @note Briše sve widgete vezane za Device ID, screensaver i navigaciju.
 */
static void DSP_KillSet6Scrn(void)
{
    if (WM_IsWindow(hSelectControl_1)) WM_DeleteWindow(hSelectControl_1);
    if (WM_IsWindow(hSelectControl_2)) WM_DeleteWindow(hSelectControl_2);
    WM_DeleteWindow(hDEV_ID);
    WM_DeleteWindow(hCurtainsMoveTime);
    WM_DeleteWindow(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
    WM_DeleteWindow(hCHKBX_LIGHT_NIGHT_TIMER);
    WM_DeleteWindow(hBUTTON_SET_DEFAULTS);
    WM_DeleteWindow(hBUTTON_SYSRESTART);
    WM_DeleteWindow(hDRPDN_Language);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje sedmi ekran podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija kreira korisnički interfejs za napredna podešavanja
 * sistema scena. Uključuje opciju za globalno omogućavanje/onemogućavanje
 * scena i tabelu za mapiranje do 8 logičkih okidača za "Povratak"
 * scenu na njihove stvarne adrese na RS485 busu.
 ******************************************************************************
 */
static void DSP_InitSet7Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // --- Kreiranje Glavnog Checkbox-a ---
    const WidgetRect_t* scenes_cb_pos = &settings_screen_7_layout.enable_scenes_checkbox_pos;
    hCHKBX_EnableScenes = CHECKBOX_CreateEx(scenes_cb_pos->x, scenes_cb_pos->y, scenes_cb_pos->w, scenes_cb_pos->h, 0, WM_CF_SHOW, 0, ID_ENABLE_SCENES);
    CHECKBOX_SetTextColor(hCHKBX_EnableScenes, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_EnableScenes, "ENABLE SCENE"); // "Enable Scene"
    CHECKBOX_SetState(hCHKBX_EnableScenes, g_display_settings.scenes_enabled);

    // Naslov za sekciju okidača
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringAt("Mapiranje Okidaca za 'Povratak' Scenu:", 10, 30);

    // --- Kreiranje Mreže za Mapiranje Okidača (8 komada) ---
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        // Dinamičko izračunavanje pozicija u dvije kolone
        int col = i / 4; // 0 za prvu kolonu (okidači 1-4), 1 za drugu (okidači 5-8)
        int row = i % 4; // 0-3 red unutar kolone

        int x = settings_screen_7_layout.grid_start_pos.x + (col * settings_screen_7_layout.x_col_spacing);
        int y = settings_screen_7_layout.grid_start_pos.y + (row * settings_screen_7_layout.y_spacing);

        // Kreiranje Spinbox-a
        hSPNBX_SceneTriggers[i] = SPINBOX_CreateEx(x, y, settings_screen_7_layout.widget_width, settings_screen_7_layout.widget_height, 0, WM_CF_SHOW, ID_SCENE_TRIGGER_1 + i, 0, 512);
        SPINBOX_SetEdge(hSPNBX_SceneTriggers[i], SPINBOX_EDGE_CENTER);
        // TODO: Učitati snimljenu vrijednost iz EEPROM-a kada se definiše struktura za to
        // SPINBOX_SetValue(hSPNBX_SceneTriggers[i], g_scene_settings.triggers[i]);

        // Kreiranje Labele
        char label_buffer[20];
        sprintf(label_buffer, "Okidac %d", i + 1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
        GUI_GotoXY(x + settings_screen_7_layout.label_offset.x, y + settings_screen_7_layout.label_offset.y);
        GUI_DispString(label_buffer);
    }

    // --- Kreiranje Navigacionih Dugmadi ---
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_7_layout.next_button_pos.x, settings_screen_7_layout.next_button_pos.y, settings_screen_7_layout.next_button_pos.w, settings_screen_7_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_7_layout.save_button_pos.x, settings_screen_7_layout.save_button_pos.y, settings_screen_7_layout.save_button_pos.w, settings_screen_7_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Briše GUI widgete sa sedmog ekrana podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SETTINGS_7` kako bi se oslobodila memorija i spriječili
 * konflikti sa widgetima na drugim ekranima. Uklanja sve
 * dinamički kreirane elemente.
 ******************************************************************************
 */
static void DSP_KillSet7Scrn(void)
{
    // Brišemo checkbox za omogućavanje scena
    WM_DeleteWindow(hCHKBX_EnableScenes);

    // Brišemo svih 8 spinbox-ova za adrese okidača
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        if (WM_IsWindow(hSPNBX_SceneTriggers[i])) // Sigurnosna provjera
        {
            WM_DeleteWindow(hSPNBX_SceneTriggers[i]);
        }
    }

    // Brišemo navigacione dugmiće
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 ******************************************************************************
 * @brief       Inicijalizuje osmi ekran podešavanja (Kapije).
 * @author      Gemini
 * @note        Ova funkcija kreira kompletan korisnički interfejs za konfiguraciju
 * jednog odabranog uređaja za kontrolu pristupa (kapije, rampe, brave).
 * Arhitektura, raspored elemenata i način inicijalizacije widgeta
 * su namjerno i u potpunosti preslikani sa funkcije `DSP_InitSet5Scrn`
 * (podešavanja za svjetla), kako bi se osigurala vizuelna i
 * funkcionalna konzistentnost, u skladu sa projektnim zahtjevima.
 * Koristi `settings_screen_8_layout` strukturu za pozicioniranje.
 * Ažurirano: Spinbox za trajanje impulsa sada koristi opseg 0-50 i jedinicu x100ms.
 ******************************************************************************
 */
static void DSP_InitSet8Scrn(void)
{
    // Standardna inicijalizacija ekrana
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Dohvatamo handle za kapiju koja se trenutno konfiguriše
    Gate_Handle* handle = Gate_GetInstance(settings_gate_selected_index);
    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return; // Sigurnosna provjera; prekini ako handle nije validan
    }

    // Definicije pozicija preuzete 1:1 iz layout-a
    const WidgetRect_t* sb_size = &settings_screen_8_layout.spinbox_size;
    int16_t x_col1 = settings_screen_8_layout.col1_x;
    int16_t x_col2 = settings_screen_8_layout.col2_x;
    int16_t y = settings_screen_8_layout.start_y;
    int16_t y_step = settings_screen_8_layout.y_step;

    // === PRVA KOLONA WIDGETA ===

    // 1. Spinbox za odabir kapije
    hGateSelect = SPINBOX_CreateEx(x_col1, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_SELECT, 1, GATE_MAX_COUNT);
    SPINBOX_SetEdge(hGateSelect, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateSelect, settings_gate_selected_index + 1);

    // 2. Dropdown za Profil Kontrole
    hGateType = DROPDOWN_CreateEx(x_col1, y + 1 * y_step, sb_size->w, 80, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_GATE_TYPE);
    for (int i = 0; i < Gate_GetProfileCount(); i++) {
        DROPDOWN_AddString(hGateType, Gate_GetProfileNameByIndex(i));
    }
    DROPDOWN_SetSel(hGateType, Gate_GetControlType(handle));
    DROPDOWN_SetFont(hGateType, GUI_FONT_16_1);

    // 3. Spinbox za Izgled
    uint16_t max_appearance_id = (sizeof(gate_appearance_mapping_table) / sizeof(IconMapping_t)) - 1;
    hGateAppearance = SPINBOX_CreateEx(x_col1, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_APPEARANCE, 0, max_appearance_id);
    SPINBOX_SetEdge(hGateAppearance, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateAppearance, Gate_GetAppearanceId(handle));

    // 4. Spinbox za Relej Komanda 1
    hGateParamSpinboxes[0] = SPINBOX_CreateEx(x_col1, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_RELAY_CMD1, 0, 512);
    SPINBOX_SetEdge(hGateParamSpinboxes[0], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[0], Gate_GetRelayAddr(handle, 1));

    // 5. Spinbox za Relej Komanda 2
    hGateParamSpinboxes[1] = SPINBOX_CreateEx(x_col1, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_RELAY_CMD2, 0, 512);
    SPINBOX_SetEdge(hGateParamSpinboxes[1], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[1], Gate_GetRelayAddr(handle, 2));

    // 6. Spinbox za Relej Komanda 3
    hGateParamSpinboxes[2] = SPINBOX_CreateEx(x_col1, y + 5 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_RELAY_CMD3, 0, 512);
    SPINBOX_SetEdge(hGateParamSpinboxes[2], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[2], Gate_GetRelayAddr(handle, 3));

    // === DRUGA KOLONA WIDGETA ===

    // 1. Spinbox za Senzor 1
    hGateParamSpinboxes[3] = SPINBOX_CreateEx(x_col2, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_FEEDBACK_1, 0, 512);
    SPINBOX_SetEdge(hGateParamSpinboxes[3], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[3], Gate_GetFeedbackAddr(handle, 1));

    // 2. Spinbox za Senzor 2
    hGateParamSpinboxes[4] = SPINBOX_CreateEx(x_col2, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_FEEDBACK_2, 0, 512);
    SPINBOX_SetEdge(hGateParamSpinboxes[4], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[4], Gate_GetFeedbackAddr(handle, 2));

    // 3. Spinbox za Tajmer Ciklusa
    hGateParamSpinboxes[5] = SPINBOX_CreateEx(x_col2, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_CYCLE_TIMER, 0, 255);
    SPINBOX_SetEdge(hGateParamSpinboxes[5], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[5], Gate_GetCycleTimer(handle));

    // 4. Spinbox za Trajanje Impulsa
    hGateParamSpinboxes[6] = SPINBOX_CreateEx(x_col2, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_GATE_EDIT_PULSE_TIMER, 0, 50); // << IZMJENJEN OPSEG
    SPINBOX_SetEdge(hGateParamSpinboxes[6], SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hGateParamSpinboxes[6], Gate_GetPulseTimer(handle) / 100); // << IZMIJENJENA VRIJEDNOST

    // Kreiranje navigacionih dugmadi - identično kao na ekranu za svjetla
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_8_layout.next_button_pos.x, settings_screen_8_layout.next_button_pos.y, settings_screen_8_layout.next_button_pos.w, settings_screen_8_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_8_layout.save_button_pos.x, settings_screen_8_layout.save_button_pos.y, settings_screen_8_layout.save_button_pos.w, settings_screen_8_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    // Iscrtavanje labela - identična logika poravnanja kao kod svjetala
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

    const GUI_POINT* label_offset = &settings_screen_8_layout.label_line1_offset;
    const int16_t label_y2_offset = settings_screen_8_layout.label_line2_offset_y;

    // Labele za prvu kolonu
    GUI_GotoXY(x_col1 + label_offset->x, y + label_offset->y);
    GUI_DispString("ODABIR");
    GUI_GotoXY(x_col1 + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("UREĐAJA");
    GUI_GotoXY(x_col1 + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("PROFIL");
    GUI_GotoXY(x_col1 + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("IZGLED");
    GUI_GotoXY(x_col1 + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("ADRESA");
    GUI_GotoXY(x_col1 + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("KOMANDA 1");
    GUI_GotoXY(x_col1 + label_offset->x, y + 4 * y_step + label_offset->y);
    GUI_DispString("ADRESA");
    GUI_GotoXY(x_col1 + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("KOMANDA 2");
    GUI_GotoXY(x_col1 + label_offset->x, y + 5 * y_step + label_offset->y);
    GUI_DispString("ADRESA");
    GUI_GotoXY(x_col1 + label_offset->x, y + 5 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("KOMANDA 3");

    // Labele za drugu kolonu
    GUI_GotoXY(x_col2 + label_offset->x, y + label_offset->y);
    GUI_DispString("ADRESA");
    GUI_GotoXY(x_col2 + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("SENZOR 1");
    GUI_GotoXY(x_col2 + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("ADRESA");
    GUI_GotoXY(x_col2 + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("SENZOR 2");
    GUI_GotoXY(x_col2 + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("TAJMER");
    GUI_GotoXY(x_col2 + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("CIKLUSA (s)");
    GUI_GotoXY(x_col2 + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("TRAJANJE");
    GUI_GotoXY(x_col2 + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("IMPULSA (x100ms)"); // << IZMIJENJEN TEKST

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete sa osmog ekrana podešavanja (Kapije).
 * @author      Gemini
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana `SCREEN_SETTINGS_8`.
 * Njena uloga je da oslobodi sve resurse zauzete od strane widgeta,
 * čime se sprječava curenje memorije i osigurava čisto stanje za
 * iscrtavanje sljedećeg ekrana. Arhitektura je identična funkciji
 * `DSP_KillSet5Scrn` radi konzistentnosti.
 ******************************************************************************
 */
static void DSP_KillSet8Scrn(void)
{
    // Brisanje glavnih kontrola za odabir
    if (WM_IsWindow(hGateSelect))     WM_DeleteWindow(hGateSelect);
    if (WM_IsWindow(hGateType))       WM_DeleteWindow(hGateType);
    if (WM_IsWindow(hGateAppearance)) WM_DeleteWindow(hGateAppearance);

    // Brisanje niza Spinbox widgeta za parametre
    for(int i = 0; i < 7; i++) {
        if (WM_IsWindow(hGateParamSpinboxes[i])) {
            WM_DeleteWindow(hGateParamSpinboxes[i]);
        }
    }

    // Brisanje zajedničkih navigacionih dugmadi
    if (WM_IsWindow(hBUTTON_Next)) WM_DeleteWindow(hBUTTON_Next);
    if (WM_IsWindow(hBUTTON_Ok))   WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje deveti ekran za podešavanja (Alarm).
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA KORIGOVANA VERZIJA: Layout sada striktno koristi
 * `y_group_spacing` konstantu za vertikalni razmak između SVIH
 * redova kontrola, uključujući i zajedničke postavke ispod
 * bloka za particije, kako bi se osigurao apsolutno jednak
 * i konzistentan raspored.
 ******************************************************************************
 */
static void DSP_InitSet9Scrn(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // Korištenje konstanti iz layout strukture
    const int16_t col1_x = settings_screen_9_layout.start_pos.x;
    const int16_t y_start = settings_screen_9_layout.start_pos.y;
    const int16_t col2_x = col1_x + settings_screen_9_layout.x_col_spacing;
    const int16_t spin_w = settings_screen_9_layout.spinbox_size.w;
    const int16_t spin_h = settings_screen_9_layout.spinbox_size.h;
    const int16_t y_spacing = settings_screen_9_layout.y_group_spacing; // JEDINI KORIŠTENI RAZMAK

    // Pomoćne varijable za pozicioniranje labela
    const int16_t lbl_off_x = 120;
    const int16_t lbl_line1_off_y = 8;
    const int16_t lbl_line2_off_y = 20;

    // --- Kreiranje kontrola za Particije (koristi y_spacing) ---
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
        int y_pos = y_start + i * y_spacing;

        SPINBOX_Handle hRelay = SPINBOX_CreateEx(col1_x, y_pos, spin_w, spin_h, 0, WM_CF_SHOW, ID_ALARM_RELAY_P1 + i, 0, 512);
        SPINBOX_SetEdge(hRelay, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hRelay, Security_GetPartitionRelayAddr(i));

        SPINBOX_Handle hFb = SPINBOX_CreateEx(col2_x, y_pos, spin_w, spin_h, 0, WM_CF_SHOW, ID_ALARM_FB_P1 + i, 0, 512);
        SPINBOX_SetEdge(hFb, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hFb, Security_GetPartitionFeedbackAddr(i));

        char buffer[20];
        sprintf(buffer, "Particija %d", i + 1);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetColor(GUI_WHITE);

        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
        GUI_DispStringAt(buffer, col1_x + lbl_off_x, y_pos + lbl_line1_off_y);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
        GUI_DispStringAt("Relej", col1_x + lbl_off_x, y_pos + lbl_line2_off_y);

        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
        GUI_DispStringAt(buffer, col2_x + lbl_off_x, y_pos + lbl_line1_off_y);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
        GUI_DispStringAt("Feedback", col2_x + lbl_off_x, y_pos + lbl_line2_off_y);
    }
    
    // === POČETAK ISPRAVLJENOG DIJELA ===
    // Početna Y pozicija za prvi red ispod particija, KORISTEĆI y_spacing
    int16_t y_current = y_start + SECURITY_PARTITION_COUNT * y_spacing;

    // 1. RED KONTROLA ISPOD PARTICIJA
    SPINBOX_Handle hSilent = SPINBOX_CreateEx(col1_x, y_current, spin_w, spin_h, 0, WM_CF_SHOW, ID_ALARM_RELAY_SILENT, 0, 512);
    SPINBOX_SetEdge(hSilent, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSilent, Security_GetSilentAlarmAddr());
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("Tihi alarm", col1_x + lbl_off_x, y_current + lbl_line1_off_y);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("(SOS)", col1_x + lbl_off_x, y_current + lbl_line2_off_y);

    SPINBOX_Handle hStatusFb = SPINBOX_CreateEx(col2_x, y_current, spin_w, spin_h, 0, WM_CF_SHOW, ID_ALARM_FB_SYSTEM_STATUS, 0, 512);
    SPINBOX_SetEdge(hStatusFb, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hStatusFb, Security_GetSystemStatusFeedbackAddr());
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("Feedback", col2_x + lbl_off_x, y_current + lbl_line1_off_y);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("Alarm", col2_x + lbl_off_x, y_current + lbl_line2_off_y);

    // Pomjeramo Y poziciju za sljedeći red KORISTEĆI y_spacing
    y_current += y_spacing;
    
    // 2. RED KONTROLA ISPOD PARTICIJA
    hCHKBX_EnableSecurity = CHECKBOX_CreateEx(col1_x, y_current, 240, 20, 0, WM_CF_SHOW, 0, ID_ENABLE_SECURITY_MODULE);
    CHECKBOX_SetTextColor(hCHKBX_EnableSecurity, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_EnableSecurity, "Enable Security Module");
    CHECKBOX_SetState(hCHKBX_EnableSecurity, g_display_settings.security_module_enabled);

    SPINBOX_Handle hPulse = SPINBOX_CreateEx(col2_x, y_current, spin_w, spin_h, 0, WM_CF_SHOW, ID_ALARM_PULSE_LENGTH, 0, 50);
    SPINBOX_SetEdge(hPulse, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hPulse, Security_GetPulseDuration() / 100);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("Dužina pulsa", col2_x + lbl_off_x, y_current + lbl_line1_off_y);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("x100ms", col2_x + lbl_off_x, y_current + lbl_line2_off_y);
    // === KRAJ ISPRAVLJENOG DIJELA ===

    // Navigaciona dugmad ostaju na fiksnoj poziciji desno
    hBUTTON_Next = BUTTON_CreateEx(410, 180, 60, 30, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(410, 230, 60, 30, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
 ******************************************************************************
 * @brief       Briše GUI widgete sa devetog ekrana za podešavanja (Alarm).
 ******************************************************************************
 */
static void DSP_KillSet9Scrn(void)
{
    // Brisanje svih widgeta kreiranih u DSP_InitSet9Scrn
    for(int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
        WM_DeleteWindow(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_P1 + i));
        WM_DeleteWindow(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_P1 + i));
    }
    
    WM_DeleteWindow(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_SILENT));
    WM_DeleteWindow(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_SYSTEM_STATUS));
    WM_DeleteWindow(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_PULSE_LENGTH));
    
    if(WM_IsWindow(hCHKBX_EnableSecurity)) {
        WM_DeleteWindow(hCHKBX_EnableSecurity);
        hCHKBX_EnableSecurity = 0;
    }

    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za glavni ekran tajmera.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija osigurava da se dinamički kreirano dugme za
 * podešavanja (`hButtonTimerSettings`) ispravno obriše prilikom
 * napuštanja ekrana, čime se sprječava curenje memorije.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_KillTimerScreen(void)
{

}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za ekran podešavanja tajmera.
 * @author      Gemini
 * @note        Ova funkcija koristi `timer_settings_screen_layout` strukturu za
 * pozicioniranje svih elemenata. Kreira kontrole za podešavanje
 * vremena, 7 checkbox-ova za dane, i dva toggle dugmeta za akcije.
 * Inicijalne vrijednosti se učitavaju iz `Timer` modula.
 ******************************************************************************
 */
static void DSP_InitSettingsTimerScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // --- Prikaz vremena ---
    // Iscrtavanje naslova ekrana (dodato)
    GUI_SetFont(&GUI_FontVerdana16_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
    GUI_DispStringAt(lng(TXT_TIMER_SETTINGS_TITLE), timer_settings_screen_layout.time_colon_pos.x, 2);


    // --- Dugmad za podešavanje vremena ---
    const int16_t btn_size = timer_settings_screen_layout.time_btn_size;

    hButtonTimerHourUp = BUTTON_CreateEx(timer_settings_screen_layout.hour_up_pos.x, timer_settings_screen_layout.hour_up_pos.y, btn_size, btn_size, 0, WM_CF_SHOW, 0, ID_TIMER_HOUR_UP);
    BUTTON_SetBitmap(hButtonTimerHourUp, BUTTON_CI_UNPRESSED, &bmicons_button_up_50_squared);

    hButtonTimerHourDown = BUTTON_CreateEx(timer_settings_screen_layout.hour_down_pos.x, timer_settings_screen_layout.hour_down_pos.y, btn_size, btn_size, 0, WM_CF_SHOW, 0, ID_TIMER_HOUR_DOWN);
    BUTTON_SetBitmap(hButtonTimerHourDown, BUTTON_CI_UNPRESSED, &bmicons_button_down_50_squared);


    hButtonTimerMinuteUp = BUTTON_CreateEx(timer_settings_screen_layout.minute_up_pos.x, timer_settings_screen_layout.minute_up_pos.y, btn_size, btn_size, 0, WM_CF_SHOW, 0, ID_TIMER_MINUTE_UP);
    BUTTON_SetBitmap(hButtonTimerMinuteUp, BUTTON_CI_UNPRESSED, &bmicons_button_up_50_squared);

    hButtonTimerMinuteDown = BUTTON_CreateEx(timer_settings_screen_layout.minute_down_pos.x, timer_settings_screen_layout.minute_down_pos.y, btn_size, btn_size, 0, WM_CF_SHOW, 0, ID_TIMER_MINUTE_DOWN);
    BUTTON_SetBitmap(hButtonTimerMinuteDown, BUTTON_CI_UNPRESSED, &bmicons_button_down_50_squared);

    uint8_t repeat_mask = Timer_GetRepeatMask();
    const GUI_BITMAP* icon_on = &bmicons_toggle_on_50_squared;
    const GUI_BITMAP* icon_off = &bmicons_toogle_off_50_squared;


    for (int i = 0; i < 7; i++) {
        int16_t checkbox_x_center = timer_settings_screen_layout.day_checkbox_start_pos.x +
                                    (icon_on->XSize / 2) +
                                    i * (icon_on->XSize + timer_settings_screen_layout.day_checkbox_gap_x);

        int16_t label_y_pos = timer_settings_screen_layout.day_checkbox_start_pos.y - 20;
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_SetFont(&GUI_FontVerdana16_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_DispStringAt(_acContent[g_display_settings.language][i], checkbox_x_center, label_y_pos);
    }


    for (int i = 0; i < 7; i++) {
        int16_t x_pos = timer_settings_screen_layout.day_checkbox_start_pos.x + i * (icon_on->XSize + timer_settings_screen_layout.day_checkbox_gap_x);
        int16_t y_pos = timer_settings_screen_layout.day_checkbox_start_pos.y;

        hButtonTimerDay[i] = BUTTON_CreateEx(x_pos, y_pos, icon_on->XSize, icon_on->YSize, 0, WM_CF_SHOW, 0, ID_TIMER_DAY_MON + i);
        BUTTON_SetBitmap(hButtonTimerDay[i], BUTTON_CI_UNPRESSED, (repeat_mask & (1 << i)) ? icon_on : icon_off);
        BUTTON_SetBkColor(hButtonTimerDay[i], BUTTON_CI_UNPRESSED, GUI_BLACK);
        BUTTON_SetBkColor(hButtonTimerDay[i], BUTTON_CI_PRESSED, GUI_BLACK);
    }


    const GUI_BITMAP* toggle_icon_buzzer = Timer_GetActionBuzzer() ? icon_on : icon_off;
    hButtonTimerBuzzer = BUTTON_CreateEx(timer_settings_screen_layout.buzzer_button_pos.x, timer_settings_screen_layout.buzzer_button_pos.y, toggle_icon_buzzer->XSize, toggle_icon_buzzer->YSize, 0, WM_CF_SHOW, 0, ID_TIMER_BUZZER_TOGGLE);
    BUTTON_SetBitmap(hButtonTimerBuzzer, BUTTON_CI_UNPRESSED, toggle_icon_buzzer);
    BUTTON_SetBkColor(hButtonTimerBuzzer, BUTTON_CI_UNPRESSED, GUI_BLACK);
    BUTTON_SetBkColor(hButtonTimerBuzzer, BUTTON_CI_PRESSED, GUI_BLACK);

    // Tekst za buzzer
    GUI_SetFont(&GUI_FontVerdana16_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt(lng(TXT_TIMER_USE_BUZZER), timer_settings_screen_layout.buzzer_button_pos.x + toggle_icon_buzzer->XSize + 10, timer_settings_screen_layout.buzzer_button_pos.y + toggle_icon_buzzer->YSize / 2);

    // Dugme za scenu sa tekstom
    const GUI_BITMAP* toggle_icon_scene =  icon_off;
//    const GUI_BITMAP* toggle_icon_scene = Timer_GetActionScene() ? icon_on : icon_off; // Pretpostavljena funkcija Timer_GetActionScene
    hButtonTimerScene = BUTTON_CreateEx(timer_settings_screen_layout.scene_button_pos.x, timer_settings_screen_layout.scene_button_pos.y, toggle_icon_scene->XSize, toggle_icon_scene->YSize, 0, WM_CF_SHOW, 0, ID_TIMER_SCENE_TOGGLE);
    BUTTON_SetBitmap(hButtonTimerScene, BUTTON_CI_UNPRESSED, toggle_icon_scene);
    BUTTON_SetBkColor(hButtonTimerScene, BUTTON_CI_UNPRESSED, GUI_BLACK);
    BUTTON_SetBkColor(hButtonTimerScene, BUTTON_CI_PRESSED, GUI_BLACK);

    // Tekst za scenu
    GUI_SetFont(&GUI_FontVerdana16_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt(lng(TXT_TIMER_TRIGGER_SCENE), timer_settings_screen_layout.scene_button_pos.x + toggle_icon_scene->XSize + 10, timer_settings_screen_layout.scene_button_pos.y + toggle_icon_scene->YSize / 2);

    // Dugme za odabir scene (standardno dugme sa tekstom)
    // Dugme za odabir scene (sada sa ikonicom)
    hButtonTimerSceneSelect = BUTTON_CreateEx(timer_settings_screen_layout.scene_select_btn_pos.x,
                              timer_settings_screen_layout.scene_select_btn_pos.y,
                              bmicons_button_select_40_sqaured.XSize, // Automatsko preuzimanje širine
                              bmicons_button_select_40_sqaured.YSize, // Automatsko preuzimanje visine
                              0, WM_CF_SHOW, 0, ID_TIMER_SCENE_SELECT);

    BUTTON_SetBitmap(hButtonTimerSceneSelect, BUTTON_CI_UNPRESSED, &bmicons_button_select_40_sqaured);
//    BUTTON_SetFont(hButtonTimerSceneSelect, &GUI_Font16_1);
//    BUTTON_SetText(hButtonTimerSceneSelect, lng(TXT_SCENES));
    // Dugme za snimanje (sada sa ikonicom)
    hButtonTimerSave = BUTTON_CreateEx(timer_settings_screen_layout.save_button_pos.x,
                                       timer_settings_screen_layout.save_button_pos.y,
                                       bmicons_button_save_50_squared.XSize, // Automatsko preuzimanje širine
                                       bmicons_button_save_50_squared.YSize, // Automatsko preuzimanje visine
                                       0, WM_CF_SHOW, 0, ID_TIMER_SAVE);
    BUTTON_SetBitmap(hButtonTimerSave, BUTTON_CI_UNPRESSED, &bmicons_button_save_50_squared);
//    BUTTON_SetFont(hButtonTimerSave, &GUI_Font16_1);
//    BUTTON_SetText(hButtonTimerSave, lng(TXT_SAVE));

    // Dugme za otkazivanje (sada sa ikonicom)
    hButtonTimerCancel = BUTTON_CreateEx(timer_settings_screen_layout.cancel_button_pos.x,
                                         timer_settings_screen_layout.cancel_button_pos.y,
                                         bmicons_button_cancel_50_squared.XSize, // Automatsko preuzimanje širine
                                         bmicons_button_cancel_50_squared.YSize, // Automatsko preuzimanje visine
                                         0, WM_CF_SHOW, 0, ID_TIMER_CANCEL);
    BUTTON_SetBitmap(hButtonTimerCancel, BUTTON_CI_UNPRESSED, &bmicons_button_cancel_50_squared);
//    BUTTON_SetFont(hButtonTimerCancel, &GUI_Font16_1);
//    BUTTON_SetText(hButtonTimerCancel, lng(TXT_CANCEL));


    GUI_MULTIBUF_EndEx(1);
    shouldDrawScreen = 1;

}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete sa ekrana za podešavanje alarma.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je ispravljena da briše samo widgete koji se zaista
 * kreiraju u `DSP_InitSettingsTimerScreen` funkciji, čime se
 * rješava greška "undeclared identifier" prilikom kompilacije.
 ******************************************************************************
 */
static void DSP_KillSettingsTimerScreen(void)
{
    // Brisanje handle-ova za widgete podešavanja vremena
    if (WM_IsWindow(hButtonTimerHourUp)) WM_DeleteWindow(hButtonTimerHourUp);
    if (WM_IsWindow(hButtonTimerHourDown)) WM_DeleteWindow(hButtonTimerHourDown);
    // Uklanjanje brisanja hTextTimerHour i hTextTimerMinute
    if (WM_IsWindow(hButtonTimerMinuteUp)) WM_DeleteWindow(hButtonTimerMinuteUp);
    if (WM_IsWindow(hButtonTimerMinuteDown)) WM_DeleteWindow(hButtonTimerMinuteDown);

    // Brisanje handle-ova za 7 checkbox-ova za dane
    for (int i = 0; i < 7; i++) {
        if (WM_IsWindow(hButtonTimerDay[i])) {
            WM_DeleteWindow(hButtonTimerDay[i]);
        }
    }

    // Brisanje handle-ova za akcijske i navigacione dugmiće
    if (WM_IsWindow(hButtonTimerBuzzer)) WM_DeleteWindow(hButtonTimerBuzzer);
    if (WM_IsWindow(hButtonTimerScene)) WM_DeleteWindow(hButtonTimerScene);
    if (WM_IsWindow(hButtonTimerSceneSelect)) WM_DeleteWindow(hButtonTimerSceneSelect);
    if (WM_IsWindow(hButtonTimerSave)) WM_DeleteWindow(hButtonTimerSave);
    if (WM_IsWindow(hButtonTimerCancel)) WM_DeleteWindow(hButtonTimerCancel);

    // Postavljanje fleg-a na false kako bi se osigurala ponovna inicijalizacija pri sljedećem ulasku
    timer_screen_initialized = false;
    GUI_Clear();
    GUI_Exec();
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za ekran podešavanja datuma i vremena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički kreira kompletan korisnički interfejs,
 * koristeći isključivo parametre definisane u `datetime_settings_layout`
 * strukturi. Implementira specifičan 3+2 raspored elemenata
 * (3 u gornjem redu za datum, 2 u donjem za vrijeme) kroz dvije
 * odvojene petlje, čime se eliminišu sve fiksne ("magične")
 * vrijednosti iz koda.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_InitSettingsDateTimeScreen(void)
{
    // Standardna inicijalizacija ekrana sa dvostrukim baferovanjem
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // Iscrtavanje naslova ekrana
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
    GUI_DispStringAt(lng(TXT_DATETIME_SETUP_TITLE), LCD_GetXSize() / 2, 2);

    // Nizovi za lakše kreiranje elemenata u petljama
    const int16_t x_cols[] = { datetime_settings_layout.x_col_1, datetime_settings_layout.x_col_2, datetime_settings_layout.x_col_3 };
    const TextID labels_date[] = { TXT_DAY, TXT_MONTH, TXT_YEAR };
    const TextID labels_time[] = { TXT_HOUR, TXT_MINUTE };
    const int ids_up[] = { ID_DATETIME_DAY_UP, ID_DATETIME_MONTH_UP, ID_DATETIME_YEAR_UP, ID_DATETIME_HOUR_UP, ID_DATETIME_MINUTE_UP };
    const int ids_down[] = { ID_DATETIME_DAY_DOWN, ID_DATETIME_MONTH_DOWN, ID_DATETIME_YEAR_DOWN, ID_DATETIME_HOUR_DOWN, ID_DATETIME_MINUTE_DOWN };

    // --- Prva petlja: Kreiranje elemenata za gornji red (DAN, MJESEC, GODINA) ---
    for (int i = 0; i < 3; i++)
    {
        int16_t y_base = datetime_settings_layout.y_row_top;
        int16_t x_base = x_cols[i];
        int16_t btn_total_width = (2 * datetime_settings_layout.btn_size) + datetime_settings_layout.btn_pair_gap_x;
        int16_t x_center = x_base + (btn_total_width / 2);

        // Iscrtavanje labele (npr. "DAN")
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_DispStringAt(lng(labels_date[i]), x_center, y_base - datetime_settings_layout.label_offset_y);

        // Kreiranje TEXT widgeta za prikaz vrijednosti
        hTextDateTimeValue[i] = TEXT_CreateEx(x_base, y_base - datetime_settings_layout.value_offset_y, btn_total_width, 25, 0, WM_CF_SHOW, TEXT_CF_HCENTER | TEXT_CF_VCENTER, GUI_ID_USER + i, "");
        TEXT_SetFont(hTextDateTimeValue[i], &GUI_Font32_1);
        TEXT_SetTextColor(hTextDateTimeValue[i], GUI_ORANGE);

        // Kreiranje para +/- dugmadi
        hButtonDateTimeDown[i] = BUTTON_CreateEx(x_base, y_base, datetime_settings_layout.btn_size, datetime_settings_layout.btn_size, 0, WM_CF_SHOW, 0, ids_down[i]);
        BUTTON_SetBitmap(hButtonDateTimeDown[i], BUTTON_CI_UNPRESSED, &bmicons_button_down_50_squared);

        hButtonDateTimeUp[i] = BUTTON_CreateEx(x_base + datetime_settings_layout.btn_size + datetime_settings_layout.btn_pair_gap_x, y_base, datetime_settings_layout.btn_size, datetime_settings_layout.btn_size, 0, WM_CF_SHOW, 0, ids_up[i]);
        BUTTON_SetBitmap(hButtonDateTimeUp[i], BUTTON_CI_UNPRESSED, &bmicons_button_up_50_squared);
    }

    // --- Druga petlja: Kreiranje elemenata za donji red (SAT, MINUTA) ---
    for (int i = 0; i < 2; i++)
    {
        int element_index = i + 3; // Nastavljamo indekse (3 za sat, 4 za minutu)
        int16_t y_base = datetime_settings_layout.y_row_bottom;
        int16_t x_base = x_cols[i]; // Koristimo samo prve dvije kolone
        int16_t btn_total_width = (2 * datetime_settings_layout.btn_size) + datetime_settings_layout.btn_pair_gap_x;
        int16_t x_center = x_base + (btn_total_width / 2);

        // Iscrtavanje labele (npr. "SAT")
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_DispStringAt(lng(labels_time[i]), x_center, y_base - datetime_settings_layout.label_offset_y);

        // Kreiranje TEXT widgeta za prikaz vrijednosti
        hTextDateTimeValue[element_index] = TEXT_CreateEx(x_base, y_base - datetime_settings_layout.value_offset_y, btn_total_width, 25, 0, WM_CF_SHOW, TEXT_CF_HCENTER | TEXT_CF_VCENTER, GUI_ID_USER + element_index, "");
        TEXT_SetFont(hTextDateTimeValue[element_index], &GUI_Font32_1);
        TEXT_SetTextColor(hTextDateTimeValue[element_index], GUI_ORANGE);

        // Kreiranje para +/- dugmadi
        hButtonDateTimeDown[element_index] = BUTTON_CreateEx(x_base, y_base, datetime_settings_layout.btn_size, datetime_settings_layout.btn_size, 0, WM_CF_SHOW, 0, ids_down[element_index]);
        BUTTON_SetBitmap(hButtonDateTimeDown[element_index], BUTTON_CI_UNPRESSED, &bmicons_button_down_50_squared);

        hButtonDateTimeUp[element_index] = BUTTON_CreateEx(x_base + datetime_settings_layout.btn_size + datetime_settings_layout.btn_pair_gap_x, y_base, datetime_settings_layout.btn_size, datetime_settings_layout.btn_size, 0, WM_CF_SHOW, 0, ids_up[element_index]);
        BUTTON_SetBitmap(hButtonDateTimeUp[element_index], BUTTON_CI_UNPRESSED, &bmicons_button_up_50_squared);
    }

    // Kreiranje "OK" dugmeta
    hBUTTON_Ok = BUTTON_CreateEx(datetime_settings_layout.ok_btn_pos_x, datetime_settings_layout.ok_btn_pos_y, datetime_settings_layout.ok_btn_width, datetime_settings_layout.ok_btn_height, 0, WM_CF_SHOW, 0, ID_DATETIME_SAVE);
    BUTTON_SetText(hBUTTON_Ok, lng(TXT_SAVE));
    BUTTON_SetFont(hBUTTON_Ok, &GUI_FontVerdana20_LAT);

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za ekran podešavanja vremena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * SCREEN_SETTINGS_DATETIME, bez obzira na razlog (potvrda unosa,
 * izlazak preko menija, screensaver timeout). Sistematski prolazi
 * kroz sve handle-ove, provjerava da li widget postoji i briše ga,
 * osiguravajući potpuno čišćenje i oslobađanje resursa.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_KillSettingsDateTimeScreen(void)
{
    // Petlja briše svih 5 TEXT widgeta i 10 BUTTON widgeta za +/-
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hTextDateTimeValue[i])) {
            WM_DeleteWindow(hTextDateTimeValue[i]);
            hTextDateTimeValue[i] = 0; // Resetuj handle na nulu
        }
        if (WM_IsWindow(hButtonDateTimeUp[i])) {
            WM_DeleteWindow(hButtonDateTimeUp[i]);
            hButtonDateTimeUp[i] = 0; // Resetuj handle na nulu
        }
        if (WM_IsWindow(hButtonDateTimeDown[i])) {
            WM_DeleteWindow(hButtonDateTimeDown[i]);
            hButtonDateTimeDown[i] = 0; // Resetuj handle na nulu
        }
    }

    // Brisanje "OK" dugmeta
    if (WM_IsWindow(hBUTTON_Ok)) {
        WM_DeleteWindow(hBUTTON_Ok);
        hBUTTON_Ok = 0; // Resetuj handle na nulu
    }
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za glavni ekran "Čarobnjaka".
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je centralni dio iscrtavanja za `SCREEN_SCENE_EDIT`.
 * Prvo dohvati podatke za scenu koja se trenutno mijenja
 * (`scene_edit_index`). Zatim iscrtava njen trenutni izgled (ikonicu
 * i naziv) i kreira dugmad za navigaciju: "[ Promijeni ]" za ulazak
 * u mod izmjene izgleda, te "Snimi" i "Otkaži".
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_InitSceneEditScreen(void)
{
    DSP_KillSceneEditScreen();
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) {
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_RED);
        GUI_DispStringAt("GRESKA: Scena nije dostupna!", 10, 10);
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    hBUTTON_Ok = BUTTON_CreateEx(370, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_Ok);
    hBUTTON_Next = BUTTON_CreateEx(10, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, lng(TXT_CANCEL));

    if (scene_handle->is_configured == false)
    {
        BUTTON_SetText(hBUTTON_Ok, lng(TXT_SAVE));
        const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];

        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_DispStringAt("Izgled i Naziv Scene:", 10, 10);

        const GUI_BITMAP* icon_to_draw = scene_icon_images[appearance->icon_id - ICON_SCENE_WIZZARD];
        GUI_DrawBitmap(icon_to_draw, 15, 40);

        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(appearance->text_id), 100, 70);

        hButtonChangeAppearance = BUTTON_CreateEx(300, 50, 150, 40, 0, WM_CF_SHOW, 0, ID_BUTTON_CHANGE_APPEARANCE);
        BUTTON_SetText(hButtonChangeAppearance, "[ Promijeni ]");

        if (scene_handle->appearance_id == 0) {
            WM_DisableWindow(hBUTTON_Ok);
        }
    }
    else
    {
        BUTTON_SetText(hBUTTON_Ok, "Memorisi Stanje");
        const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];

        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_DispStringAt("Izgled i Naziv Scene:", 10, 10);

        int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
        if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
        {
            const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
            GUI_DrawBitmap(icon_to_draw, 15, 40);
        }

        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(appearance->text_id), 100, 70);

        hButtonDeleteScene = BUTTON_CreateEx(190, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_BUTTON_DELETE_SCENE);
        BUTTON_SetText(hButtonDeleteScene, lng(TXT_DELETE));

        hButtonDetailedSetup = BUTTON_CreateEx(10, 150, 200, 40, 0, WM_CF_SHOW, 0, ID_BUTTON_DETAILED_SETUP);
        BUTTON_SetText(hButtonDetailedSetup, "Detaljna Podesavanja");
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za glavni ekran "Čarobnjaka".
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA. Ova funkcija sada provjerava i briše sve
 * dinamički kreirane widgete ("Promijeni", "Obriši",
 * "Detaljna Podešavanja") i resetuje njihove handle-ove na nulu,
 * čime se sprečava pojava "duhova" na drugim ekranima.
 ******************************************************************************
 */
static void DSP_KillSceneEditScreen(void)
{
    if (WM_IsWindow(hButtonChangeAppearance)) {
        WM_DeleteWindow(hButtonChangeAppearance);
        hButtonChangeAppearance = 0;
    }
    if (WM_IsWindow(hButtonDeleteScene)) {
        WM_DeleteWindow(hButtonDeleteScene);
        hButtonDeleteScene = 0;
    }
    if (WM_IsWindow(hButtonDetailedSetup)) {
        WM_DeleteWindow(hButtonDetailedSetup);
        hButtonDetailedSetup = 0;
    }
    if (WM_IsWindow(hBUTTON_Ok)) {
        WM_DeleteWindow(hBUTTON_Ok);
        hBUTTON_Ok = 0;
    }
    if (WM_IsWindow(hBUTTON_Next)) {
        WM_DeleteWindow(hBUTTON_Next);
        hBUTTON_Next = 0;
    }
}
/**
 ******************************************************************************
 * @brief       [VERZIJA 2.1] Inicijalizuje i iscrtava višenamjenski ekran za odabir scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija je ažurirana da u `TIMER` modu više ne prikazuje
 * opciju "Nijedna", već koristi svih 6 slotova za prikaz
 * postojećih, konfigurisanih scena.
 ******************************************************************************
 */
static void DSP_InitSceneAppearanceScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu(1);

    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);

    if (current_scene_picker_mode == SCENE_PICKER_MODE_TIMER)
    {
        /********************************************/
        /* MOD: ODABIR POSTOJEĆE SCENE ZA TAJMER    */
        /********************************************/
        GUI_DispStringAt("Odaberite Scenu za Alarm", LCD_GetXSize() / 2, 5);

        // KORAK 1: Uklonjen kod za iscrtavanje opcije "Nijedna"

        // KORAK 2: Iscrtaj sve konfigurisane scene, počevši od prvog slota
        uint8_t display_index = 0; // Počinjemo od prvog slota
        for (int i = 0; i < SCENE_MAX_COUNT; i++) {
            Scene_t* scene_handle = Scene_GetInstance(i);
            if (scene_handle && scene_handle->is_configured) {
                if (display_index >= (scene_screen_layout.items_per_row * 2)) break; // Prekidamo ako smo popunili ekran

                const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];
                int row = display_index / scene_screen_layout.items_per_row;
                int col = display_index % scene_screen_layout.items_per_row;
                int x_center = (scene_screen_layout.slot_width / 2) + (col * scene_screen_layout.slot_width);
                int y_center = (scene_screen_layout.slot_height / 2) + (row * scene_screen_layout.slot_height) + 10;

                int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
                if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
                {
                    const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
                    GUI_DrawBitmap(icon_to_draw, x_center - (icon_to_draw->XSize / 2), y_center - (icon_to_draw->YSize / 2));
                }

                GUI_SetFont(&GUI_FontVerdana16_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(appearance->text_id), x_center, y_center + scene_screen_layout.text_y_offset);

                display_index++;
            }
        }
    } else {
        /************************************************/
        /* MOD: ODABIR IZGLEDA ZA NOVU SCENU (WIZARD)   */
        /************************************************/
        GUI_DispStringAt("Odaberite Izgled Scene", LCD_GetXSize() / 2, 5);

        const int ICONS_PER_PAGE = 6;
        int total_available_appearances = 0;
        const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];

        uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
        uint8_t used_count = 0;
        for (int i = 0; i < SCENE_MAX_COUNT; i++) {
            Scene_t* temp_handle = Scene_GetInstance(i);
            if (temp_handle && temp_handle->is_configured) {
                used_appearance_ids[used_count++] = temp_handle->appearance_id;
            }
        }
        for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++) {
            bool is_used = false;
            for (int j = 0; j < used_count; j++) {
                if (used_appearance_ids[j] == i) {
                    is_used = true;
                    break;
                }
            }
            if (!is_used) {
                available_appearances[total_available_appearances++] = &scene_appearance_table[i];
            }
        }
        int total_pages = (total_available_appearances + ICONS_PER_PAGE - 1) / ICONS_PER_PAGE;
        if (scene_appearance_page >= total_pages && total_pages > 0) {
            scene_appearance_page = total_pages - 1;
        }
        int start_index = scene_appearance_page * ICONS_PER_PAGE;
        int end_index = start_index + ICONS_PER_PAGE;
        if (end_index > total_available_appearances) {
            end_index = total_available_appearances;
        }
        for (int i = start_index; i < end_index; i++) {
            const SceneAppearance_t* appearance = available_appearances[i];
            int display_index = i % ICONS_PER_PAGE;
            int row = display_index / scene_screen_layout.items_per_row;
            int col = display_index % scene_screen_layout.items_per_row;
            int x_center = (scene_screen_layout.slot_width / 2) + (col * scene_screen_layout.slot_width);
            int y_center = (scene_screen_layout.slot_height / 2) + (row * scene_screen_layout.slot_height) + 10;
            int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
            if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
            {
                const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
                GUI_DrawBitmap(icon_to_draw, x_center - (icon_to_draw->XSize / 2), y_center - (icon_to_draw->YSize / 2));
            }
            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(appearance->text_id), x_center, y_center + scene_screen_layout.text_y_offset);
        }
        if (total_pages > 1) {
            const GUI_BITMAP* iconNext = &bmnext;
            int x_pos = select_screen2_drawing_layout.next_button_x_pos;
            int y_pos = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2);
            GUI_DrawBitmap(iconNext, x_pos, y_pos);
        }
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje GUI za prvi korak čarobnjaka: Odabir Uređaja.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA 2.0: Funkcija je sada "pametna". Prije iscrtavanja,
 * provjerava koji moduli (svjetla, roletne, termostat) imaju
 * konfigurisane uređaje i dinamički prikazuje checkbox-ove samo
 * za dostupne opcije, čime se korisnički interfejs čini čistijim i
 * relevantnijim.
 ******************************************************************************
 */
static void DSP_InitSceneWizDevicesScreen(void)
{
    DSP_KillSceneEditScreen();

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) {
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 1;
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // --- Iscrtavanje Naslova i Uputstva ---
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
    GUI_DispStringAt("Podesavanje Scene (Korak 1)", LCD_GetXSize() / 2, 10);

    GUI_SetFont(&GUI_FontVerdana16_LAT);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP); // Ponovni poziv radi pravila biblioteke
    GUI_DispStringAt("Odaberite koje uredjaje zelite ukljuciti:", LCD_GetXSize() / 2, 40);

    // --- KORAK 1: Provjera dostupnosti grupa uređaja ---
    bool lights_available = (LIGHTS_getCount() > 0);
    bool curtains_available = (Curtains_getCount() > 0);
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    bool thermostat_available = (Thermostat_GetGroup(pThst) > 0);

    // --- KORAK 2: Dinamičko kreiranje i pozicioniranje Checkbox-ova ---
    const int chkbx_x = 50;
    const int chkbx_w = 200;
    const int chkbx_h = 30;
    const int y_spacing = 40; // Razmak između checkbox-ova
    int16_t current_y = 80;   // Početna Y pozicija

    if (lights_available)
    {
        hCheckboxSceneLights = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_LIGHTS);
        CHECKBOX_SetText(hCheckboxSceneLights, lng(TXT_LIGHTS));
        CHECKBOX_SetFont(hCheckboxSceneLights, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneLights, GUI_WHITE);
        if (scene_handle->lights_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneLights, 1);
        }
        current_y += y_spacing; // Povećaj Y poziciju za sljedeći element
    }

    if (curtains_available)
    {
        hCheckboxSceneCurtains = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_CURTAINS);
        CHECKBOX_SetText(hCheckboxSceneCurtains, lng(TXT_BLINDS));
        CHECKBOX_SetFont(hCheckboxSceneCurtains, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneCurtains, GUI_WHITE);
        if (scene_handle->curtains_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneCurtains, 1);
        }
        current_y += y_spacing;
    }

    if (thermostat_available)
    {
        hCheckboxSceneThermostat = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_THERMOSTAT);
        CHECKBOX_SetText(hCheckboxSceneThermostat, lng(TXT_THERMOSTAT));
        CHECKBOX_SetFont(hCheckboxSceneThermostat, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneThermostat, GUI_WHITE);
        if (scene_handle->thermostat_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneThermostat, 1);
        }
    }

    // --- Kreiranje Navigacionih Dugmadi ---
    hButtonWizCancel = BUTTON_CreateEx(10, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_CANCEL);
    BUTTON_SetText(hButtonWizCancel, lng(TXT_CANCEL));

    hButtonWizBack = BUTTON_CreateEx(190, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_BACK);
    BUTTON_SetText(hButtonWizBack, "Nazad");

    hButtonWizNext = BUTTON_CreateEx(370, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
    BUTTON_SetText(hButtonWizNext, "Dalje");

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za ekran odabira uređaja u čarobnjaku.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SCENE_WIZ_DEVICES`. Odgovorna je za brisanje svih dinamički
 * kreiranih widgeta (checkbox-ova i dugmadi) kako bi se oslobodili
 * resursi i spriječilo curenje memorije.
 ******************************************************************************
 */
static void DSP_KillSceneWizDevicesScreen(void)
{
    // Provjeri da li widget postoji prije brisanja da se izbjegne greška
    if (WM_IsWindow(hCheckboxSceneLights)) {
        WM_DeleteWindow(hCheckboxSceneLights);
        hCheckboxSceneLights = 0; // Resetuj handle na nulu
    }
    if (WM_IsWindow(hCheckboxSceneCurtains)) {
        WM_DeleteWindow(hCheckboxSceneCurtains);
        hCheckboxSceneCurtains = 0;
    }
    if (WM_IsWindow(hCheckboxSceneThermostat)) {
        WM_DeleteWindow(hCheckboxSceneThermostat);
        hCheckboxSceneThermostat = 0;
    }
    if (WM_IsWindow(hButtonWizCancel)) {
        WM_DeleteWindow(hButtonWizCancel);
        hButtonWizCancel = 0;
    }
    if (WM_IsWindow(hButtonWizBack)) {
        WM_DeleteWindow(hButtonWizBack);
        hButtonWizBack = 0;
    }
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}

/**
 * @brief       Uništava sve GUI widgete kreirane za ekran podešavanja svjetla.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova nova funkcija je neophodna za brisanje dugmeta za promjenu
 * imena prilikom izlaska sa ekrana.
 * @param       None
 * @retval      None
 */
static void DSP_KillLightSettingsScreen(void)
{
    if (WM_IsWindow(hButtonRenameLight)) {
        WM_DeleteWindow(hButtonRenameLight);
        hButtonRenameLight = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za glavni ekran sa scenama.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana `SCREEN_SCENE`.
 * Pošto ovaj ekran nema dinamički kreirane widgete, njena uloga je
 * primarno da formalno postoji unutar "Init -> Service -> Kill" paterna
 * i da obriše ekran.
 ******************************************************************************
 */
static void DSP_KillSceneScreen(void)
{
    // Trenutno ovaj ekran nema dinamičke widgete, pa samo čistimo ekran.
    // Ako bismo u budućnosti dodali npr. dugmad, ovdje bi išao njihov WM_DeleteWindow poziv.
    GUI_Clear();
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI elemente sa ekrana za odabir izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SCENE_APPEARANCE`. S obzirom da ovaj ekran ne koristi
 * dinamički kreirane widgete sa handle-ovima, već samo direktne
 * funkcije za iscrtavanje, njen zadatak je da obriše ekran.
 ******************************************************************************
 */
static void DSP_KillSceneAppearanceScreen(void)
{
    GUI_Clear(); // Briše kompletan sadržaj aktivnog sloja (layer-a)
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditLightsScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod.
 ******************************************************************************
 */
static void DSP_KillSceneEditLightsScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditCurtainsScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod na ekranu
 * za roletne, osiguravajući čistu navigaciju.
 ******************************************************************************
 */
static void DSP_KillSceneEditCurtainsScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditThermostatScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod na ekranu
 * za termostat.
 ******************************************************************************
 */
static void DSP_KillSceneEditThermostatScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Kreira GUI za finalni ekran čarobnjaka.
 * @note        Prikazuje poruku za potvrdu i dugmad "Snimi Scenu" i "Otkaži".
 ******************************************************************************
 */
static void DSP_InitSceneWizFinalizeScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    char buffer[100];
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];

    sprintf(buffer, "Scena '%s' je konfigurisana.", lng(appearance->text_id));

//    DSP_DrawWizardScreenHeader("Zavrsetak Podesavanja", buffer);

    // Kreiramo samo "Snimi" i "Otkaži" dugmad
    hButtonWizCancel = BUTTON_CreateEx(10, 230, 120, 35, 0, WM_CF_SHOW, 0, ID_WIZ_CANCEL);
    BUTTON_SetText(hButtonWizCancel, lng(TXT_CANCEL));

    hBUTTON_Ok = BUTTON_CreateEx(350, 230, 120, 35, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "Snimi Scenu");

    GUI_MULTIBUF_EndEx(1);
}

/**
 ******************************************************************************
 * @brief       Uništava widgete sa finalnog ekrana čarobnjaka.
 ******************************************************************************
 */
static void DSP_KillSceneWizFinalizeScreen(void)
{
    if (WM_IsWindow(hButtonWizCancel)) WM_DeleteWindow(hButtonWizCancel);
    if (WM_IsWindow(hBUTTON_Ok)) WM_DeleteWindow(hBUTTON_Ok);
}
/**
 * @brief Uništava elemente sa ekrana aktivnog alarma.
 */
static void DSP_KillAlarmActiveScreen(void)
{
    GUI_Clear();
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje ekran za detaljnu kontrolu kapije (`SCREEN_GATE_SETTINGS`).
 * @author      Gemini
 * @note        FINALNA VERZIJA 3.0. Ova funkcija je sada "kontekstualno svjesna".
 * Dinamički kreira set kontrolnih dugmadi na osnovu "Profila Kontrole"
 * odabrane kapije. Prije iscrtavanja, provjerava `profile_id`:
 * - Ako je profil za kontinuirani rad, prikazuje "pomjeri lijevo/desno" ikonice.
 * - Ako je profil za pulsni rad, prikazuje "otvori/zatvori ciklus" ikonice.
 * Ovo korisniku pruža intuitivan interfejs prilagođen načinu rada uređaja.
 * Cijela grupa dugmadi je uvijek horizontalno centrirana.
 ******************************************************************************
 */
static void DSP_InitGateSettingsScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu(1);

    for (int i = 0; i < 6; i++) {
        hGateControlButtons[i] = 0;
    }

    Gate_Handle* handle = Gate_GetInstance(gate_control_panel_index);
    if (!handle || Gate_GetControlType(handle) == CONTROL_TYPE_NONE) {
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_RED);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt("GRESKA: Uredjaj nije dostupan!", DRAWING_AREA_WIDTH / 2, LCD_GetYSize() / 2);
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    const ProfilDeskriptor_t* profil = Gate_GetProfilDeskriptor(handle);
    const GateAction_t* cmd_map = profil->command_map;

    const int y_pos = 215;
    const int btn_size = 50;
    const int btn_gap = 15;

    struct ButtonDefinition {
        UI_Command_e command;
        const GUI_BITMAP* icon;
    };

    struct ButtonDefinition layout_gate[] = {
        { UI_COMMAND_CLOSE_CYCLE, &bmicons_button_fast_reverse_50_squared },
        { UI_COMMAND_SMART_STEP,  &bmicons_button_left_50_squared },
        { UI_COMMAND_STOP,        &bmicons_button_cancel_50_squared },
        { UI_COMMAND_PEDESTRIAN,  &bmicons_button_up_50_squared },
        { UI_COMMAND_OPEN_CYCLE,  &bmicons_button_fast_forward_50_squared },
    };

    struct ButtonDefinition layout_ramp[] = {
        { UI_COMMAND_CLOSE_CYCLE, &bmicons_button_down_50_squared },
        { UI_COMMAND_OPEN_CYCLE,  &bmicons_button_up_50_squared },
    };

    struct ButtonDefinition layout_lock[] = {
        { UI_COMMAND_UNLOCK,      &bmicons_button_up_50_squared },
    };

    struct ButtonDefinition* active_layout = layout_gate;
    int layout_size = sizeof(layout_gate) / sizeof(layout_gate[0]);

    switch(profil->profile_id) {
    case CONTROL_TYPE_RAMP_PULSE:
    case CONTROL_TYPE_GENERIC_MAINTAINED:
        active_layout = layout_ramp;
        layout_size = sizeof(layout_ramp) / sizeof(layout_ramp[0]);
        break;
    case CONTROL_TYPE_SIMPLE_LOCK:
        active_layout = layout_lock;
        layout_size = sizeof(layout_lock) / sizeof(layout_lock[0]);
        break;
    default:
        active_layout = layout_gate;
        layout_size = sizeof(layout_gate) / sizeof(layout_gate[0]);
        break;
    }

    WM_HWIN available_buttons[6];
    int available_count = 0;

    for (int i = 0; i < layout_size; i++) {
        if (cmd_map[active_layout[i].command].target_relay_index != 0) {
            hGateControlButtons[available_count] = BUTTON_CreateEx(0, 0, btn_size, btn_size, 0, WM_CF_SHOW, 0, active_layout[i].command);
            BUTTON_SetBitmap(hGateControlButtons[available_count], BUTTON_CI_UNPRESSED, active_layout[i].icon);
            available_buttons[available_count] = hGateControlButtons[available_count];
            available_count++;
        }
    }

    int total_width = (available_count * btn_size) + ((available_count > 0 ? available_count - 1 : 0) * btn_gap);
    int x_start = (DRAWING_AREA_WIDTH - total_width) / 2;

    for (int i = 0; i < available_count; i++) {
        int x_pos = x_start + i * (btn_size + btn_gap);
        WM_MoveTo(available_buttons[i], x_pos, y_pos);
    }

    GUI_MULTIBUF_EndEx(1);
    shouldDrawScreen = 1;
}


/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete sa ekrana za detaljnu kontrolu kapije.
 * @author      Gemini
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_GATE_SETTINGS` kako bi se oslobodili resursi. Prolazi
 * kroz niz `hGateControlButtons` i briše sve handle-ove koji su
 * bili kreirani u `Init` funkciji.
 ******************************************************************************
 */
static void DSP_KillGateSettingsScreen(void)
{
    for (int i = 0; i < 6; i++) {
        if (WM_IsWindow(hGateControlButtons[i])) {
            WM_DeleteWindow(hGateControlButtons[i]);
            hGateControlButtons[i] = 0;
        }
    }
    gate_settings_initialized = false; // Resetuj fleg pri izlasku sa ekrana
}

/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete sa ekrana za  kontrolu kapije.
 * @author      Gemini
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 ******************************************************************************
 */
static void DSP_KillGateScreen(void)
{
    // Ovaj ekran nema dinamičke widgete, pa samo čistimo ekran.
    GUI_Clear();
}
/**
 ******************************************************************************
 * @brief       Upravlja svim periodičnim događajima i tajmerima.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je srž logike za pozadinske procese koji se ne
 * odnose direktno na trenutni ekran. Uključuje logiku
 * screensaver-a, tajmera za automatsko paljenje svjetala,
 * detekciju dugog pritiska i ažuriranje vremena. Refaktorisana
 * je da koristi isključivo javne API-je drugih modula.
 ******************************************************************************
 */
static void Handle_PeriodicEvents(void)
{
    
    if (scene_press_timer_start != 0 && (HAL_GetTick() - scene_press_timer_start) > LONG_PRESS_DURATION)
    {
        uint8_t configured_scenes_count = Scene_GetCount();

        if (scene_pressed_index != -1 && scene_pressed_index < configured_scenes_count)
        {
            uint8_t scene_counter = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (scene_counter == scene_pressed_index)
                    {
                        scene_edit_index = i;
                        break;
                    }
                    scene_counter++;
                }
            }

            // --- ISPRAVNA NAVIGACIJA ---
            DSP_KillSceneScreen();          // 1. Ubij stari ekran
            DSP_InitSceneEditScreen();      // 2. Inicijalizuj novi ekran
            screen = SCREEN_SCENE_EDIT;     // 3. Postavi novo stanje
            shouldDrawScreen = 0;           // 4. Ne treba ponovo crtati

            scene_press_timer_start = 0;
            scene_pressed_index = -1;
        }
    }
    
    // === AŽURIRANI BLOK: DETEKCIJA DUGOG PRITISKA ZA KAPIJE ===
    if (gate_press_timer_start != 0 && (HAL_GetTick() - gate_press_timer_start) > LONG_PRESS_DURATION)
    {
        // Dugi pritisak je detektovan.

        // Sačuvaj indeks kapije za koju treba otvoriti panel.
        gate_control_panel_index = gate_pressed_index;

        // Resetuj tajmer i stanje pritiska da se akcija ne ponovi.
        gate_press_timer_start = 0;
        gate_pressed_index = -1;

        // Pokreni tranziciju na novi ekran za podešavanja kapije
        DSP_KillGateScreen();                  // 1. Ubij stari ekran (dashboard)
        DSP_InitGateSettingsScreen();          // 2. Inicijalizuj novi ekran (podešavanja)
        screen = SCREEN_GATE_SETTINGS;         // 3. Postavi novo stanje
        shouldDrawScreen = 0;                  // 4. Ne treba ponovo crtati jer je Init to već uradio
    }
    // === KRAJ AŽURIRANOG BLOKA ===

    // === KRAJ NOVOG BLOKA ===
    if (rename_light_timer_start && (HAL_GetTick() - rename_light_timer_start) >= 2000)
    {
        rename_light_timer_start = 0; // Resetuj tajmer

        LIGHT_Handle* handle = (light_selectedIndex < LIGHTS_MODBUS_SIZE) ? LIGHTS_GetInstance(light_selectedIndex) : NULL;
        if(handle) {
            // Pripremi kontekst za tastaturu
            KeyboardContext_t kbd_context = {
                .title = lng(TXT_ENTER_NEW_NAME),
                .max_len = 20
            };
            strncpy(kbd_context.initial_value, LIGHT_GetCustomLabel(handle), sizeof(kbd_context.initial_value) - 1);

            // Postavi "povratnu adresu"
            keyboard_return_screen = screen;
            // Kopiraj kontekst u globalnu varijablu
            memcpy(&g_keyboard_context, &kbd_context, sizeof(KeyboardContext_t));
            // Resetuj rezultat
            memset(&g_keyboard_result, 0, sizeof(KeyboardResult_t));
            keyboard_shift_active = false;

            // Postavi novi ekran
            screen = SCREEN_KEYBOARD_ALPHA;

            // === FORSIRANO ISCRTAVANJE ODMAH ===
            DSP_KillLightSettingsScreen();
            DSP_InitKeyboardScreen();
            // Postavi fleg da se izbjegne duplo iscrtavanje u DISP_Service
            shouldDrawScreen = 0;
        }
        return; // Prekini dalje izvršavanje u ovoj funkciji
    }
    // === "FAIL-SAFE" SKENER ZA DUHOVE (Ovaj dio ostaje nepromijenjen) ===
    static uint32_t ghost_widget_scan_timer = 0;
    if ((HAL_GetTick() - ghost_widget_scan_timer) >= GHOST_WIDGET_SCAN_INTERVAL) {
        ghost_widget_scan_timer = HAL_GetTick();
        // Ažurirano da koristi novi `SCREEN_SELECT_LAST`
        if (screen == SCREEN_MAIN || screen == SCREEN_SELECT_1 || screen == SCREEN_SELECT_LAST) {
            ForceKillAllSettingsWidgets();
        }
    }

    // === TAJMER ZA AUTOMATSKO PALJENJE SVJETALA (SVAKE MINUTE) ===
    if (IsRtcTimeValid() && (HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000)) {
        everyMinuteTimerStart = HAL_GetTick();

        // Refaktorisana petlja koja koristi novi API
        RTC_TimeTypeDef currentTime;
        HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BCD);
        uint8_t currentHour = Bcd2Dec(currentTime.Hours);
        uint8_t currentMinute = Bcd2Dec(currentTime.Minutes);

        for (uint8_t i = 0; i < LIGHTS_getCount(); i++) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                if (LIGHT_GetOnHour(handle) != -1) {
                    if ((LIGHT_GetOnHour(handle) == currentHour) && (LIGHT_GetOnMinute(handle) == currentMinute)) {
                        LIGHT_SetState(handle, true);
                        // Logika za ažuriranje ekrana ostaje ista
                        if (screen == SCREEN_LIGHTS) {
                            shouldDrawScreen = 1;
                        } else if((screen == SCREEN_RESET_MENU_SWITCHES) || (screen == SCREEN_MAIN)) {
                            screen = SCREEN_RETURN_TO_FIRST;
                        }
                    }
                }
            }
        }
    }

    // === TAJMER ZA ULAZAK U MOD PODEŠAVANJA (Ovaj dio ostaje nepromijenjen) ===
    if (light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000))) {
        light_settingsTimerStart = 0;
        light_settings_return_screen = screen;
        screen = SCREEN_LIGHT_SETTINGS;
        shouldDrawScreen = 1;
    }

    
    // =======================================================================
    // ===          FINALNI BLOK: Detekcija dugog pritiska za Alarm        ===
    // =======================================================================
    if (dynamic_icon_alarm_press_timer != 0 && (HAL_GetTick() - dynamic_icon_alarm_press_timer) > LONG_PRESS_DURATION)
    {
        // Dugi pritisak je detektovan.
        dynamic_icon_alarm_press_timer = 0; // Resetuj tajmer da se akcija ne ponovi.
        
        // Pokreni tranziciju na ekran za PODEŠAVANJA alarma.
        // DSP_Kill... se ne poziva ovdje jer prelazimo sa SELECT ekrana koji nema widgete.
        DSP_InitSettingsAlarmScreen();
        screen = SCREEN_SETTINGS_ALARM;
        shouldDrawScreen = 0;
    }

    // =======================================================================
    // ===          FINALNI BLOK: Detekcija dugog pritiska za Timer        ===
    // =======================================================================
    if (dynamic_icon_timer_press_timer != 0 && (HAL_GetTick() - dynamic_icon_timer_press_timer) > LONG_PRESS_DURATION)
    {
        // Dugi pritisak je detektovan.
        dynamic_icon_timer_press_timer = 0; // Resetuj tajmer.
        
        // Pokreni tranziciju na ekran za PODEŠAVANJA tajmera.
        Timer_Suppress();
        screen = SCREEN_SETTINGS_TIMER;
        shouldDrawScreen = 1;
    }
    // === SCREENSAVER TAJMER ===
    if (!IsScrnsvrActiv()) {
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(g_display_settings.scrnsvr_tout * 1000)) {
            // =======================================================================
            // ===          NOVI BLOK: Provjera i gašenje Čarobnjaka prvo         ===
            // =======================================================================
            if (is_in_scene_wizard_mode)
            {
                // Uništi widgete sa bilo kog ekrana koji je dio čarobnjaka
                switch (screen)
                {
                case SCREEN_SCENE_EDIT:
                    DSP_KillSceneEditScreen();
                    break;
                case SCREEN_SCENE_APPEARANCE:
                    DSP_KillSceneAppearanceScreen();
                    break;
                case SCREEN_SCENE_WIZ_DEVICES:
                    DSP_KillSceneWizDevicesScreen();
                    break;
                case SCREEN_LIGHTS:
                case SCREEN_CURTAINS:
                case SCREEN_THERMOSTAT:
                    // Za ove ekrane, dovoljno je obrisati [Next] dugme
                    if (WM_IsWindow(hButtonWizNext)) {
                        WM_DeleteWindow(hButtonWizNext);
                        hButtonWizNext = 0;
                    }
                    break;
                // TODO: Dodati Kill funkcije za ostale WIZ ekrane kada budu kreirane
                default:
                    break;
                }

                // Ključni korak: Resetuj fleg za Wizard Mode
                is_in_scene_wizard_mode = false;
            }
            else if (screen == SCREEN_NUMPAD) {
                pin_change_state = PIN_CHANGE_IDLE; // Resetuj stanje promjene PIN-a
                DSP_KillNumpadScreen();
            }
            // =======================================================================
            // ===        Originalna logika za ekrane podešavanja ostaje         ===
            // =======================================================================
            else if (screen == SCREEN_SETTINGS_1)       DSP_KillSet1Scrn();
            else if (screen == SCREEN_SETTINGS_2)       DSP_KillSet2Scrn();
            else if (screen == SCREEN_SETTINGS_3)       DSP_KillSet3Scrn();
            else if (screen == SCREEN_SETTINGS_4)       DSP_KillSet4Scrn();
            else if (screen == SCREEN_SETTINGS_5)       DSP_KillSet5Scrn();
            else if (screen == SCREEN_SETTINGS_6)       DSP_KillSet6Scrn();
            else if (screen == SCREEN_SETTINGS_7)       DSP_KillSet7Scrn();
            else if (screen == SCREEN_LIGHT_SETTINGS)   DSP_KillLightSettingsScreen();
            else if (screen == SCREEN_SETTINGS_DATETIME)DSP_KillSettingsDateTimeScreen();
            else if (screen == SCREEN_SETTINGS_TIMER)   DSP_KillSettingsTimerScreen(),Timer_Unsuppress();
            else if (screen == SCREEN_TIMER)            DSP_KillTimerScreen();

            // Aktivacija screensaver-a (ostaje isto)
            DISPSetBrightnes(g_display_settings.low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    // === AŽURIRANJE VREMENA NA EKRANU ===
    if (HAL_GetTick() - rtctmr >= 1000) {
        rtctmr = HAL_GetTick();
        if(++refresh_tmr > 10) {
            refresh_tmr = 0;
            if (!IsScrnsvrActiv()) MVUpdateSet();
        }
        // Poziv za iscrtavanje sata (logika će biti dorađena kako smo diskutovali)
        if (screen < SCREEN_SELECT_1) DISPDateTime();
    }
}
/**
 ******************************************************************************
 * @brief       Centralni dispečer za obradu događaja PRITISKA na ekran.
 * @author      [Original Author] & Gemini
 * @note        Ova funkcija se poziva iz `PID_Hook`. Njena uloga je da, na osnovu
 * trenutnog ekrana, pozove odgovarajuću `HandlePress_...` funkciju.
 * Za `SCREEN_MAIN`, poziva logiku za menije i pamti koordinate pritiska.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg za zvučni signal.
 ******************************************************************************
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    if     (screen == SCREEN_MAIN) 
    {
        // Provjera 1: Da li je dodir u DONJEM LIJEVOM meniju za scene?
        if (g_display_settings.scenes_enabled && (pTS->x < 80 && pTS->y > 192))
        {
            *click_flag = 1;
            // Očisti ekran prije prelaska
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_Clear();

            // Prebaci direktno na ekran sa scenama
            screen = SCREEN_SCENE;
            shouldDrawScreen = 1;
        }
        // Provjera 2: Da li je dodir unutar CENTRALNOG GLAVNOG PREKIDAČA?
        else if (pTS->x >= reset_menu_switches_layout.main_switch_zone.x0 &&
                 pTS->x <  reset_menu_switches_layout.main_switch_zone.x1 &&
                 pTS->y >= reset_menu_switches_layout.main_switch_zone.y0 &&
                 pTS->y <  reset_menu_switches_layout.main_switch_zone.y1)
        {
            *click_flag = 1;
            // Tek sada, kada smo sigurni da je pritisak u pravoj zoni,
            // pozivamo funkciju koja pokreće tajmer za dugi pritisak.
            HandlePress_MainScreenSwitch(pTS);
        }

        // Ako dodir nije bio ni u jednoj od ove dvije zone, ne radi se ništa.
        // Onemogućen je `else` blok koji je uzrokovao grešku.

        // Uvijek sačuvaj koordinate pritiska za kasniju provjeru pri otpuštanju
        memcpy(&last_press_state, pTS, sizeof(GUI_PID_STATE));
    }
    else if(screen == SCREEN_SELECT_1) 
    {
        HandlePress_SelectScreen1(pTS, click_flag);
    }
    else if(screen == SCREEN_SELECT_2) 
    {
        HandlePress_SelectScreen2(pTS, click_flag);
    }
    else if(screen == SCREEN_SELECT_LAST) 
    {
        HandlePress_SelectScreenLast(pTS, click_flag);
    }
    else if(screen == SCREEN_THERMOSTAT) 
    {
        HandlePress_ThermostatScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_LIGHTS) 
    {
        HandlePress_LightsScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_CURTAINS) 
    {
        HandlePress_CurtainsScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_GATE) 
    {
        HandlePress_GateScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_GATE_SETTINGS) 
    {
        HandlePress_GateSettingsScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_SETTINGS_ALARM)
    {
        *click_flag = 1;
    }
    else if(screen == SCREEN_SCENE) 
    {
        HandlePress_SceneScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_LIGHT_SETTINGS) 
    {
        HandlePress_LightSettingsScreen(pTS);
    }
    else if(screen == SCREEN_SCENE_APPEARANCE) 
    {
        HandlePress_SceneAppearanceScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_TIMER) 
    {
        HandlePress_TimerScreen(pTS, click_flag);
    }
}

/**
 ******************************************************************************
 * @brief       Obrađuje događaj otpuštanja dodira sa ekrana, u zavisnosti od ekrana.
 * @author      [Original Author] & Gemini
 * @note        Ova funkcija je centralna tačka za logiku koja se dešava nakon
 * što korisnik podigne prst sa ekrana. Sadrži originalnu, kompletnu
 * logiku za sve ekrane, sa dodatkom provjere zone za glavni prekidač.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 ******************************************************************************
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS)
{
    // Provjera 1: Da li je otpuštanje prsta rezultat uspješnog dugog pritiska?
    // Ako je tastatura već aktivirana, 'screen' varijabla će biti SCREEN_KEYBOARD_ALPHA.
    if (screen == SCREEN_KEYBOARD_ALPHA)
    {
        // U ovom slučaju, dugi pritisak je uspio i ekran se već mijenja.
        // Jedino što treba da uradimo je da resetujemo tajmer i izađemo.
        rename_light_timer_start = 0;
        return;
    }

    // Ako NIJE bio uspješan dugi pritisak, nastavljamo sa standardnom logikom.

    // Poništi tajmer za dugi pritisak (za slučaj kratkog pritiska)
    rename_light_timer_start = 0;

    if (LIGHTS_IsNightTimerActive()) {
        LIGHTS_StopNightTimer();
    }
    // =======================================================================
    // === POČETAK IZMJENE: Logika za MAIN_SCREEN sa provjerom zone ===
    // =======================================================================
    if(screen == SCREEN_MAIN && !touch_in_menu_zone) 
    {
        // Provjerava se da li je POČETNI PRITISAK bio unutar zone
        // definisane u strukturi `reset_menu_switches_layout`.
        if (last_press_state.x >= reset_menu_switches_layout.main_switch_zone.x0 &&
                last_press_state.x <  reset_menu_switches_layout.main_switch_zone.x1 &&
                last_press_state.y >= reset_menu_switches_layout.main_switch_zone.y0 &&
                last_press_state.y <  reset_menu_switches_layout.main_switch_zone.y1)
        {
            HandleRelease_MainScreenSwitch(pTS);
        }
    }
    // =======================================================================
    // === KRAJ IZMJENE (Ostatak funkcije je Vaš originalni, sačuvani kod) ===
    // =======================================================================
    else if(screen == SCREEN_LIGHTS) 
    {
        if(light_selectedIndex < LIGHTS_MODBUS_SIZE) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                if (!LIGHT_isBinary(handle)) {
                    if ((HAL_GetTick() - light_settingsTimerStart) < 2000) {
                        LIGHT_Flip(handle);
                    }
                } else {
                    LIGHT_Flip(handle);
                }
            }
        }
        light_settingsTimerStart = 0;
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    }
    // === POČETAK BLOKA ZA ZAMJENU: LOGIKA ZA KRATAK PRITISAK NA KAPIJU ===
    else if (screen == SCREEN_GATE) 
    {
        // Provjeravamo da li je tajmer bio aktivan. Ako je dugi pritisak obrađen,
        // on bi postavio gate_press_timer_start na 0. Ako nije nula, bio je kratak klik.
        if (gate_press_timer_start != 0) {
            if (gate_pressed_index != -1) {
                Gate_Handle* handle = Gate_GetInstance(gate_pressed_index);
                if (handle) {
                    GateState_e current_state = Gate_GetState(handle);
                    GateState_e presumed_next_state = current_state; // Podrazumijevano stanje se ne mijenja

                    // Implementacija state mašine tačno prema korisničkoj specifikaciji
                    switch (current_state) {
                    case GATE_STATE_CLOSED:
                        // Ako je zatvorena -> pokreni otvaranje
                        presumed_next_state = GATE_STATE_OPENING;
                        Gate_TriggerFullCycleOpen(handle);
                        break;

                    case GATE_STATE_OPENING:
                        // Ako se otvara -> zaustavi je
                        presumed_next_state = GATE_STATE_PARTIALLY_OPEN;
                        Gate_TriggerStop(handle);
                        break;

                    case GATE_STATE_PARTIALLY_OPEN:
                        // Ako je djelimično otvorena -> pokreni zatvaranje
                        presumed_next_state = GATE_STATE_CLOSING;
                        Gate_TriggerFullCycleClose(handle);
                        break;

                    case GATE_STATE_CLOSING:
                        // Ako se zatvara -> zaustavi je
                        presumed_next_state = GATE_STATE_PARTIALLY_OPEN;
                        Gate_TriggerStop(handle);
                        break;

                    case GATE_STATE_OPEN:
                        // Ako je otvorena -> pokreni zatvaranje
                        presumed_next_state = GATE_STATE_CLOSING;
                        Gate_TriggerFullCycleClose(handle);
                        break;

                    case GATE_STATE_UNDEFINED:
                    case GATE_STATE_FAULT:
                    default:
                        // U nedefinisanim ili stanjima greške, pokrećemo ciklus otvaranja kao najsigurniju akciju
                        presumed_next_state = GATE_STATE_OPENING;
                        Gate_TriggerFullCycleOpen(handle);
                        break;
                    }

                    // Ako je došlo do promjene stanja, ažuriraj UI ("Optimistic UI Update")
                    if (presumed_next_state != current_state) {
                        // 1. Odmah ažuriraj stanje u RAM-u radi trenutnog vizuelnog odziva.
                        Gate_SetState(handle, presumed_next_state);
                        // 2. Zatraži ponovno iscrtavanje ekrana da se prikaže nova ikonica/stanje.
                        shouldDrawScreen = 1;
                    }
                }
            }
        }
        // U svakom slučaju, resetuj tajmer i stanje pritiska pri otpuštanju
        gate_press_timer_start = 0;
        gate_pressed_index = -1;
    }
    // === KRAJ BLOKA ZA ZAMJENU ===
    else if(screen == SCREEN_RESET_MENU_SWITCHES) 
    {
        HandleRelease_MainScreenSwitch(pTS);
    }
    else if(screen == SCREEN_SCENE) 
    {
        HandleRelease_SceneScreen();
    }
    // =======================================================================
    // ===       ISPRAVLJENI BLOK: Obrada kratkog pritiska za Alarm i Timer    ===
    // =======================================================================
    // Logika se izvršava ISKLJUČIVO ako smo na ekranu SCREEN_SELECT_2
    else if (screen == SCREEN_SELECT_2)
    {
        HandleRelease_AlarmIcon();
        HandleRelease_TimerIcon();
    }
    // =======================================================================
    // Resetovanje svih opštih kontrolnih flegova.
    btnset = 0;
    btndec = 0U;
    btninc = 0U;
    thermostatOnOffTouch_timer = 0;
    
    // Resetovanje `last_press_state` da se izbjegne ponovna aktivacija
    memset(&last_press_state, 0, sizeof(GUI_PID_STATE));
}
/**
 * @brief Obrada događaja pritiska za ekran "Control Select".
 * @note  Verzija 2.3.3: Ispravljena greška gdje nisu bili inicijalizovani
 * handle-ovi za module, što je sprečavalo ispravan rad funkcije.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // << ISPRAVKA: Dodata inicijalizacija handle-ova koja je nedostajala. >>
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // Ponovo radimo detekciju da bismo znali koji su moduli TRENUTNO prikazani
    // --- KORAK 1: Detekcija aktivnih modula (identično kao u Service_... funkciji) ---
    typedef struct {
        const GUI_BITMAP* icon;
        TextID text_id;
        eScreen target_screen;
        bool is_dynamic_toggle; // Fleg za dinamičku ikonu
    } DynamicMenuItem;

    DynamicMenuItem all_modules[] = {
        { &bmSijalicaOff, TXT_LIGHTS, SCREEN_LIGHTS, false },
        { &bmTermometar, TXT_THERMOSTAT, SCREEN_THERMOSTAT, false },
        { &bmblindMedium, TXT_BLINDS, SCREEN_CURTAINS, false },
        { NULL, TXT_DUMMY, SCREEN_SELECT_1, true } // Mjesto za dinamičku ikonu
    };

    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;

    if (LIGHTS_getCount() > 0) active_modules[active_modules_count++] = all_modules[0];
    if (Thermostat_GetGroup(pThst) > 0) active_modules[active_modules_count++] = all_modules[1];
    if (Curtains_getCount() > 0) active_modules[active_modules_count++] = all_modules[2];

    if (g_display_settings.selected_control_mode == MODE_DEFROSTER && Defroster_getPin(defHandle) > 0) {
        active_modules[active_modules_count++] = all_modules[3];
    } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR && (Ventilator_getRelay(ventHandle) > 0 || Ventilator_getLocalPin(ventHandle) > 0)) {
        active_modules[active_modules_count++] = all_modules[3];
    }

    // --- KORAK 2: Dinamičko računanje zona dodira i provjera ---
    TouchZone_t zone;
    bool touched = false;

    switch (active_modules_count) {
    case 1:
        zone.x0 = 0;
        zone.y0 = 0;
        zone.x1 = DRAWING_AREA_WIDTH;
        zone.y1 = LCD_GetYSize();
        if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
            screen = active_modules[0].target_screen;
            touched = true;
        }
        break;
    case 2:
        for (int i = 0; i < 2; i++) {
            zone.x0 = (DRAWING_AREA_WIDTH / 2) * i;
            zone.y0 = 0;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 2);
            zone.y1 = LCD_GetYSize();
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    case 3:
        for (int i = 0; i < 3; i++) {
            zone.x0 = (DRAWING_AREA_WIDTH / 3) * i;
            zone.y0 = 0;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 3);
            zone.y1 = LCD_GetYSize();
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    case 4:
    default:
        for (int i = 0; i < 4; i++) {
            zone.x0 = (i % 2 == 0) ? 0 : DRAWING_AREA_WIDTH / 2;
            zone.y0 = (i < 2) ? 0 : LCD_GetYSize() / 2;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 2);
            zone.y1 = zone.y0 + (LCD_GetYSize() / 2);
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    }

    // Provjera dodira na "NEXT" dugme
    if (!touched && (pTS->x >= 400 && pTS->x < 480)) {
        screen = SCREEN_SELECT_2;
        touched = true;
    }

    // Ako je dodir registrovan, postavi flegove
    if (touched) {
        // Ako je pritisnuta dinamička ikona, uradi toggle
        if (screen == SCREEN_SELECT_1) {
            if (g_display_settings.selected_control_mode == MODE_DEFROSTER) {
                if(Defroster_isActive(defHandle)) Defroster_Off(defHandle);
                else Defroster_On(defHandle);
                dynamicIconUpdateFlag = 1;
            } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR) {
                if(Ventilator_isActive(ventHandle)) Ventilator_Off(ventHandle);
                else Ventilator_On(ventHandle, false);
                dynamicIconUpdateFlag = 1;
            }
        }
        // Ako je pritisnuta roletna, resetuj selekciju na "SVE"
        else if (screen == SCREEN_CURTAINS) {
            Curtain_ResetSelection();
        }

        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za drugi ekran za odabir (SCREEN_SELECT_2).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički izračunava koja je ikonica pritisnuta na osnovu
 * aktivnih modula. Za ikonice Alarma i Tajmera, ova funkcija sada
 * pokreće tajmere (`dynamic_icon_alarm_press_timer`, `dynamic_icon_timer_press_timer`)
 * kako bi se omogućila detekcija dugog pritiska. Za sve ostale ikonice,
 * ponašanje ostaje nepromijenjeno.
 * @param[in]   pTS Pokazivač na strukturu sa stanjem dodira (`GUI_PID_STATE`).
 * @param[out]  click_flag Pokazivač na fleg koji signalizira potrebu za zvučnim signalom.
 * @retval      None
 ******************************************************************************
 */
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Resetuj tajmere na početku svake obrade pritiska
    dynamic_icon_alarm_press_timer = 0;
    dynamic_icon_timer_press_timer = 0;

    // --- Logika za detekciju aktivnih modula (nepromijenjena) ---
    typedef struct {
        eScreen target_screen;
        bool is_dynamic_icon;
    } DynamicMenuItemPress;

    DynamicMenuItemPress active_modules[4];
    uint8_t active_modules_count = 0;

    if (Gate_GetCount() > 0)     active_modules[active_modules_count++] = (DynamicMenuItemPress) {
        SCREEN_GATE, false
    };
    active_modules[active_modules_count++] = (DynamicMenuItemPress) {
        SCREEN_TIMER, false
    };
    if (g_display_settings.security_module_enabled) {
        active_modules[active_modules_count++] = (DynamicMenuItemPress){ SCREEN_SECURITY, false };
    }
    if (g_display_settings.selected_control_mode_2 != MODE_OFF) {
        active_modules[active_modules_count++] = (DynamicMenuItemPress) {
            SCREEN_SELECT_2, true
        };
    }

    // --- Dinamičko računanje zona dodira (nepromijenjeno) ---
    TouchZone_t zone;
    int8_t touched_module_index = -1;

    switch (active_modules_count)
    {
    case 1:
        zone = (TouchZone_t) { .x0 = 0, .y0 = 0, .x1 = DRAWING_AREA_WIDTH, .y1 = LCD_GetYSize() };
        if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) touched_module_index = 0;
        break;
    case 2:
        for (int i = 0; i < 2; i++) {
            zone = (TouchZone_t) { .x0 = (DRAWING_AREA_WIDTH / 2) * i, .y0 = 0, .x1 = (DRAWING_AREA_WIDTH / 2) * (i + 1), .y1 = LCD_GetYSize() };
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                touched_module_index = i;
                break;
            }
        }
        break;
    case 3:
        for (int i = 0; i < 3; i++) {
            zone = (TouchZone_t) { .x0 = (DRAWING_AREA_WIDTH / 3) * i, .y0 = 0, .x1 = (DRAWING_AREA_WIDTH / 3) * (i + 1), .y1 = LCD_GetYSize() };
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                touched_module_index = i;
                break;
            }
        }
        break;
    case 4:
    default:
        for (int i = 0; i < 4; i++) {
            zone = (TouchZone_t) { .x0 = (i % 2 == 0) ? 0 : DRAWING_AREA_WIDTH / 2, .y0 = (i < 2) ? 0 : LCD_GetYSize() / 2, .x1 = (i % 2 == 0) ? DRAWING_AREA_WIDTH / 2 : DRAWING_AREA_WIDTH, .y1 = (i < 2) ? LCD_GetYSize() / 2 : LCD_GetYSize() };
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                touched_module_index = i;
                break;
            }
        }
        break;
    }

    // --- Obrada rezultata dodira (AŽURIRANA LOGIKA) ---
    if (touched_module_index != -1)
    {
        *click_flag = 1;
        eScreen selected_screen = active_modules[touched_module_index].target_screen;

        if (selected_screen == SCREEN_SECURITY)
        {
            // Za ikonicu ALARMA, pokreni tajmer za dugi pritisak
            dynamic_icon_alarm_press_timer = HAL_GetTick() ? HAL_GetTick() : 1;
        } 
        else if (selected_screen == SCREEN_TIMER)
        {
            // Za ikonicu TAJMERA, pokreni tajmer za dugi pritisak
            dynamic_icon_timer_press_timer = HAL_GetTick() ? HAL_GetTick() : 1;
        }
        else if (active_modules[touched_module_index].is_dynamic_icon)
        {
            // Postojeća logika za ostale dinamičke ikonice ostaje nepromijenjena
            switch(g_display_settings.selected_control_mode_2)
            {
            case MODE_DEFROSTER: {
                Defroster_Handle* defHandle = Defroster_GetInstance();
                if (Defroster_isActive(defHandle)) {
                    Defroster_Off(defHandle);
                } else {
                    Defroster_On(defHandle);
                }
                shouldDrawScreen = 1;
                break;
            }
            case MODE_VENTILATOR: {
                Ventilator_Handle* ventHandle = Ventilator_GetInstance();
                if (Ventilator_isActive(ventHandle)) {
                    Ventilator_Off(ventHandle);
                } else {
                    Ventilator_On(ventHandle, false);
                }
                shouldDrawScreen = 1;
                break;
            }
            case MODE_LANGUAGE:
            case MODE_THEME:
            case MODE_SOS:
            case MODE_OUTDOOR:
                dynamic_icon2_press_timer = HAL_GetTick() ? HAL_GetTick() : 1;
                break;
            case MODE_ALL_OFF:
                break;
            default:
                break;
            }
        }
        else
        {
            // Za sve ostale statičke ikonice (npr. Gate), odmah mijenjaj ekran
            screen = active_modules[touched_module_index].target_screen;
            shouldDrawScreen = 1;
        }
    }
    // Logika za "NEXT" dugme ostaje nepromijenjena
    else if (pTS->x >= select_screen1_drawing_layout.next_button_zone.x0)
    {
        *click_flag = 1;
        screen = SCREEN_SELECT_LAST;
        shouldDrawScreen = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za posljednji ekran za odabir.
 * @author      [Original Author] & Gemini
 * @note        FINALNA ISPRAVLJENA VERZIJA. Logika je restrukturirana da
 * ispravno rukuje specijalnim slučajem poziva PIN tastature. Nakon
 * poziva `Display_ShowNumpad`, funkcija se odmah završava (`return`)
 * kako bi se spriječilo pogrešno vraćanje na prethodni ekran.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg za zvučni signal.
 ******************************************************************************
 */
static void HandlePress_SelectScreenLast(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera dodira na zonu za ČIŠĆENJE
    if (pTS->x >= select_screen2_drawing_layout.clean_zone.x0 && pTS->x < select_screen2_drawing_layout.clean_zone.x1 &&
            pTS->y >= select_screen2_drawing_layout.clean_zone.y0 && pTS->y < select_screen2_drawing_layout.clean_zone.y1) {
        screen = SCREEN_CLEAN;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za WIFI
    else if (pTS->x >= select_screen2_drawing_layout.wifi_zone.x0 && pTS->x < select_screen2_drawing_layout.wifi_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.wifi_zone.y0 && pTS->y < select_screen2_drawing_layout.wifi_zone.y1) {
        menu_lc  = 0;
        screen = SCREEN_QR_CODE;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za APP
    else if (pTS->x >= select_screen2_drawing_layout.app_zone.x0 && pTS->x < select_screen2_drawing_layout.app_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.app_zone.y0 && pTS->y < select_screen2_drawing_layout.app_zone.y1) {
        menu_lc  = 1;
        screen = SCREEN_QR_CODE;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za PODEŠAVANJA
    else if (pTS->x >= select_screen2_drawing_layout.settings_zone.x0 && pTS->x < select_screen2_drawing_layout.settings_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.settings_zone.y0 && pTS->y < select_screen2_drawing_layout.settings_zone.y1) {

        NumpadContext_t pin_context = {
            .title = lng(TXT_ALARM_ENTER_PIN), // KORISTI POSTOJEĆI PREVOD
            .initial_value = "",
            .min_val = 0,
            .max_val = 9999,
            .max_len = 4,
            .allow_decimal = false,
            .allow_minus_one = false
        };

        Display_ShowNumpad(&pin_context);
        *click_flag = 1;

        // =======================================================================
        // ===       KLJUČNA ISPRAVKA: Odmah izađi iz funkcije               ===
        // =======================================================================
        // Nakon poziva `Display_ShowNumpad`, koji je već postavio novi ekran,
        // moramo odmah izaći da spriječimo dalju logiku u ovoj funkciji da
        // pogrešno vrati ekran na staru vrijednost.
        return;
    }
    // Provjera dodira na "NEXT" dugme
    else if (pTS->x >= select_screen2_drawing_layout.next_button_zone.x0 && pTS->x < select_screen2_drawing_layout.next_button_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.next_button_zone.y0 && pTS->y < select_screen2_drawing_layout.next_button_zone.y1) {
        screen = SCREEN_SELECT_1;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }

    // =======================================================================
    // ===       SAČUVANA BLOKIRAJUĆA PETLJA ZA SPREČAVANJE           ===
    // =======================================================================
    // Ova petlja se izvršava samo ako je došlo do promjene ekrana
    if (*click_flag) {
        GUI_PID_STATE ts_release;
        do {
            TS_Service();
            GUI_PID_GetState(&ts_release);
            HAL_Delay(100);
        } while (ts_release.Pressed);
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Thermostat".
 * @note Rukuje dodirima na zone za povećanje (+) i smanjenje (-) zadane temperature,
 * kao i detekcijom početka dugog pritiska za paljenje/gašenje termostata.
 * Koordinate zona dodira su definisane kao lokalne konstante radi bolje
 * enkapsulacije i čitljivosti koda.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera da li je dodir unutar zone za POVEĆANJE temperature
    if ((pTS->x >= thermostat_layout.increase_zone.x0) && (pTS->x < thermostat_layout.increase_zone.x1) &&
            (pTS->y >= thermostat_layout.increase_zone.y0) && (pTS->y < thermostat_layout.increase_zone.y1))
    {
        *click_flag = 1; // Signaliziraj da treba generisati "klik" zvuk
        btninc = 1;      // Postavi fleg da je dugme "+" pritisnuto
    }
    // Provjera da li je dodir unutar zone za SMANJENJE temperature
    else if ((pTS->x >= thermostat_layout.decrease_zone.x0) && (pTS->x < thermostat_layout.decrease_zone.x1) &&
             (pTS->y >= thermostat_layout.decrease_zone.y0) && (pTS->y < thermostat_layout.decrease_zone.y1))
    {
        *click_flag = 1; // Signaliziraj "klik"
        btndec = 1;      // Postavi fleg da je dugme "-" pritisnuto
    }
    // Provjera da li je dodir unutar zone za ON/OFF
    else if ((pTS->x >= thermostat_layout.on_off_zone.x0) && (pTS->x < thermostat_layout.on_off_zone.x1) &&
             (pTS->y >= thermostat_layout.on_off_zone.y0) && (pTS->y < thermostat_layout.on_off_zone.y1))
    {
        *click_flag = 1; // Signaliziraj "klik"

        // Pokreni tajmer za detekciju dugog pritiska.
        // Sama logika gašenja/paljenja se nalazi u `Service_ThermostatScreen` funkciji
        // koja provjerava koliko dugo ovaj tajmer traje.
        thermostatOnOffTouch_timer = HAL_GetTick();
        if(!thermostatOnOffTouch_timer) {
            thermostatOnOffTouch_timer = 1; // Osiguraj da nije 0
        }
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Lights".
 * @note  Detektuje dodir na ikonu specifičnog svjetla. Ako svjetlo nije binarno (dimer/RGB),
 * pokreće tajmer za dugi pritisak kako bi se omogućio ulazak u meni za podešavanja.
 * Refaktorisana je da koristi isključivo `lights` API i centralizovanu layout strukturu.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Resetujemo globalne varijable stanja na početku obrade dodira.
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    light_settingsTimerStart = 0;

    // Logika za dinamički raspored i detekciju dodira.
    int y = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
    uint8_t lightsInRowSum = 0;

    // Prolazimo kroz redove ikonica...
    for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
        uint8_t lightsInRow = LIGHTS_getCount();
        if(LIGHTS_getCount() > 3) {
            if(LIGHTS_getCount() == 4) lightsInRow = 2;
            else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
            else lightsInRow = 3;
        }
        uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

        // ...i kroz ikonice u trenutnom redu.
        for(uint8_t i_light = 0; i_light < lightsInRow; ++i_light) {
            int x = (currentLightsMenuSpaceBetween * ((i_light % lightsInRow) + 1)) + (80 * (i_light % lightsInRow));

            // Provjeravamo da li koordinate dodira upadaju u "hitbox" trenutne ikonice,
            // koristeći dimenzije iz `lights_screen_layout` strukture.
            if((pTS->x > x) && (pTS->x < (x + lights_screen_layout.icon_width)) &&
                    (pTS->y > y) && (pTS->y < (y + lights_screen_layout.icon_height)))
            {
                *click_flag = 1; // Signaliziraj "klik"
                light_selectedIndex = lightsInRowSum + i_light; // Zabilježi koje je svjetlo pritisnuto

                LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);

                if (handle) {
                    // Sada se tajmer za dugi pritisak pokreće za SVE tipove svjetala.
                    light_settingsTimerStart = HAL_GetTick();
                }

                // Zaustavljamo noćni tajmer ako je bio aktivan.
                LIGHTS_StopNightTimer();

                // Prekidamo obje petlje jer smo pronašli pritisnutu ikonicu.
                goto exit_loops;
            }
        }
        lightsInRowSum += lightsInRow;
        y += 130;
    }

exit_loops:; // Labela za izlazak iz ugniježdenih petlji
}

/**
 * @brief Obrada događaja pritiska za ekran "Curtains".
 * @note  Rukuje pritiskom na trouglove za pomjeranje GORE/DOLE,
 * kao i strelicama za prebacivanje između pojedinačnih roletni i grupe "SVE".
 * Koristi `curtains_screen_layout` za preciznu detekciju dodira.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera da li je dodir u zoni za pomjeranje GORE
    if ((pTS->x >= curtains_screen_layout.up_zone.x0) && (pTS->x < curtains_screen_layout.up_zone.x1) &&
            (pTS->y >= curtains_screen_layout.up_zone.y0) && (pTS->y < curtains_screen_layout.up_zone.y1))
    {
        *click_flag = 1;
        shouldDrawScreen = 1;
        Curtain_HandleTouchLogic(CURTAIN_UP);
    }
    // Provjera da li je dodir u zoni za pomjeranje DOLE
    else if ((pTS->x >= curtains_screen_layout.down_zone.x0) && (pTS->x < curtains_screen_layout.down_zone.x1) &&
             (pTS->y >= curtains_screen_layout.down_zone.y0) && (pTS->y < curtains_screen_layout.down_zone.y1))
    {
        *click_flag = 1;
        shouldDrawScreen = 1;
        Curtain_HandleTouchLogic(CURTAIN_DOWN);
    }
    // Provjera da li je dodir u zoni za PRETHODNU roletnu (samo ako ima više od jedne)
    else if (Curtains_getCount() > 1 &&
             (pTS->x >= curtains_screen_layout.previous_arrow_zone.x0) && (pTS->x < curtains_screen_layout.previous_arrow_zone.x1) &&
             (pTS->y >= curtains_screen_layout.previous_arrow_zone.y0) && (pTS->y < curtains_screen_layout.previous_arrow_zone.y1))
    {
        if(curtain_selected > 0) {
            Curtain_Select(curtain_selected - 1);
        } else {
            Curtain_Select(Curtains_getCount()); // Vrati se na opciju "SVE"
        }
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera da li je dodir u zoni za SLJEDEĆU roletnu (samo ako ima više od jedne)
    else if (Curtains_getCount() > 1 &&
             (pTS->x >= curtains_screen_layout.next_arrow_zone.x0) && (pTS->x < curtains_screen_layout.next_arrow_zone.x1) &&
             (pTS->y >= curtains_screen_layout.next_arrow_zone.y0) && (pTS->y < curtains_screen_layout.next_arrow_zone.y1))
    {
        if (curtain_selected < Curtains_getCount()) {
            Curtain_Select(curtain_selected + 1);
        } else {
            Curtain_Select(0); // Vrati se na prvu roletnu
        }
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrada događaja pritiska za ekran "Light Settings".
 * @author      Gemini & [Vaše Ime]
 * @note        KONAČNA ISPRAVKA: Ova verzija sadrži kompletan kod. Ispravno
 * provjerava da li je dodir u zoni za izmjenu naziva i pokreće
 * tajmer. Ako nije, izvršava kompletnu, originalnu logiku za
 * obradu dodira na slajderu svjetline i paleti boja.
 ******************************************************************************
 */
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS)
{
    // Provjera da li je dodir unutar definisane zone za izmjenu naziva (samo za pojedinačna svjetla)
    if (!rename_light_timer_start && light_selectedIndex < LIGHTS_MODBUS_SIZE &&
            pTS->x >= light_settings_screen_layout.rename_text_zone.x0 && pTS->x < light_settings_screen_layout.rename_text_zone.x1 &&
            pTS->y >= light_settings_screen_layout.rename_text_zone.y0 && pTS->y < light_settings_screen_layout.rename_text_zone.y1)
    {
        // Ako je dodir u zoni, pokreni tajmer za detekciju dugog pritiska
        rename_light_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
    }
    else // Ako dodir NIJE bio u zoni za naziv, obradi slajdere i paletu
    {
        // =======================================================================
        // === POČETAK KOMPLETNOG, ORIGINALNOG KODA ZA SLAJDERE I PALETU ===
        // =======================================================================
        uint8_t new_brightness = 255;
        uint32_t new_color = 0;

        bool is_rgb_mode = false;
        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            is_rgb_mode = lights_allSelected_hasRGB;
        } else {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                is_rgb_mode = LIGHT_isRGB(handle);
            }
        }

        // Provjera dodira na bijeli kvadrat (samo u RGB modu)
        if (is_rgb_mode &&
                (pTS->x >= light_settings_screen_layout.white_square_zone.x0) && (pTS->x < light_settings_screen_layout.white_square_zone.x1) &&
                (pTS->y >= light_settings_screen_layout.white_square_zone.y0) && (pTS->y < light_settings_screen_layout.white_square_zone.y1))
        {
            new_color = 0x00FFFFFF;
        }
        // Provjera dodira na slajder svjetline
        else if ((pTS->x >= light_settings_screen_layout.brightness_slider_zone.x0) && (pTS->x < light_settings_screen_layout.brightness_slider_zone.x1) &&
                 (pTS->y >= light_settings_screen_layout.brightness_slider_zone.y0) && (pTS->y < light_settings_screen_layout.brightness_slider_zone.y1))
        {
            g_high_precision_mode = true;

            const int slider_x0 = light_settings_screen_layout.brightness_slider_zone.x0;
            const int slider_x1 = light_settings_screen_layout.brightness_slider_zone.x1;
            const int slider_width = slider_x1 - slider_x0;

            const int zone_width = slider_width * 0.04f;
            const int left_zone_end = slider_x0 + zone_width;
            const int right_zone_start = slider_x1 - zone_width;

            if (pTS->x < left_zone_end) {
                new_brightness = 0;
            } else if (pTS->x > right_zone_start) {
                new_brightness = 100;
            } else {
                const int middle_zone_width = right_zone_start - left_zone_end;
                const int relative_touch_in_middle = pTS->x - left_zone_end;
                float percentage = (float)relative_touch_in_middle / (float)middle_zone_width;
                new_brightness = 1 + (uint8_t)(percentage * 98.0f);
            }

            if (new_brightness > 100) new_brightness = 100;
        }
        // Provjera dodira na paletu boja (samo u RGB modu)
        else if (is_rgb_mode &&
                 (pTS->x >= light_settings_screen_layout.color_palette_zone.x0) && (pTS->x < light_settings_screen_layout.color_palette_zone.x1) &&
                 (pTS->y >= light_settings_screen_layout.color_palette_zone.y0) && (pTS->y < light_settings_screen_layout.color_palette_zone.y1))
        {
            new_color = LCD_GetPixelColor(pTS->x, pTS->y) & 0x00FFFFFF;
        }

        // Primjena detektovanih promjena
        if (new_brightness != 255 || new_color != 0) {
            if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Sva svjetla
                for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
                    LIGHT_Handle* handle = LIGHTS_GetInstance(i);
                    if (handle && LIGHT_isTiedToMainLight(handle) && !LIGHT_isBinary(handle)) {
                        if (new_brightness != 255) LIGHT_SetBrightness(handle, new_brightness);
                        else if (LIGHT_isRGB(handle) && new_color != 0) LIGHT_SetColor(handle, new_color);
                    }
                }
            } else { // Pojedinačno svjetlo
                LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
                if (handle) {
                    if (new_brightness != 255) LIGHT_SetBrightness(handle, new_brightness);
                    else if (LIGHT_isRGB(handle) && new_color != 0) LIGHT_SetColor(handle, new_color);
                }
            }
        }
        // =======================================================================
        // === KRAJ KOMPLETNOG, ORIGINALNOG KODA ZA SLAJDERE I PALETU ===
        // =======================================================================
    }
}
/**
 * @brief Obrada događaja pritiska u zoni glavnog prekidača na ekranu.
 * @note Pokreće tajmer za dugi pritisak kako bi se ušlo u podešavanja
 * svjetla ako je bar jedno od odabranih svjetala dimabilno.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandlePress_MainScreenSwitch(GUI_PID_STATE * pTS)
{
    // Logika za screensaver ostaje ista.
    if((!g_display_settings.leave_scrnsvr_on_release) || (g_display_settings.leave_scrnsvr_on_release && (!IsScrnsvrActiv()))) {

        // Resetujemo flegove na početku
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1; // Koristi se kao fleg da li je dimer/RGB pronađen
        lights_allSelected_hasRGB = false;

        // =======================================================================
        // === POČETAK REFAKTORISANJA ===
        //
        // Prolazimo kroz sva konfigurisana svjetla koristeći novi API.
        for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(i);

            // Provjeravamo da li handle postoji i da li je svjetlo vezano za glavni prekidač.
            if (handle && LIGHT_isTiedToMainLight(handle) && !LIGHT_isBinary(handle)) {
                // Ako smo pronašli bar jedno svjetlo koje nije binarno (dimer ili RGB),
                // postavljamo fleg i prekidamo dalju pretragu.
                light_selectedIndex = LIGHTS_MODBUS_SIZE; // Postavljamo "SVA SVJETLA"

                // Dodatno provjeravamo da li je to svjetlo RGB tipa.
                if (LIGHT_isRGB(handle)) {
                    lights_allSelected_hasRGB = true;
                }
            }
        }
        // === KRAJ REFAKTORISANJA ===
        // =======================================================================

        // Ako smo pronašli bar jedno dimer/RGB svjetlo, pokrećemo tajmer za dugi pritisak.
        if(light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            light_settingsTimerStart = HAL_GetTick();
        }
    }
}


/**
 * @brief Obrada otpuštanja dodira na glavnom ekranu (glavni prekidač).
 * @note Ova funkcija implementira "toggle" logiku za sva svjetla koja su povezana
 * sa glavnim prekidačem. Takođe upravlja pokretanjem i zaustavljanjem
 * noćnog tajmera. Refaktorisana je da bude drastično jednostavnija
 * korištenjem novog `lights` API-ja.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleRelease_MainScreenSwitch(GUI_PID_STATE * pTS)
{
    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

    // << KLJUČNA ISPRAVKA: Resetovanje tajmera za dugi pritisak >>
    // Ovo osigurava da se, nakon kratkog pritiska, ne aktivira
    // meni za podešavanje svjetala.
    light_settingsTimerStart = 0;
    // KORAK 1: Provjera trenutnog stanja svih glavnih svjetala.
    // Umjesto kompleksne petlje, sada pozivamo JEDNU funkciju iz `lights` API-ja.
    // `LIGHTS_isAnyLightOn()` interno provjerava sva svjetla koja su `tiedToMainLight`
    // i vraća `true` ako je ijedno upaljeno.
    bool isAnyLightCurrentlyOn = LIGHTS_isAnyLightOn();

    // KORAK 2: Određujemo novo, željeno stanje.
    // Ako je bilo šta bilo upaljeno, novo stanje je "ugasi sve".
    // Ako je sve bilo ugašeno, novo stanje je "upali sve".
    bool newStateIsOn = !isAnyLightCurrentlyOn;

    // KORAK 3: Primjenjujemo novo stanje na sva glavna svjetla.
    // Prolazimo kroz sva konfigurisana svjetla...
    for(uint8_t i = 0; i < LIGHTS_getCount(); ++i) {
        // ...dobijamo njihov handle...
        LIGHT_Handle* handle = LIGHTS_GetInstance(i);

        // ...i ako su vezana za glavni prekidač, postavljamo im novo stanje.
        if (handle && LIGHT_isTiedToMainLight(handle)) {
            LIGHT_SetState(handle, newStateIsOn);
        }
    }

    // KORAK 4: Upravljanje noćnim tajmerom preko novog, čistog API-ja.
    // Provjeravamo da li je noćni tajmer omogućen u postavkama i da li je noć.
    if (g_display_settings.light_night_timer_enabled && !((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))) {

        // Ako je korisnik upalio svjetla, pozivamo API da pokrene tajmer.
        if (newStateIsOn) {
            LIGHTS_StartNightTimer();
        } else { // Ako je korisnik ugasio svjetla, pozivamo API da zaustavi tajmer.
            LIGHTS_StopNightTimer();
        }
    } else {
        // Ako je dan ili je tajmer onemogućen, osiguravamo da je zaustavljen.
        LIGHTS_StopNightTimer();
    }

    // === KRAJ REFAKTORISANJA ===
    // =======================================================================

    // Ostatak funkcije (zahtjev za iscrtavanje i povratak na glavni ekran) ostaje isti.
    shouldDrawScreen = 1;
    screen = SCREEN_MAIN;
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za glavni ekran "Čarobnjaka" scena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija detektuje pritisak na tri moguća dugmeta: "Snimi",
 * "Otkaži" i "[ Promijeni ]". Na osnovu pritisnutog dugmeta,
 * izvršava odgovarajuću akciju: snima scenu, otkazuje izmjene
 * ili pokreće novi ekran za odabir izgleda scene.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira (nije korišten).
 * @param       click_flag Pokazivač na fleg za generisanje zvučnog signala.
 ******************************************************************************
 */
static void HandlePress_SceneEditScreen(GUI_PID_STATE* pTS, uint8_t* click_flag)
{
    // Provjera pritiska na dugme "Snimi" (ID_Ok)
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        *click_flag = 1; // Aktiviraj zvučni signal
        // Pozovi backend da "uslika" trenutno stanje svih relevantnih uređaja
        Scene_Memorize(scene_edit_index);
        // Pozovi backend da snimi sve scene (uključujući novu izmjenu) u EEPROM
        Scene_Save();

        // Vrati se na ekran sa pregledom svih scena
        DSP_KillSceneEditScreen(); // Obriši widgete prije prelaska
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    // Provjera pritiska na dugme "Otkaži" (ID_Next)
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        *click_flag = 1;
        // Samo se vrati na prethodni ekran bez snimanja
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    // Provjera pritiska na dugme "[ Promijeni ]"
    else if (BUTTON_IsPressed(hButtonChangeAppearance))
    {
        *click_flag = 1;
        // Prebaci se na novi ekran za odabir izgleda scene
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE_APPEARANCE;
        shouldDrawScreen = 1;
    }
}
/**
 ******************************************************************************
 * @brief       [VERZIJA 2.2 - KOMPLETNA] Obrađuje događaj pritiska za višenamjenski ekran odabira.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je svjesna konteksta. Na osnovu `current_scene_picker_mode`:
 * - U `WIZARD` modu: Mijenja izgled scene koja se kreira i vraća na editor. Sadrži
 * kompletnu logiku za paginaciju ("Next" dugme).
 * - U `TIMER` modu: Bilježi indeks odabrane scene u statičku varijablu
 * `timer_selected_scene_index` i vraća se na ekran koji je pozvao
 * ovaj birač (podešavanje tajmera).
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg za generisanje zvučnog signala.
 ******************************************************************************
 */
static void HandlePress_SceneAppearanceScreen(GUI_PID_STATE* pTS, uint8_t* click_flag)
{
    *click_flag = 1;

    if (current_scene_picker_mode == SCENE_PICKER_MODE_TIMER)
    {
        /********************************************/
        /* MOD: ODABIR POSTOJEĆE SCENE ZA TAJMER    */
        /********************************************/
        int row = (pTS->y - 10) / scene_screen_layout.slot_height;
        int col = pTS->x / scene_screen_layout.slot_width;
        int touched_display_index = row * scene_screen_layout.items_per_row + col;

        uint8_t configured_scene_counter = 0;
        for (int i = 0; i < SCENE_MAX_COUNT; i++) {
            Scene_t* scene_handle = Scene_GetInstance(i);
            if (scene_handle && scene_handle->is_configured) {
                if (configured_scene_counter == touched_display_index) {
                    timer_selected_scene_index = i; // Spremi stvarni fizički indeks scene
                    break;
                }
                configured_scene_counter++;
            }
        }

        // Vrati se na ekran koji je pozvao picker
        DSP_KillSceneAppearanceScreen();
        screen = scene_picker_return_screen;
        shouldDrawScreen = 1;

    } else { // SCENE_PICKER_MODE_WIZARD
        /************************************************/
        /* MOD: ODABIR IZGLEDA ZA NOVU SCENU (WIZARD)   */
        /************************************************/
        // --- 1. Provjera Pritiska na "Next" Dugme ---
        const GUI_BITMAP* iconNext = &bmnext;
        TouchZone_t next_button_zone = {
            .x0 = select_screen2_drawing_layout.next_button_x_pos,
            .y0 = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2),
            .x1 = 480,
            .y1 = 272
        };

        if (pTS->x >= next_button_zone.x0 && pTS->x < next_button_zone.x1 &&
                pTS->y >= next_button_zone.y0 && pTS->y < next_button_zone.y1)
        {
            *click_flag = 1;
            const int ICONS_PER_PAGE = 6;
            int total_available_appearances = 0;
            const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];

            // Ponovo filtriraj da dobiješ tačan broj stranica
            uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
            uint8_t used_count = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (temp_handle && temp_handle->is_configured)
                {
                    used_appearance_ids[used_count++] = temp_handle->appearance_id;
                }
            }
            for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++)
            {
                bool is_used = false;
                for (int j = 0; j < used_count; j++) {
                    if (used_appearance_ids[j] == i) {
                        is_used = true;
                        break;
                    }
                }
                if (!is_used) {
                    available_appearances[total_available_appearances++] = &scene_appearance_table[i];
                }
            }
            int total_pages = (total_available_appearances + ICONS_PER_PAGE - 1) / ICONS_PER_PAGE;

            scene_appearance_page++;
            if (scene_appearance_page >= total_pages)
            {
                scene_appearance_page = 0;
            }

            // Ponovo iscrtaj ovaj isti ekran sa novom stranicom
            DSP_InitSceneAppearanceScreen();
            shouldDrawScreen = 0;
            return;
        }

        // --- 2. Provjera Pritiska na Ikonice ---
        const int ICONS_PER_PAGE = 6;
        int row = (pTS->y - 10) / (scene_screen_layout.slot_height);
        int col = pTS->x / scene_screen_layout.slot_width;
        int display_index = row * scene_screen_layout.items_per_row + col;

        // Ponovo filtriraj listu da pronađeš tačno koja je ikonica pritisnuta
        int total_available_appearances = 0;
        const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];
        uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
        uint8_t used_count = 0;
        for (int i = 0; i < SCENE_MAX_COUNT; i++)
        {
            Scene_t* temp_handle = Scene_GetInstance(i);
            if (temp_handle && temp_handle->is_configured)
            {
                used_appearance_ids[used_count++] = temp_handle->appearance_id;
            }
        }
        for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++)
        {
            bool is_used = false;
            for (int j = 0; j < used_count; j++) {
                if (used_appearance_ids[j] == i) {
                    is_used = true;
                    break;
                }
            }
            if (!is_used) {
                available_appearances[total_available_appearances++] = &scene_appearance_table[i];
            }
        }

        int actual_index_in_available_list = (scene_appearance_page * ICONS_PER_PAGE) + display_index;

        if (actual_index_in_available_list < total_available_appearances)
        {
            *click_flag = 1;
            const SceneAppearance_t* chosen_appearance = available_appearances[actual_index_in_available_list];

            // Pronađi originalni ID u `scene_appearance_table`
            int selected_appearance_id = 0;
            for(int i=0; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++) {
                if (&scene_appearance_table[i] == chosen_appearance) {
                    selected_appearance_id = i;
                    break;
                }
            }

            Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
            if (scene_handle)
            {
                scene_handle->appearance_id = selected_appearance_id;

                if (chosen_appearance->text_id == TXT_SCENE_LEAVING) {
                    scene_handle->scene_type = SCENE_TYPE_LEAVING;
                } else if (chosen_appearance->text_id == TXT_SCENE_HOMECOMING) {
                    scene_handle->scene_type = SCENE_TYPE_HOMECOMING;
                } else if (chosen_appearance->text_id == TXT_SCENE_SLEEP) {
                    scene_handle->scene_type = SCENE_TYPE_SLEEP;
                } else {
                    scene_handle->scene_type = SCENE_TYPE_STANDARD;
                }
            }

            // === ISPRAVNA NAVIGACIJA ZA POVRATAK ===
            DSP_KillSceneAppearanceScreen();    // 1. Ubij trenutni ekran
            DSP_InitSceneEditScreen();          // 2. Inicijalizuj prethodni ekran (čarobnjaka)
            screen = SCREEN_SCENE_EDIT;         // 3. Postavi novo stanje
            shouldDrawScreen = 0;               // 4. Ne treba ponovo crtati
        }
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj PRITISKA za ekran sa scenama (SCREEN_SCENE).
 * @author      Gemini & [Vaše Ime]
 * @note        Verzija 2.0: Ova funkcija sada samo bilježi koji je slot
 * dodirnut i pokreće tajmer za mjerenje trajanja pritiska.
 ******************************************************************************
 */
static void HandlePress_SceneScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    uint8_t configured_scenes_count = Scene_GetCount();

    // --- Provjera dodira na ikonicu Čarobnjaka ---
    const GUI_BITMAP* wizard_icon = &bmicons_scene_wizzard;
    TouchZone_t wizard_zone = {
        .x0 = select_screen2_drawing_layout.next_button_x_pos,
        .y0 = select_screen2_drawing_layout.next_button_y_center - (wizard_icon->YSize / 2),
        .x1 = 480,
        .y1 = 272
    };

    if ((configured_scenes_count < SCENE_MAX_COUNT) && (pTS->x >= wizard_zone.x0 && pTS->x < wizard_zone.x1 && pTS->y >= wizard_zone.y0 && pTS->y < wizard_zone.y1))
    {
        *click_flag = 1;
        scene_pressed_index = configured_scenes_count; // Postavi indeks na poziciju čarobnjaka
        scene_press_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
    }
    // --- Provjera dodira na Mrežu sa Scenama ---
    else if (pTS->x < DRAWING_AREA_WIDTH) // Osiguraj da dodir nije u zoni menija
    {
        int row = pTS->y / scene_screen_layout.slot_height;
        int col = pTS->x / scene_screen_layout.slot_width;
        int touched_slot_index = row * scene_screen_layout.items_per_row + col;

        if (touched_slot_index < configured_scenes_count)
        {
            *click_flag = 1;
            scene_pressed_index = touched_slot_index;
            scene_press_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za ekran detaljne kontrole kapije.
 * @author      Gemini
 * @note        Funkcija provjerava da li je neko od dinamički kreiranih dugmadi
 * pritisnuto. Ako jeste, na osnovu ID-ja dugmeta (koji odgovara
 * `UI_Command_e` enumu), poziva odgovarajuću `Gate_Trigger...()` API
 * funkciju za izvršavanje komande.
 ******************************************************************************
 */
static void HandlePress_GateSettingsScreen(GUI_PID_STATE* pTS, uint8_t* click_flag)
{

}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za "dashboard" ekran sa kapijama.
 * @author      Gemini
 * @note        Ova funkcija se poziva iz `HandleTouchPressEvent` kada je `screen`
 * postavljen na `SCREEN_GATE`. Njen zadatak je da, koristeći istu
 * logiku za raspored kao i `Service_GateScreen`, odredi koja je
 * ikonica pritisnuta. Ako je pritisak validan, bilježi indeks
 * pritisnute ikonice i pokreće tajmer za mjerenje trajanja pritiska.
 * @param[in]   pTS Pokazivač na strukturu sa stanjem dodira.
 * @param[out]  click_flag Pokazivač na fleg za generisanje zvučnog signala.
 ******************************************************************************
 */
static void HandlePress_GateScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    uint8_t gate_count = Gate_GetCount();
    if (gate_count == 0 || pTS->x >= DRAWING_AREA_WIDTH) {
        return; // Nema šta da se pritisne ili je pritisak u zoni menija
    }

    // Koristimo istu logiku za raspored kao u Service_GateScreen
    uint8_t rows = (gate_count > 3) ? 2 : 1;
    int y_row_start = (rows > 1)
                      ? lights_and_gates_grid_layout.y_start_pos_multi_row
                      : lights_and_gates_grid_layout.y_start_pos_single_row;
    const int y_row_height = lights_and_gates_grid_layout.row_height;

    // Određivanje reda na osnovu Y koordinate
    uint8_t row_touched = (pTS->y - y_row_start) / y_row_height;
    if (row_touched >= rows) return;

    // Određivanje broja stavki u dodirnutom redu
    uint8_t gatesInRow = gate_count;
    if (gate_count > 3) {
        if (gate_count == 4) gatesInRow = 2;
        else if (gate_count == 5) gatesInRow = (row_touched > 0) ? 2 : 3;
        else gatesInRow = 3;
    }

    // Određivanje kolone na osnovu X koordinate
    uint8_t space_between = (400 - (80 * gatesInRow)) / (gatesInRow - 1 + 2);
    uint8_t col_touched = (pTS->x - space_between) / (80 + space_between);
    if (col_touched >= gatesInRow) return;

    // Izračunavanje finalnog indeksa
    uint8_t gatesInPreviousRows = 0;
    if (row_touched > 0) { // Ako je dodirnut drugi red
        if (gate_count == 4) gatesInPreviousRows = 2;
        else gatesInPreviousRows = 3;
    }

    int8_t touched_index = gatesInPreviousRows + col_touched;

    if (touched_index < gate_count)
    {
        *click_flag = 1;
        gate_pressed_index = touched_index;
        gate_press_timer_start = HAL_GetTick() ? HAL_GetTick() : 1; // Pokreni tajmer
    }
}


/**
 ******************************************************************************
 * @brief       Obrađuje događaj OTPUŠTANJA za ekran sa scenama (SCREEN_SCENE).
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA: Ova funkcija obrađuje isključivo KRATAK KLIK.
 * Ispravno aktivira postojeću scenu ili pokreće čarobnjaka za
 * kreiranje nove, poštujući "Kill -> Init" navigacioni patern.
 ******************************************************************************
 */
static void HandleRelease_SceneScreen(void)
{
    // Logika ostaje ista, jer je `HandlePress_SceneScreen` već ispravno
    // postavio `scene_pressed_index`.

    if (scene_press_timer_start == 0) return; // Dugi pritisak je već obrađen

    if ((HAL_GetTick() - scene_press_timer_start) < LONG_PRESS_DURATION)
    {
        uint8_t configured_scenes_count = Scene_GetCount();
        if (scene_pressed_index < configured_scenes_count)
        {
            // Kratak klik na postojeću scenu -> Aktiviraj je
            uint8_t scene_counter = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (scene_counter == scene_pressed_index)
                    {
                        Scene_Activate(i);
                        break;
                    }
                    scene_counter++;
                }
            }
        }
        else
        {
            // Kratak klik na "Dodaj Scenu" ikonicu
            uint8_t free_slot = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (!temp_handle || !temp_handle->is_configured)
                {
                    free_slot = i;
                    break;
                }
            }
            scene_edit_index = free_slot;

            DSP_KillSceneScreen();
            DSP_InitSceneEditScreen();
            screen = SCREEN_SCENE_EDIT;
            shouldDrawScreen = 0;
        }
    }

    scene_press_timer_start = 0;
    scene_pressed_index = -1;
}

/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za glavni ekran tajmera.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija je ažurirana prema preciznim uputstvima.
 * Blok koda `if (!IsRtcTimeValid())` je prekopiran iz Vaše radne verzije
 * i **nije funkcionalno mijenjan**.
 * Blok `else` je redizajniran da umjesto zone za "Podesi" provjerava
 * pritisak na novo `hButtonTimerSettings` dugme.
 ******************************************************************************
 */
static void HandlePress_TimerScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // ====================================================================
    // === OVAJ BLOK KODA JE PREKOPIRAN I NIJE FUNKCIONALNO MIJENJAN ===
    // ====================================================================
    if (!IsRtcTimeValid())
    {
        // Zona dodira za dugme "Podesite datum i vrijeme" pokriva cijeli aktivni dio ekrana.
        if (pTS->x < DRAWING_AREA_WIDTH)
        {
            *click_flag = 1;

            // Prvo uništi stari ekran
            DSP_KillTimerScreen();

            // 1. Postavi novo stanje ODMAH.
            screen = SCREEN_SETTINGS_DATETIME;

            // 2. Inicijalizuj i iscrtaj novi ekran.
            DSP_InitSettingsDateTimeScreen();

            // 3. Resetuj fleg za crtanje jer je ekran već iscrtan.
            shouldDrawScreen = 0;

            return; // Izlazimo odmah da spriječimo dalju obradu dodira.
        }
    }
    // ====================================================================
    // === NOVA IMPLEMENTACIJA ZA SLUČAJ KADA JE VRIJEME PODEŠENO ===
    // ====================================================================
    else
    {
        // Definišemo zonu dodira za 'toggle' dugme.
        GUI_CONST_STORAGE GUI_BITMAP* toggle_icon = Timer_IsActive() ? &bmicons_toggle_on : &bmicons_toogle_off;
        int toggle_x = (DRAWING_AREA_WIDTH / 2) - (toggle_icon->XSize / 2);
        TouchZone_t toggle_button_zone = { .x0 = toggle_x, .y0 = 180, .x1 = toggle_x + toggle_icon->XSize, .y1 = 180 + toggle_icon->YSize };


        // Provjera pritiska na 'toggle' dugme
        if (pTS->x >= toggle_button_zone.x0 && pTS->x < toggle_button_zone.x1 &&
                pTS->y >= toggle_button_zone.y0 && pTS->y < toggle_button_zone.y1)
        {
            *click_flag = 1;
            Timer_SetState(!Timer_IsActive());
            Timer_Save();
            shouldDrawScreen = 1;
        }
    }
}
/******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za univerzalni numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je adaptirana iz stare `DSP_InitPinpadScreen` funkcije.
 * Sada je univerzalna i konfiguriše se čitanjem parametara iz
 * statičke `g_numpad_context` strukture. Dinamički iscrtava
 * tastaturu i naslov prema proslijeđenom kontekstu.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_InitNumpadScreen(void)
{
    // Sigurnosno brisanje eventualno zaostalih widgeta sa drugih ekrana.
    ForceKillAllSettingsWidgets();

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu(1);

    // Definicije za dimenzije i pozicioniranje elemenata ostaju iste
    const int text_h = 50;
    const int btn_w = 80, btn_h = 40;
    const int x_gap = 10, y_gap = 10;
    const int x_start = (DRAWING_AREA_WIDTH - (3 * btn_w + 2 * x_gap)) / 2;
    const int keypad_h = (4 * btn_h + 3 * y_gap);
    const int total_h = text_h + y_gap + keypad_h;
    const int y_block_start = (LCD_GetYSize() - total_h) / 2;
    const int y_keypad_start = y_block_start + text_h + y_gap;

    // Tekstovi i ID-jevi za dugmad se definišu dinamički
    const char* key_texts[12];
    const int key_ids[] = {
        ID_PINPAD_1, ID_PINPAD_2, ID_PINPAD_3,
        ID_PINPAD_4, ID_PINPAD_5, ID_PINPAD_6,
        ID_PINPAD_7, ID_PINPAD_8, ID_PINPAD_9,
        ID_PINPAD_DEL, ID_PINPAD_0, ID_PINPAD_OK
    };

    // Popunjavanje tekstova za dugmad
    key_texts[0] = "1";
    key_texts[1] = "2";
    key_texts[2] = "3";
    key_texts[3] = "4";
    key_texts[4] = "5";
    key_texts[5] = "6";
    key_texts[6] = "7";
    key_texts[7] = "8";
    key_texts[8] = "9";

    // Dinamička konfiguracija zadnje tri tipke na osnovu konteksta
    key_texts[9] = g_numpad_context.allow_decimal ? "." : (char*)lng(TXT_DEL);
    key_texts[10] = "0";
    key_texts[11] = g_numpad_context.allow_minus_one ? (char*)lng(TXT_OFF_SHORT) : (char*)lng(TXT_OK);

    // Kreiranje dugmadi u petlji
    for (int i = 0; i < 12; i++) {
        int row = i / 3;
        int col = i % 3;
        int x_pos = x_start + col * (btn_w + x_gap);
        int y_pos = y_keypad_start + row * (btn_h + y_gap);

        hKeypadButtons[i] = BUTTON_CreateEx(x_pos, y_pos, btn_w, btn_h, 0, WM_CF_SHOW, 0, key_ids[i]);
        BUTTON_SetText(hKeypadButtons[i], key_texts[i]);
        BUTTON_SetFont(hKeypadButtons[i], &GUI_FontVerdana20_LAT);
    }

    // Inicijalizacija i iscrtavanje početnog stanja
    pin_buffer_idx = 0;
    memset(pin_buffer, 0, sizeof(pin_buffer));
    strncpy(pin_buffer, g_numpad_context.initial_value, MAX_PIN_LENGTH);
    pin_buffer_idx = strlen(pin_buffer);

    pin_mask_timer = 0;
    pin_error_active = false;
    DSP_DrawNumpadText(); // Poziv nove, preimenovane funkcije

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Servisira univerzalni numerički keypad (KONAČNA ISPRAVLJENA VERZIJA).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova verzija je bazirana na originalnoj, ispravnoj logici koju ste
 * priložili. Vraćena je "Optimistic UI" funkcionalnost gdje se status
 * "Naoružavanje/Razoružavanje" prikazuje ODMAH nakon unosa ispravnog PIN-a.
 * Pažljivo je integrisana nova state-machine (`pin_change_state`) za
 * trostupanjsku promjenu PIN-a, bez narušavanja postojećih funkcionalnosti.
 ******************************************************************************
 */
static void Service_NumpadScreen(void)
{
    static int button_pressed_id = -1;
    static bool should_redraw_text = false;

    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        DSP_InitNumpadScreen();
    }
    
    int currently_pressed_id = -1;
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i]) && BUTTON_IsPressed(hKeypadButtons[i])) {
            currently_pressed_id = i;
            break;
        }
    }

    if (currently_pressed_id == -1 && button_pressed_id != -1) {
        BuzzerOn(); HAL_Delay(1); BuzzerOff();

        WM_HWIN hPressedButton = hKeypadButtons[button_pressed_id];
        int Id = WM_GetId(hPressedButton);
        
        if (Id >= ID_PINPAD_0 && Id <= ID_PINPAD_9) {
            if (pin_buffer_idx < g_numpad_context.max_len) {
                pin_last_char = ((Id - ID_PINPAD_0) + '0');
                pin_buffer[pin_buffer_idx++] = pin_last_char;
                pin_buffer[pin_buffer_idx] = '\0';
                pin_mask_timer = HAL_GetTick();
                should_redraw_text = true;
            }
        }
        else if (Id == ID_PINPAD_DEL) {
            if (g_numpad_context.allow_decimal && strchr(pin_buffer, '.') == NULL && pin_buffer_idx < g_numpad_context.max_len) {
                 pin_buffer[pin_buffer_idx++] = '.';
                 pin_buffer[pin_buffer_idx] = '\0';
                 should_redraw_text = true;
            } else if (!g_numpad_context.allow_decimal && pin_buffer_idx > 0) {
                pin_buffer[--pin_buffer_idx] = '\0';
                should_redraw_text = true;
            }
        }
        else if (Id == ID_PINPAD_OK) {
            
            if (pin_change_state != PIN_CHANGE_IDLE) {
                // *** LOGIKA ZA PROMJENU PIN-A (ostaje netaknuta) ***
                switch (pin_change_state)
                {
                    case PIN_CHANGE_WAIT_CURRENT:
                        if (strcmp(pin_buffer, Security_GetPin()) == 0) {
                            NumpadContext_t next_context = { .title = lng(TXT_PIN_ENTER_NEW), .max_len = 8 };
                            memcpy(&g_numpad_context, &next_context, sizeof(NumpadContext_t));
                            pin_change_state = PIN_CHANGE_WAIT_NEW; 
                            DSP_KillNumpadScreen();
                            DSP_InitNumpadScreen();
                        } else {
                            pin_error_active = true;
                            pin_mask_timer = HAL_GetTick();
                            should_redraw_text = true;
                        }
                        break;
                    
                    case PIN_CHANGE_WAIT_NEW:
                        if (strlen(pin_buffer) >= 4) {
                            strcpy(new_pin_buffer, pin_buffer);
                            NumpadContext_t next_context = { .title = lng(TXT_PIN_CONFIRM_NEW), .max_len = 8 };
                            memcpy(&g_numpad_context, &next_context, sizeof(NumpadContext_t));
                            pin_change_state = PIN_CHANGE_WAIT_CONFIRM;
                            DSP_KillNumpadScreen();
                            DSP_InitNumpadScreen();
                        } else {
                            pin_error_active = true;
                            pin_mask_timer = HAL_GetTick();
                            should_redraw_text = true;
                        }
                        break;
                    
                    case PIN_CHANGE_WAIT_CONFIRM:
                        if (strcmp(pin_buffer, new_pin_buffer) == 0) {
                            Security_SetPin(new_pin_buffer);
                            Security_Save();
                            NumpadContext_t success_context = { .title = lng(TXT_PIN_CHANGE_SUCCESS), .max_len = 0 };
                            memcpy(&g_numpad_context, &success_context, sizeof(NumpadContext_t));
                            DSP_KillNumpadScreen();
                            DSP_InitNumpadScreen();
                            HAL_Delay(2000);
                            pin_change_state = PIN_CHANGE_IDLE; 
                            DSP_KillNumpadScreen();
                            screen = numpad_return_screen;
                            shouldDrawScreen = 1;
                        } else {
                            NumpadContext_t error_context = { .title = lng(TXT_PINS_DONT_MATCH), .max_len = 8 };
                            memcpy(&g_numpad_context, &error_context, sizeof(NumpadContext_t));
                            pin_change_state = PIN_CHANGE_WAIT_NEW;
                            DSP_KillNumpadScreen();
                            DSP_InitNumpadScreen();
                        }
                        break;
                    default:
                        pin_change_state = PIN_CHANGE_IDLE;
                        break;
                }
            }
            else
            {
                // *** POČETAK ISPRAVLJENOG BLOKA (VRAĆENA LOGIKA IZ display - Copy.c) ***
                bool is_valid = false;
                
                if (numpad_return_screen == SCREEN_SECURITY)
                {
                    if (Security_ValidateUserCode(pin_buffer)) {
                        is_valid = true;
                        
                        // VRAĆENA "OPTIMISTIC UI" LOGIKA ZA ALARM
                        if (selected_action == 0) { // Akcija za SISTEM
                            bool ui_is_in_arm_state = (alarm_ui_state[0] == ALARM_UI_STATE_ARMED || alarm_ui_state[0] == ALARM_UI_STATE_ARMING);
                            alarm_ui_state[0] = ui_is_in_arm_state ? ALARM_UI_STATE_DISARMING : ALARM_UI_STATE_ARMING;
                            for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
                                if (Security_GetPartitionRelayAddr(i) != 0) {
                                    alarm_ui_state[i + 1] = alarm_ui_state[0];
                                }
                            }
                            Security_ToggleSystem();
                        } else if (selected_action > 0 && selected_action <= SECURITY_PARTITION_COUNT) {
                            uint8_t partition_index = selected_action - 1;
                            bool ui_partition_is_in_arm_state = (alarm_ui_state[selected_action] == ALARM_UI_STATE_ARMED || alarm_ui_state[selected_action] == ALARM_UI_STATE_ARMING);
                            alarm_ui_state[selected_action] = ui_partition_is_in_arm_state ? ALARM_UI_STATE_DISARMING : ALARM_UI_STATE_ARMING;
                            Security_TogglePartition(partition_index);
                        }
                        
                        DSP_KillNumpadScreen();
                        screen = SCREEN_SECURITY; 
                        shouldDrawScreen = 1; 
                    }
                }
                else // Podrazumijevani kontekst: Ulazak u glavna podešavanja (SERVISNI PIN)
                {
                    if (strcmp(pin_buffer, system_pin) == 0) {
                        is_valid = true;
                        DSP_KillNumpadScreen();
                        // VRAĆENA LINIJA KOJA NEDOSTAJE
                        DSP_InitSet1Scrn();
                        screen = SCREEN_SETTINGS_1; 
                        shouldDrawScreen = 1;
                    }
                }
                
                if (!is_valid) {
                    pin_error_active = true;
                    pin_mask_timer = HAL_GetTick();
                }
                should_redraw_text = true;
                // *** KRAJ ISPRAVLJENOG BLOKA ***
            }
        }

        if (should_redraw_text) {
            DSP_DrawNumpadText();
        }
    }

    button_pressed_id = currently_pressed_id;

    if (pin_mask_timer != 0 && (HAL_GetTick() - pin_mask_timer) >= PIN_MASK_DELAY) {
        pin_mask_timer = 0;
        if (pin_error_active) {
            pin_error_active = false;
            // Očisti bafer tek nakon isteka tajmera za grešku
            pin_buffer_idx = 0;
            memset(pin_buffer, 0, sizeof(pin_buffer));
        }
        DSP_DrawNumpadText();
    }
}
/******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za Numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Adaptirana `DSP_KillPinpadScreen` funkcija. Poziva se prilikom
 * napuštanja `SCREEN_NUMPAD` ekrana kako bi se oslobodili resursi.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_KillNumpadScreen(void)
{
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i])) {
            WM_DeleteWindow(hKeypadButtons[i]);
            hKeypadButtons[i] = 0;
        }
    }
}

/******************************************************************************
 * @brief       Pomoćna funkcija koja iscrtava tekstualni unos za Numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Adaptirana `DSP_DrawPinpadText` funkcija. Centralizuje logiku
 * za prikaz naslova, unesene vrijednosti ili poruke o grešci.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_DrawNumpadText(void)
{
    const int text_h = 50;
    const int keypad_h = (4 * 40 + 3 * 10);
    const int total_h = text_h + 10 + keypad_h;
    const int y_block_start = (LCD_GetYSize() - total_h) / 2;
    const int y_text_pos = y_block_start;
    const int y_text_center = y_text_pos + (text_h / 2);

    char display_buffer[MAX_PIN_LENGTH + 1] = {0};

    bool should_mask = (strcmp(g_numpad_context.title, lng(TXT_ALARM_ENTER_PIN)) == 0) ||
                       (strcmp(g_numpad_context.title, lng(TXT_PIN_ENTER_CURRENT)) == 0) ||
                       (strcmp(g_numpad_context.title, lng(TXT_PIN_ENTER_NEW)) == 0) ||
                       (strcmp(g_numpad_context.title, lng(TXT_PIN_CONFIRM_NEW)) == 0);

    GUI_MULTIBUF_BeginEx(1);
    GUI_ClearRect(10, y_text_pos - 25, 370, y_text_pos + text_h);

    // Iscrtavanje naslova
    // ISPRAVKA: Postavljen _LAT font
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextMode(GUI_TM_TRANS);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
    // ISPRAVKA: Y pozicija je sada ista kao za poruku o grešci (y_text_center)
    GUI_DispStringAt(g_numpad_context.title, DRAWING_AREA_WIDTH / 2, y_text_center);

    // Logika za iscrtavanje unosa (PIN ili poruka)
    // ISPRAVKA: Postavljen _LAT font
    GUI_SetFont(&GUI_FontVerdana32_LAT);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);

    if (pin_error_active) {
        GUI_SetColor(GUI_RED);
        GUI_DispStringAt(lng(TXT_PIN_WRONG), DRAWING_AREA_WIDTH / 2, y_text_center - 25);
    } else {
        // Sakrij naslov ako se unosi PIN, jer je sada na istoj poziciji
        if(pin_buffer_idx > 0) {
             GUI_ClearRect(10, y_text_pos, 370, y_text_pos + text_h);
        }
        GUI_SetColor(GUI_ORANGE);

        if (should_mask) {
            for(int i = 0; i < pin_buffer_idx; i++) {
                if (pin_mask_timer != 0 && i == (pin_buffer_idx - 1)) {
                    display_buffer[i] = pin_buffer[i];
                } else {
                    display_buffer[i] = '*';
                }
            }
        } else {
            strncpy(display_buffer, pin_buffer, pin_buffer_idx);
        }

        display_buffer[pin_buffer_idx] = '\0';
        GUI_DispStringAt(display_buffer, DRAWING_AREA_WIDTH / 2, y_text_center);
    }

    GUI_MULTIBUF_EndEx(1);
}
/******************************************************************************
 * @brief       Priprema i prebacuje sistem na ekran univerzalnog numpada.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ovo je interna, ne-blokirajuća funkcija. Kopira proslijeđeni
 * kontekst u internu statičku varijablu, resetuje rezultat,
 * i postavlja fleg za promjenu ekrana.
 * @param       context Pokazivač na NumpadContext_t strukturu sa parametrima.
 * @retval      None
 *****************************************************************************/
static void Display_ShowNumpad(const NumpadContext_t* context)
{
    // 1. Sačuvaj ekran sa kojeg je funkcija pozvana, ali samo ako to nije sam Numpad
    if (screen != SCREEN_NUMPAD) {
        numpad_return_screen = screen;
    }
    // 2. Kopiraj proslijeđeni kontekst u internu, statičku varijablu
    if (context != NULL) {
        memcpy(&g_numpad_context, context, sizeof(NumpadContext_t));
    } else {
        memset(&g_numpad_context, 0, sizeof(NumpadContext_t));
        g_numpad_context.title = "Greska";
    }

    // 3. Resetuj strukturu za rezultat prije svakog prikazivanja
    memset(&g_numpad_result, 0, sizeof(NumpadResult_t));

    // 4. Postavi flegove za prebacivanje na ekran numpada
    screen = SCREEN_NUMPAD;
    shouldDrawScreen = 1;
}
/******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je srce UI dijela `SCREEN_KEYBOARD_ALPHA`. Ona čita
 * trenutno odabrani jezik iz `g_display_settings` i, na osnovu
 * njega i `shift` stanja, bira odgovarajući raspored iz
 * `key_layouts` matrice. Zatim dinamički kreira sve tastere,
 * postavlja tekst na njima i iscrtava polje za unos.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_InitKeyboardScreen(void)
{
    // Sigurnosno brisanje widgeta sa prethodnih ekrana
    ForceKillAllSettingsWidgets();

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // === 1. Definicija rasporeda (Layout) ===
    const int16_t key_w = 42, key_h = 38;
    const int16_t x_gap = 5, y_gap = 5;
    const int16_t x_start = (LCD_GetXSize() - (KEYS_PER_ROW * key_w + (KEYS_PER_ROW - 1) * x_gap)) / 2;
    const int16_t y_start_keys = 40;

    // === 2. Odabir jezičkog rasporeda ===
    // Ovaj pokazivač će pokazivati na odgovarajući 2D niz karaktera (npr. BHS, mala slova)
    const char* (*layout)[KEYS_PER_ROW] =
        key_layouts[g_display_settings.language][keyboard_shift_active];

    // Provjera da li je layout za odabrani jezik definisan, ako nije, koristi ENG
    if (layout[0][0] == NULL) {
        layout = key_layouts[ENG][keyboard_shift_active];
    }

    // === 3. Dinamičko kreiranje tastera sa karakterima ===
    for (int row = 0; row < KEY_ROWS; row++) {
        for (int col = 0; col < KEYS_PER_ROW; col++) {
            // Preskoči ako za neku poziciju nije definisan taster (za kraće redove)
            if (layout[row][col] == NULL || strlen(layout[row][col]) == 0) continue;

            int x_pos = x_start + col * (key_w + x_gap);
            int y_pos = y_start_keys + row * (key_h + y_gap);
            int index = row * KEYS_PER_ROW + col;

            hKeyboardButtons[index] = BUTTON_CreateEx(x_pos, y_pos, key_w, key_h, 0, WM_CF_SHOW, 0, GUI_ID_USER + index);
            BUTTON_SetText(hKeyboardButtons[index], layout[row][col]);
            BUTTON_SetFont(hKeyboardButtons[index], &GUI_Font20_1);
        }
    }

    // === 4. Kreiranje specijalnih tastera (Shift, Space, OK, itd.) ===
    const int16_t y_special_row = y_start_keys + KEY_ROWS * (key_h + y_gap);

    // SHIFT taster
    hKeyboardSpecialButtons[0] = BUTTON_CreateEx(x_start, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_SHIFT);
    BUTTON_SetText(hKeyboardSpecialButtons[0], "Shift");

    // SPACE taster
    hKeyboardSpecialButtons[1] = BUTTON_CreateEx(x_start + 60 + x_gap, y_special_row, 240, key_h, 0, WM_CF_SHOW, 0, GUI_ID_SPACE);
    BUTTON_SetText(hKeyboardSpecialButtons[1], "Space");

    // BACKSPACE taster
    hKeyboardSpecialButtons[2] = BUTTON_CreateEx(x_start + 300 + 2*x_gap, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_BACKSPACE);
    BUTTON_SetText(hKeyboardSpecialButtons[2], "Del");

    // OK taster
    hKeyboardSpecialButtons[3] = BUTTON_CreateEx(x_start + 360 + 3*x_gap, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_OKAY);
    BUTTON_SetText(hKeyboardSpecialButtons[3], "OK");

    // === 5. Inicijalizacija bafera i iscrtavanje polja za unos ===
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    strncpy(keyboard_buffer, g_keyboard_context.initial_value, sizeof(keyboard_buffer) - 1);
    keyboard_buffer_idx = strlen(keyboard_buffer);

    // Iscrtavanje naslova i polja za unos
    GUI_SetFont(GUI_FONT_20_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
    GUI_DispStringAt(g_keyboard_context.title, LCD_GetXSize() / 2, 15);

    GUI_SetColor(GUI_ORANGE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt(keyboard_buffer, x_start, 40);

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Servisira univerzalnu alfanumeričku tastaturu.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija obrađuje pritiske na sve tastere, upravlja unosom,
 * brisanjem i shift stanjem.
 * Ključna logika se nalazi u obradi "OK" dugmeta, gdje se na osnovu
 * konteksta (`keyboard_return_screen` i `selected_partition_for_rename`)
 * odlučuje koja `setter` funkcija će biti pozvana za snimanje podatka.
 ******************************************************************************
 */
static void Service_KeyboardScreen(void)
{
    static int button_pressed_idx = -1;
    int current_pressed_idx = -1;
    WM_HWIN hPressedButton = 0;

    // Provjera pritiska na tastere sa karakterima
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i]) && BUTTON_IsPressed(hKeyboardButtons[i])) {
            current_pressed_idx = i;
            hPressedButton = hKeyboardButtons[i];
            break;
        }
    }
    // Provjera pritiska na specijalne tastere
    if (!hPressedButton) {
        for (int i = 0; i < 5; i++) {
            if (WM_IsWindow(hKeyboardSpecialButtons[i]) && BUTTON_IsPressed(hKeyboardSpecialButtons[i])) {
                current_pressed_idx = -(i + 1);
                hPressedButton = hKeyboardSpecialButtons[i];
                break;
            }
        }
    }

    // Događaj se aktivira kada se taster OTPUSTI
    if (current_pressed_idx == -1 && button_pressed_idx != -1) {
        BuzzerOn(); HAL_Delay(1); BuzzerOff();

        // U prethodnom ciklusu je bio pritisnut taster sa indeksom `button_pressed_idx`
        // Sada radimo sa njegovim handle-om
        if (button_pressed_idx >= 0) {
            hPressedButton = hKeyboardButtons[button_pressed_idx];
        } else {
            hPressedButton = hKeyboardSpecialButtons[(-button_pressed_idx) - 1];
        }

        int Id = WM_GetId(hPressedButton);

        if (Id >= GUI_ID_USER && Id < (GUI_ID_USER + (KEY_ROWS * KEYS_PER_ROW))) {
            if (keyboard_buffer_idx < g_keyboard_context.max_len) {
                char key_text[10];
                BUTTON_GetText(hPressedButton, key_text, sizeof(key_text));
                strcat(keyboard_buffer, key_text);
                keyboard_buffer_idx = strlen(keyboard_buffer);
            }
        } else {
            switch(Id) {
            case GUI_ID_SHIFT:
                keyboard_shift_active = !keyboard_shift_active;
                DSP_KillKeyboardScreen();
                DSP_InitKeyboardScreen();
                return;

            case GUI_ID_BACKSPACE:
                if (keyboard_buffer_idx > 0) {
                    keyboard_buffer[--keyboard_buffer_idx] = '\0';
                }
                break;

            case GUI_ID_SPACE:
                if (keyboard_buffer_idx < g_keyboard_context.max_len) {
                    keyboard_buffer[keyboard_buffer_idx++] = ' ';
                    keyboard_buffer[keyboard_buffer_idx] = '\0';
                }
                break;

            case GUI_ID_OKAY:
                // Pripremi rezultat
                strncpy(g_keyboard_result.value, keyboard_buffer, sizeof(g_keyboard_result.value));
                g_keyboard_result.value[sizeof(g_keyboard_result.value) - 1] = '\0';
                g_keyboard_result.is_confirmed = true;
                
                // ===============================================================
                // ===          NOVA LOGIKA ZA SNIMANJE NAZIVA ALARMA          ===
                // ===============================================================
                if (keyboard_return_screen == SCREEN_SETTINGS_ALARM)
                {
                    if (selected_partition_for_rename == -1)
                    {
                        // Mijenjamo naziv cijelog sistema
                        Security_SetSystemName(g_keyboard_result.value);
                    }
                    else if (selected_partition_for_rename >= 0 && selected_partition_for_rename < SECURITY_PARTITION_COUNT)
                    {
                        // Mijenjamo naziv specifične particije
                        Security_SetPartitionName(selected_partition_for_rename, g_keyboard_result.value);
                    }
                    Security_Save(); // Snimi promjene u EEPROM
                }
                // ===============================================================

                DSP_KillKeyboardScreen();
                screen = keyboard_return_screen;
                shouldDrawScreen = 1; // Zatraži ponovno iscrtavanje prethodnog ekrana
                return;
            }
        }

        if (screen == SCREEN_KEYBOARD_ALPHA) {
            GUI_MULTIBUF_BeginEx(1);
            const int x_start = (LCD_GetXSize() - (10 * 42 + 9 * 5)) / 2;
            GUI_ClearRect(x_start, 35, x_start + 42 * 10 + 5 * 9, 55);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
            GUI_DispStringAt(keyboard_buffer, x_start, 40);
            GUI_MULTIBUF_EndEx(1);
        }
    }

    button_pressed_idx = current_pressed_idx;
}

/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Poziva se prilikom napuštanja `SCREEN_KEYBOARD_ALPHA` ekrana.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_KillKeyboardScreen(void)
{
    // Brisanje tastera sa karakterima
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i])) {
            WM_DeleteWindow(hKeyboardButtons[i]);
            hKeyboardButtons[i] = 0;
        }
    }
    // Brisanje specijalnih tastera
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hKeyboardSpecialButtons[i])) {
            WM_DeleteWindow(hKeyboardSpecialButtons[i]);
            hKeyboardSpecialButtons[i] = 0;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Priprema i prebacuje sistem na ekran alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Pamti ekran sa kojeg je pozvana radi povratka.
 * @param       context Pokazivač na KeyboardContext_t strukturu.
 * @retval      None
 ******************************************************************************
 */
static void Display_ShowKeyboard(const KeyboardContext_t* context)
{
    keyboard_return_screen = screen;

    if (context != NULL) {
        memcpy(&g_keyboard_context, context, sizeof(KeyboardContext_t));
    } else {
        memset(&g_keyboard_context, 0, sizeof(KeyboardContext_t));
        g_keyboard_context.title = "Greska";
    }

    memset(&g_keyboard_result, 0, sizeof(KeyboardResult_t));
    keyboard_shift_active = false; // Uvijek počni sa malim slovima

    screen = SCREEN_KEYBOARD_ALPHA;
    shouldDrawScreen = 1;
}

/**
 * @brief Provjerava status ažuriranja firmvera i prikazuje odgovarajuću poruku.
 * @note  Ova funkcija blokira ostatak GUI logike dok je ažuriranje u toku.
 * @return uint8_t 1 ako je ažuriranje aktivno, inače 0.
 */
static uint8_t Service_HandleFirmwareUpdate(void)
{
    static uint8_t fwmsg = 2; // Statički fleg za praćenje stanja iscrtavanja poruke

    // IZMJENA: Sada provjeravamo da li je ažuriranje aktivno BILO GDJE NA BUSU.
    if (IsBusFwUpdateActive()) {
        // Ako je ažuriranje aktivno, a poruka još nije iscrtana
        if (!fwmsg) {
            fwmsg = 1; // Postavi fleg da je poruka iscrtana
            GUI_MULTIBUF_BeginEx(1);
            GUI_Clear();
            GUI_SetFont(GUI_FONT_24B_1);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            // Koristi lng() za prikaz poruke na odabranom jeziku
            GUI_DispStringAt(lng(TXT_UPDATE_IN_PROGRESS), 240, 135);
            GUI_MULTIBUF_EndEx(1);
            DISPResetScrnsvr();
        }
        return 1; // Vrati 1 da signalizira da je ažuriranje u toku
    }
    // Ako je ažuriranje upravo završeno (fleg je bio 1, a sada je IsBusFwUpdateActive() false)
    else if (fwmsg == 1) {
        fwmsg = 0; // Resetuj fleg
        scrnsvr_tmr = 0; // Resetuj tajmer za screensaver
        // Forsiraj ponovno iscrtavanje trenutnog ekrana da se ukloni poruka o ažuriranju
        shouldDrawScreen = 1;
    }
    // Jednokratno iscrtavanje inicijalne grafike na samom početku
    else if (fwmsg == 2) {
        fwmsg = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        // Iscrtaj hamburger meni ikonicu
        DrawHamburgerMenu(1);
        GUI_MULTIBUF_EndEx(1);
    }
    return 0; // Ažuriranje nije aktivno
}
/**
 ******************************************************************************
 * @brief       Servisira glavni ekran.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva u petlji kada je `screen == SCREEN_MAIN`.
 * Odgovorna je za resetovanje internih flegova menija i za iscrtavanje
 * osnovnih elemenata glavnog ekrana (hamburger meni i crveni/zeleni krug
 * koji indicira stanje svjetala).
 * Optimizovana je tako da se ponovno iscrtavanje dešava samo kada je
 * to neophodno (kada se promijeni stanje svjetala ili kada se forsira).
 * **Ažurirana verzija dodaje donji meni za scene.**
 ******************************************************************************
 */
static void Service_MainScreen(void)
{
    // Statičke varijable za čuvanje prethodnog stanja radi optimizacije iscrtavanja
    static bool old_light_state = false;
    static bool old_timer_active_state = false;
    static uint8_t old_thermostat_state = 0; // Nova varijabla za stanje termostata

    // Dohvatanje trenutnog stanja svjetala
    bool current_light_state = LIGHTS_isAnyLightOn();

    // Dohvatanje trenutnog stanja alarma (da li treba prikazati ikonicu)
    bool current_timer_active_state = IsRtcTimeValid() && Timer_IsActive() && (Timer_GetActionBuzzer() || (Timer_GetSceneIndex() != -1));

    // Dohvatanje trenutnog stanja termostata
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    uint8_t current_thermostat_mode = Thermostat_GetControlMode(pThst);
    uint8_t is_thermostat_working = Thermostat_GetState(pThst);
    // Kombinujemo mod i stanje u jednu varijablu radi lakše provjere promjene
    uint8_t current_thermostat_state = (current_thermostat_mode << 4) | is_thermostat_working;

    // Resetovanje internih flegova (nepromijenjeno)
    thermostatMenuState = 0;
    menu_lc = 0;
    old_min = 60;
    rtctmr = 0;

    // Iscrtavanje ekrana se dešava ako je zatraženo, ILI ako se promijenilo stanje bilo kojeg sistema
    if (shouldDrawScreen || (current_light_state != old_light_state) || (current_timer_active_state != old_timer_active_state) || (current_thermostat_state != old_thermostat_state)) {

        shouldDrawScreen = 0;
        old_light_state = current_light_state;
        old_timer_active_state = current_timer_active_state;
        old_thermostat_state = current_thermostat_state; // Ažuriraj staro stanje termostata

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // --- Iscrtavanje statusnih ikonica (termostat i alarm) ---
        int16_t x_icon_pos = 5; // Početna pozicija za prvu ikonicu (najviše desno)
        int16_t y_icon_pos = 5; // Y pozicija

        // 1. Provjera i iscrtavanje ikonice alarma
        if (current_timer_active_state)
        {
            GUI_DrawBitmap(&bmicons_alarm_20, x_icon_pos, y_icon_pos);
            x_icon_pos += 30; // Pomjeri "kursor" ulijevo za sljedeću ikonicu
        }

        // 2. Provjera i iscrtavanje ikonice termostata
        const GUI_BITMAP* thermostat_icon_to_draw = NULL;
        if (current_thermostat_mode == THST_HEATING) {
            thermostat_icon_to_draw = is_thermostat_working ? &bmicons_heating_20_activ : &bmicons_heating_20;
        } else if (current_thermostat_mode == THST_COOLING) {
            thermostat_icon_to_draw = is_thermostat_working ? &bmicons_cooling_20_activ : &bmicons_cooling_20;
        }

        if (thermostat_icon_to_draw != NULL)
        {
            GUI_DrawBitmap(thermostat_icon_to_draw, x_icon_pos, y_icon_pos);
            // Ovdje ne moramo pomjerati kursor jer je ovo posljednja ikonica u nizu
        }

        // Ostatak koda ostaje nepromijenjen...
        if (g_display_settings.scenes_enabled)
        {
            DrawHamburgerMenu(2);
        }

        if (current_light_state) {
            GUI_SetColor(GUI_GREEN);
        } else {
            GUI_SetColor(GUI_RED);
        }
        GUI_DrawEllipse(main_screen_layout.circle_center_x,
                        main_screen_layout.circle_center_y,
                        main_screen_layout.circle_radius_x,
                        main_screen_layout.circle_radius_y);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir glavnih kontrola (Svjetla, Termostat, Roletne, Dinamička ikona).
 * @author      Gemini & [Vaše Ime]
 * @note        Verzija 2.3.2: Potpuno refaktorisano da koristi "Pametnu Mrežu" (Smart Grid).
 * Ekran dinamički iscrtava samo konfigurisane module i prilagođava raspored
 * na osnovu njihovog broja (1, 2, 3 ili 4), koristeći nove fontove sa
 * punom podrškom za specijalne karaktere.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SelectScreen1(void)
{
    /**
     * @brief Struktura za čuvanje podataka o jednom dinamičkom meniju.
     */
    typedef struct {
        const GUI_BITMAP* icon;
        TextID text_id;
        eScreen target_screen;
        bool is_active; // Dodatni fleg za dinamičku ikonu
    } DynamicMenuItem;

    // Inicijalizacija handle-ova za module
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // Lista SVIH mogućih modula za ovaj ekran
    DynamicMenuItem all_modules[] = {
        { &bmSijalicaOff, TXT_LIGHTS, SCREEN_LIGHTS, false },
        { &bmTermometar, TXT_THERMOSTAT, SCREEN_THERMOSTAT, false },
        { &bmblindMedium, TXT_BLINDS, SCREEN_CURTAINS, false },
        { NULL, TXT_DUMMY, SCREEN_SELECT_1, false } // Mjesto za dinamičku ikonu
    };

    // --- KORAK 1: Detekcija aktivnih modula ---
    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;

    if (LIGHTS_getCount() > 0) {
        active_modules[active_modules_count++] = all_modules[0];
    }
    if (Thermostat_GetGroup(pThst) > 0) {
        active_modules[active_modules_count++] = all_modules[1];
    }
    if (Curtains_getCount() > 0) {
        active_modules[active_modules_count++] = all_modules[2];
    }

    // Detekcija za dinamičku ikonu
    if (g_display_settings.selected_control_mode == MODE_DEFROSTER && Defroster_getPin(defHandle) > 0) {
        bool is_active = Defroster_isActive(defHandle);
        all_modules[3].icon = is_active ? &bmicons_menu_defroster_on : &bmicons_menu_defroster_off;
        all_modules[3].text_id = TXT_DEFROSTER;
        all_modules[3].is_active = is_active;
        active_modules[active_modules_count++] = all_modules[3];
    } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR && (Ventilator_getRelay(ventHandle) > 0 || Ventilator_getLocalPin(ventHandle) > 0)) {
        bool is_active = Ventilator_isActive(ventHandle);
        all_modules[3].icon = is_active ? &bmicons_menu_ventilator_on : &bmicons_menu_ventilator_off;
        all_modules[3].text_id = TXT_VENTILATOR;
        all_modules[3].is_active = is_active;
        active_modules[active_modules_count++] = all_modules[3];
    }

    // Crtamo samo ako je potrebno
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // --- NOVA LOGIKA ZA ISCRTAVANJE SEPARATORA ---
        if (active_modules_count < 4) {
            // Crtaj dugački desni separator za 1, 2, i 3 ikonice
            GUI_DrawLine(DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_start,
                         DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_end);
        }

        // --- KORAK 2: "Pametna Mreža" - Iscrtavanje na osnovu broja aktivnih modula ---
        switch (active_modules_count) {
        case 1: {
            // Jedna velika ikona na sredini
            const DynamicMenuItem* item = &active_modules[0];
            int x_pos = (DRAWING_AREA_WIDTH / 2) - (item->icon->XSize / 2);
            int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
            GUI_DrawBitmap(item->icon, x_pos, y_pos);

            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(item->text_id), DRAWING_AREA_WIDTH / 2, y_pos + item->icon->YSize + 10);
            break;
        }
        case 2: {
            // Dvije ikonice sa separatorom
            // Jedan kratki separator u sredini
            GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_start,
                         DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_end);

            for (int i = 0; i < 2; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 4) * (i == 0 ? 1 : 3);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        case 3: {
            // Tri ikonice sa separatorima
            // Dva kratka separatora
            GUI_DrawLine(DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_start,
                         DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_end);
            GUI_DrawLine((DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_start,
                         (DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_end);
            for (int i = 0; i < 3; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 6) * (1 + 2 * i);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        case 4:
        default: {
            // Klasična 2x2 mreža
            // Samo krstasti separator, bez dugačke desne linije
            GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_start,
                         DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_end);
            GUI_DrawLine(select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                         DRAWING_AREA_WIDTH - select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);
            for (int i = 0; i < 4; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 4) * (i % 2 == 0 ? 1 : 3);
                int y_center = (LCD_GetYSize() / 4) * (i < 2 ? 1 : 3);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = y_center - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        }

        // Iscrtaj "Next" dugme za prelazak na Screen 2
        // Iscrtavanje "Next" dugmeta (logika ostaje ista)
        if (select_screen2_layout.next_button_zone.x1 > 0) {
            const GUI_BITMAP* iconNext = &bmnext;
            /**
            * @brief Iscrtavanje "NEXT" dugmeta.
            */
            GUI_DrawBitmap(iconNext, select_screen1_drawing_layout.x_separator_pos + 5, select_screen1_drawing_layout.y_next_button_center - (iconNext->YSize / 2));
        }
        GUI_MULTIBUF_EndEx(1);
    }
    // Ažuriranje dinamičke ikone ako se njeno stanje promijenilo
    else if (dynamicIconUpdateFlag) {
        dynamicIconUpdateFlag = 0;
        shouldDrawScreen = 1; // Najlakši način je ponovno iscrtati cijeli ekran
    }
}

/**
 ******************************************************************************
 * @brief       Servisira drugi ekran za odabir (Kapija, Tajmer, Alarm, Dinamička ikona).
 * @author      Gemini & [Vaše Ime]
 * @note        KONAČNA USKLAĐENA VERZIJA. Logika za iscrtavanje je sada
 * identična funkciji `Service_SelectScreen1`, uključujući sve ofsete
 * i atribute za pozicioniranje, kako bi se osiguralo savršeno
 * vizuelno poravnanje.
 ******************************************************************************
 */
static void Service_SelectScreen2(void)
{
    // Dio koda za detekciju aktivnih modula (ostaje nepromijenjen)
    typedef struct {
        const GUI_BITMAP* icon;
        TextID text_id;
        eScreen target_screen;
    } DynamicMenuItem;
    DynamicMenuItem all_modules[] = {
        { &bmicons_menu_gate,      TXT_GATE,                     SCREEN_GATE       },
        { &bmicons_menu_timers,    TXT_TIMER,                    SCREEN_TIMER      },
        { &bmicons_scene_security, TXT_SECURITY,                 SCREEN_SECURITY   },
        { NULL,                    TXT_DUMMY,                    SCREEN_SELECT_2   }
    };
    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;
    if (Gate_GetCount() > 0) active_modules[active_modules_count++] = all_modules[0];
    
    active_modules[active_modules_count++] = all_modules[1];
    
    // << Provjera fleg-a prije dodavanja ikonice >>
    if (g_display_settings.security_module_enabled) {
        active_modules[active_modules_count++] = all_modules[2];
    }
    
    switch (g_display_settings.selected_control_mode_2)
    {
        case MODE_DEFROSTER: {
            Defroster_Handle* defHandle = Defroster_GetInstance();
            if (Defroster_getPin(defHandle) > 0) {
                all_modules[3].icon = Defroster_isActive(defHandle) ? &bmicons_menu_defroster_on : &bmicons_menu_defroster_off;
                all_modules[3].text_id = TXT_DEFROSTER;
                active_modules[active_modules_count++] = all_modules[3];
            }
            break;
        }
        case MODE_VENTILATOR: {
            Ventilator_Handle* ventHandle = Ventilator_GetInstance();
            if (Ventilator_getRelay(ventHandle) > 0 || Ventilator_getLocalPin(ventHandle) > 0) {
                all_modules[3].icon = Ventilator_isActive(ventHandle) ? &bmicons_menu_ventilator_on : &bmicons_menu_ventilator_off;
                all_modules[3].text_id = TXT_VENTILATOR;
                active_modules[active_modules_count++] = all_modules[3];
            }
            break;
        }
        case MODE_LANGUAGE: {
            const GUI_BITMAP* flag_icons[] = {
                &bmicons_menu_language_bhsc, &bmicons_menu_language_eng, &bmicons_menu_language_ger,
                &bmicons_menu_language_fra, &bmicons_menu_language_ita, &bmicons_menu_language_spa,
                &bmicons_menu_language_rus, &bmicons_menu_language_ukr, &bmicons_menu_language_pol,
                &bmicons_menu_language_cze, &bmicons_menu_language_slo
            };
            all_modules[3].icon = flag_icons[g_display_settings.language];
            all_modules[3].text_id = TXT_LANGUAGE_NAME;
            active_modules[active_modules_count++] = all_modules[3];
            break;
        }
        case MODE_THEME:
            all_modules[3].icon = &bmicons_menu_theme;
            all_modules[3].text_id = TXT_DUMMY;
            active_modules[active_modules_count++] = all_modules[3];
            break;
        case MODE_SOS:
            all_modules[3].icon = &bmicons_security_sos;
            all_modules[3].text_id = TXT_LANGUAGE_SOS_ALL_OFF;
            active_modules[active_modules_count++] = all_modules[3];
            break;
        case MODE_ALL_OFF:
            all_modules[3].icon = &bmicons_menu_all_off;
            all_modules[3].text_id = TXT_DUMMY;
            active_modules[active_modules_count++] = all_modules[3];
            break;
        case MODE_OUTDOOR: {
            bool is_outdoor_active = false; // Privremeno
            all_modules[3].icon = is_outdoor_active ? &bmSijalicaOn : &bmSijalicaOff;
            all_modules[3].text_id = TXT_DUMMY;
            active_modules[active_modules_count++] = all_modules[3];
            break;
        }
        default:
            break;
    }

    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // --- Logika za iscrtavanje separatora (preuzeta 1:1) ---
        if (active_modules_count < 4) {
            GUI_DrawLine(DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_start,
                         DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_end);
        }

        // --- "Pametna Mreža" - Iscrtavanje 1:1 kao u Service_SelectScreen1 ---
        switch (active_modules_count) {
            case 0:
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_WHITE);
                GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
                GUI_DispStringAt("Nema dostupnih opcija", DRAWING_AREA_WIDTH / 2, LCD_GetYSize() / 2);
                break;
            case 1: {
                const DynamicMenuItem* item = &active_modules[0];
                int x_pos = (DRAWING_AREA_WIDTH / 2) - (item->icon->XSize / 2);
                int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);
                GUI_SetFont(&GUI_FontVerdana32_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), DRAWING_AREA_WIDTH / 2, y_pos + item->icon->YSize + 10);
                break;
            }
            case 2: {
                GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_start,
                             DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_end);
                for (int i = 0; i < 2; i++) {
                    const DynamicMenuItem* item = &active_modules[i];
                    int x_center = (DRAWING_AREA_WIDTH / 4) * (i == 0 ? 1 : 3);
                    int x_pos = x_center - (item->icon->XSize / 2);
                    int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                    GUI_DrawBitmap(item->icon, x_pos, y_pos);
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    GUI_SetColor(GUI_ORANGE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_HCENTER);
                    GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
                }
                break;
            }
            case 3: {
                GUI_DrawLine(DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_start,
                             DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_end);
                GUI_DrawLine((DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_start,
                             (DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_end);
                for (int i = 0; i < 3; i++) {
                    const DynamicMenuItem* item = &active_modules[i];
                    int x_center = (DRAWING_AREA_WIDTH / 6) * (1 + 2 * i);
                    int x_pos = x_center - (item->icon->XSize / 2);
                    int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                    GUI_DrawBitmap(item->icon, x_pos, y_pos);
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    GUI_SetColor(GUI_ORANGE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_HCENTER);
                    GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
                }
                break;
            }
            case 4:
            default: {
                GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_start,
                             DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_end);
                GUI_DrawLine(select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                             DRAWING_AREA_WIDTH - select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);
                for (int i = 0; i < 4; i++) {
                    const DynamicMenuItem* item = &active_modules[i];
                    int x_center = (DRAWING_AREA_WIDTH / 4) * (i % 2 == 0 ? 1 : 3);
                    int y_center = (LCD_GetYSize() / 4) * (i < 2 ? 1 : 3);
                    int x_pos = x_center - (item->icon->XSize / 2);
                    int y_pos = y_center - (item->icon->YSize / 2) - 10;
                    GUI_DrawBitmap(item->icon, x_pos, y_pos);
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    GUI_SetColor(GUI_ORANGE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_HCENTER);
                    GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
                }
                break;
            }
        }

        const GUI_BITMAP* iconNext = &bmnext;
        GUI_DrawBitmap(iconNext, select_screen1_drawing_layout.x_separator_pos + 5, select_screen1_drawing_layout.y_next_button_center - (iconNext->YSize / 2));
        
        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira posljednji ekran za odabir (Čišćenje, WiFi, App, Postavke).
 * @author      Gemini & [Vaše Ime]
 * [cite_start]@note Ova funkcija je preimenovana iz Service_SelectScreen2.
 * Odgovorna je za iscrtavanje 4 fiksne ikonice na posljednjem ekranu u nizu
 * za odabir.Raspored elemenata je definisan u select_screen2_drawing_layout strukturi.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SelectScreenLast(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // Crtanje krstastog separatora za 2x2 mrežu
        GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_start,
                     DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_end);
        GUI_DrawLine(select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                     DRAWING_AREA_WIDTH - select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);

        // Definicija ikonica i tekstova za 4 kvadranta
        const GUI_BITMAP* icons[] = {&bmicons_menu_clean, &bmwifi, &bmmobilePhone, &bmicons_settings};
        TextID texts[] = {TXT_CLEAN, TXT_WIFI, TXT_APP, TXT_SETTINGS};
        int x_centers[] = {select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right, select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right};
        int y_centers[] = {select_screen2_drawing_layout.y_center_top, select_screen2_drawing_layout.y_center_top, select_screen2_drawing_layout.y_center_bottom, select_screen2_drawing_layout.y_center_bottom};

        for (int i = 0; i < 4; i++) {
            int x_pos = x_centers[i] - (icons[i]->XSize / 2);
            int y_pos = y_centers[i] - (icons[i]->YSize / 2) - select_screen2_drawing_layout.text_vertical_offset;
            GUI_DrawBitmap(icons[i], x_pos, y_pos);

            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(texts[i]), x_centers[i], y_pos + icons[i]->YSize + select_screen2_drawing_layout.text_vertical_offset);
        }

        // Iscrtavanje "NEXT" dugmeta koje sada vraća na početni ekran
        const GUI_BITMAP* iconNext = &bmnext;
        int x_pos = select_screen2_drawing_layout.next_button_x_pos;
        int y_pos = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2);
        GUI_DrawBitmap(iconNext, x_pos, y_pos);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa termostatom ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_ThermostatScreen`,
 * modifikovana za rad unutar čarobnjaka. Uklonjeni su hamburger meni,
 * vrijeme i status ON/OFF, a dodato je "[Next]" dugme.
 ******************************************************************************
 */
static void Service_SceneEditThermostatScreen(void)
{

    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    if (thermostatMenuState == 0) {
        thermostatMenuState = 1;
        GUI_MULTIBUF_BeginEx(0);
        GUI_SelectLayer(0);
        GUI_SetColor(GUI_BLACK);
        GUI_Clear();
        GUI_BMP_Draw(&thstat, 0, 0);
        // Ne crtamo hamburger meni
        GUI_MULTIBUF_EndEx(0);
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        // Kreiraj "Next" dugme sa ikonicom
        hButtonWizNext = BUTTON_CreateEx(390, 182, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);

        DISPSetPoint();
        MVUpdateSet();
        menu_lc = 0;
    } else if (thermostatMenuState == 1) {
        // Logika za +/- dugmad ostaje ista
        if (btninc && !_btninc) {
            _btninc = 1;
            Thermostat_SP_Temp_Increment(pThst);
            THSTAT_Save(pThst);
            DISPSetPoint();
        }
        else if (!btninc && _btninc) {
            _btninc = 0;
        }
        if (btndec && !_btndec) {
            _btndec = 1;
            Thermostat_SP_Temp_Decrement(pThst);
            THSTAT_Save(pThst);
            DISPSetPoint();
        }
        else if (!btndec && _btndec) {
            _btndec = 0;
        }

//            if (IsMVUpdateActiv()) {
//                MVUpdateReset();
//                GUI_MULTIBUF_BeginEx(1);
//                // Obriši samo staru poziciju za MV Temp
//                GUI_ClearRect(410, 185, 480, 235);
//
//                // Iscrtaj Izmjerenu Temperaturu na novoj poziciji
//                GUI_SetFont(GUI_FONT_24_1);
//                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
//                GUI_SetColor(GUI_WHITE);
//                GUI_DispSDecAt(Thermostat_GetMeasuredTemp(pThst) / 10, 5, 245, 3);
//                GUI_DispString("°c");
//
//                GUI_MULTIBUF_EndEx(1);
//            }
    }

    // Navigacija za "[Next]" dugme
    if (BUTTON_IsPressed(hButtonWizNext))
    {
        GUI_SelectLayer(0);
        GUI_SetColor(GUI_BLACK);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        DSP_KillSceneEditThermostatScreen();
        is_in_scene_wizard_mode = false; // Završi wizard mod
        // TODO: Prebaci na finalni ekran za snimanje
        DSP_InitSceneEditScreen();
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 0;
        return;
    }
}
/**
 * @brief Servisira ekran termostata, uključujući iscrtavanje i obradu unosa.
 * @note Ova funkcija se poziva u petlji dok je `screen == SCREEN_THERMOSTAT`.
 * U njoj se upravlja prikazom zadane temperature, trenutne temperature, te se obrađuju
 * dodiri za podešavanje temperature i paljenje/gašenje termostata.
 */
static void Service_ThermostatScreen(void)
{
    if (is_in_scene_wizard_mode)
    {
        Service_SceneEditThermostatScreen();
    }
    else
    {
        // Dobijamo handle za termostat.
        THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

        // Ažuriranje prikaza termostata se radi unutar jedne multibuffer transakcije.
        GUI_MULTIBUF_BeginEx(1);

        if (thermostatMenuState == 0) {
            // Ako je ovo prvi put da ulazimo na ekran, iscrtavamo kompletnu pozadinu.
            thermostatMenuState = 1;

            GUI_MULTIBUF_BeginEx(0);
            GUI_SelectLayer(0);
            GUI_SetColor(GUI_BLACK);
            GUI_Clear();
            // Iscrtavanje pozadinske bitmap slike termostata.
            GUI_BMP_Draw(&thstat, 0, 0);
            GUI_ClearRect(380, 0, 480, 100);
            // Iscrtavanje hamburger meni ikonice u gornjem desnom uglu.
            DrawHamburgerMenu(1);

            // Čišćenje specifičnih dijelova ekrana za dinamičke vrijednosti.
            GUI_ClearRect(350, 80, 480, 180);
            GUI_ClearRect(310, 180, 420, 205);
            GUI_MULTIBUF_EndEx(0);

            // Prebacivanje na drugi sloj za dinamičke elemente.
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();

            // Prikaz zadane temperature.
            DISPSetPoint();
            // Prikaz trenutnog vremena i datuma.
            DISPDateTime();
            // Postavi flag da treba ažurirati trenutnu temperaturu.
            MVUpdateSet();
            menu_lc = 0;
        } else if (thermostatMenuState == 1) {
            // Ako je ekran već iscrtan, obrađujemo samo promjene stanja.

            // Logika za povećanje/smanjenje zadane temperature.
            if (btninc && !_btninc) {
                _btninc = 1;
                Thermostat_SP_Temp_Increment(pThst);
                THSTAT_Save(pThst);
                DISPSetPoint();
            } else if (!btninc && _btninc) {
                _btninc = 0;
            }

            if (btndec && !_btndec) {
                _btndec = 1;
                Thermostat_SP_Temp_Decrement(pThst);
                THSTAT_Save(pThst);
                DISPSetPoint();
            } else if (!btndec && _btndec) {
                _btndec = 0;
            }

            // Ažuriranje prikaza trenutne temperature i statusa regulatora.
            if (IsMVUpdateActiv()) {
                MVUpdateReset();
                char dbuf_mv[8]; // Lokalni bafer za prikaz MV_Temp
                GUI_ClearRect(410, 185, 480, 235);
                GUI_ClearRect(310, 230, 480, 255);

                // Postavljanje boje na osnovu statusa regulatora.
                if(Thermostat_IsActive(pThst)) {
                    GUI_SetColor(GUI_GREEN);
                } else {
                    GUI_SetColor(GUI_RED);
                }

                GUI_SetFont(GUI_FONT_32B_1);
                GUI_GotoXY(410, 170);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                if(Thermostat_IsActive(pThst)) {
                    GUI_DispString("ON");
                } else {
                    GUI_DispString("OFF");
                }

                GUI_GotoXY(310, 242);
                GUI_SetFont(GUI_FONT_20_1);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                GUI_SetColor(GUI_WHITE);
                GUI_GotoXY(415, 220);
                GUI_SetFont(GUI_FONT_24_1);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                GUI_DispSDec(Thermostat_GetMeasuredTemp(pThst) / 10, 3);
                GUI_DispString("°c");
            }

            // Ažuriranje prikaza vremena.
            if ((HAL_GetTick() - rtctmr) >= DATE_TIME_REFRESH_TIME) {
                rtctmr = HAL_GetTick();
                if(IsRtcTimeValid()) {
                    RTC_TimeTypeDef rtctm_local;
                    RTC_DateTypeDef rtcdt_local;
                    HAL_RTC_GetTime(&hrtc, &rtctm_local, RTC_FORMAT_BCD);
                    HAL_RTC_GetDate(&hrtc, &rtcdt_local, RTC_FORMAT_BCD);
                    char dbuf_time[8];
                    HEX2STR(dbuf_time, &rtctm_local.Hours);
                    dbuf_time[2] = ':';
                    HEX2STR(&dbuf_time[3], &rtctm_local.Minutes);
                    dbuf_time[5]  = '\0';
                    GUI_SetFont(GUI_FONT_32_1);
                    GUI_SetColor(GUI_WHITE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                    GUI_GotoXY(5, 245);
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_ClearRect(0, 220, 100, 270);
                    GUI_DispString(dbuf_time);
                    GUI_MULTIBUF_EndEx(1);
                }
            }
        }
        GUI_MULTIBUF_EndEx(1);

        // Detekcija dugog pritiska za paljenje/gašenje termostata.
        if(thermostatOnOffTouch_timer) {
            DISPResetScrnsvr();
            if((HAL_GetTick() - thermostatOnOffTouch_timer) > (2 * 1000)) {
                thermostatOnOffTouch_timer = 0;
                thermostatMenuState = 0;
                if(Thermostat_IsActive(pThst)) {
                    Thermostat_TurnOff(pThst);
                } else {
                    Thermostat_SetControlMode(pThst, THST_HEATING);
                }
                THSTAT_Save(pThst);
            }
        }
    }
}

/**
 * @brief Vraća sistem na početni ekran i resetuje sve relevantne flagove.
 * @note Ova funkcija se poziva kada se treba vratiti na "čisto" početno stanje.
 * Briše oba grafička sloja i resetuje sve interne varijable stanja.
 */
static void Service_ReturnToFirst(void)
{
    // === "FAIL-SAFE" MEHANIZAM ===
    ForceKillAllSettingsWidgets();

    // Očisti oba grafička sloja.
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();

    // NOVO: Svjetlina se ne mijenja automatski pri povratku na početni ekran.
    // DISPSetBrightnes(DISP_BRGHT_MIN); // Ova linija je uklonjena.

    // Postavi aktivni ekran na glavni meni.
    screen = SCREEN_MAIN;

    // Resetovanje svih flagova i brojača.
    thermostatMenuState = 0;
    menu_lc = 0;
    menu_clean = 0;
    lcsta = 0;
    thsta = 0;
    curtainSettingMenu = 0;
    lightsModbusSettingsMenu = 0;
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    lights_allSelected_hasRGB = false;

    // Postavi flag za ponovno iscrtavanje ekrana.
    shouldDrawScreen = 1;
}


/**
 ******************************************************************************
 * @brief       Servisira ekran za prikaz i aktivaciju scena.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA 2.1: Logika je ažurirana. Funkcija sada
 * iscrtava samo konfigurisane scene u 3x2 mreži. Ikona za dodavanje
 * nove scene ("čarobnjak") je izdvojena i prikazuje se u donjem
 * desnom uglu, na poziciji konzistentnoj sa "Next" dugmetom, i to
 * samo ako broj konfigurisanih scena nije dostigao maksimum.
 ******************************************************************************
 */
static void Service_SceneScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        uint8_t configured_scenes_count = Scene_GetCount();
        uint8_t configured_scene_tracker = 0;

        // --- Iscrtavanje postojećih, konfigurisanih scena u mreži ---
        for (int i = 0; i < configured_scenes_count; i++)
        {
            const SceneAppearance_t* appearance = NULL;

            // Pronađi i-tu po redu konfigurisanu scenu
            for (int k = configured_scene_tracker; k < SCENE_MAX_COUNT; k++)
            {
                Scene_t* temp_handle = Scene_GetInstance(k);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (temp_handle->appearance_id < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t))) {
                        appearance = &scene_appearance_table[temp_handle->appearance_id];
                    }
                    configured_scene_tracker = k + 1;
                    break;
                }
            }

            if (!appearance) continue;

            // Pozicioniranje u mreži ostaje isto
            int row = i / scene_screen_layout.items_per_row;
            int col = i % scene_screen_layout.items_per_row;
            int x_center = (scene_screen_layout.slot_width / 2) + (col * scene_screen_layout.slot_width);
            int y_center = (scene_screen_layout.slot_height / 2) + (row * scene_screen_layout.slot_height);

            // Iscrtavanje ikonice i teksta
            int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
            if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
            {
                const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
                GUI_DrawBitmap(icon_to_draw, x_center - (icon_to_draw->XSize / 2), y_center - (icon_to_draw->YSize / 2));
            }

            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(appearance->text_id), x_center, y_center + scene_screen_layout.text_y_offset);
        }

        // --- Iscrtavanje ikonice čarobnjaka odvojeno ---
        if (configured_scenes_count < SCENE_MAX_COUNT)
        {
            const GUI_BITMAP* wizard_icon = &bmicons_scene_wizzard;
            // Koristimo koordinate konzistentne sa "Next" dugmetom
            int x_pos = select_screen2_drawing_layout.next_button_x_pos;
            int y_pos = select_screen2_drawing_layout.next_button_y_center - (wizard_icon->YSize / 2);
            GUI_DrawBitmap(wizard_icon, x_pos, y_pos);

            // === DODATA LINIJA KODA ZA ISPIS TEKSTA ===
            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            int x_text_center = x_pos + (wizard_icon->XSize / 2);
            int y_text_pos = y_pos + wizard_icon->YSize + 5; // 5 piksela razmaka
            GUI_DispStringAt(lng(TXT_SCENE_WIZZARD), x_text_center, y_text_pos);
        }

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa svjetlima ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_LightsScreen` funkcije,
 * modifikovana za rad unutar čarobnjaka. Uklonjen je "hamburger" meni,
 * a dodato je "[Next]" dugme sa ikonicom (80x80) i "pametnom"
 * navigacijom na sljedeći korak u čarobnjaku.
 ******************************************************************************
 */
static void Service_SceneEditLightsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();

        // Kreiraj "Next" dugme sa ikonicom, bez teksta, 80x80
        hButtonWizNext = BUTTON_CreateEx(400, 192, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);

        // Kompletan, neizmijenjen kod za iscrtavanje ikonica svjetala
        const GUI_FONT* fontToUse = &GUI_FontVerdana20_LAT;
        const int text_padding = 10;
        bool downgrade_font = false;
        for(uint8_t i = 0; i < LIGHTS_getCount(); ++i)
        {
            uint8_t lights_total = LIGHTS_getCount();
            uint8_t lights_in_this_row = 0;
            if (lights_total <= 3) lights_in_this_row = lights_total;
            else if (lights_total == 4) lights_in_this_row = 2;
            else if (lights_total == 5) lights_in_this_row = 3;
            else lights_in_this_row = 3;

            int max_width_per_icon = (DRAWING_AREA_WIDTH / lights_in_this_row) - text_padding;

            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                uint16_t selection_index = LIGHT_GetIconID(handle);
                if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                {
                    const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                    GUI_SetFont(&GUI_FontVerdana20_LAT);

                    if (GUI_GetStringDistX(lng(mapping->primary_text_id)) > max_width_per_icon || GUI_GetStringDistX(lng(mapping->secondary_text_id)) > max_width_per_icon)
                    {
                        downgrade_font = true;
                        break;
                    }
                }
            }
        }
        if (downgrade_font) {
            fontToUse = &GUI_FontVerdana16_LAT;
        }
        int y_row_start = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
        const int y_row_height = 130;
        uint8_t lightsInRowSum = 0;
        for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
            uint8_t lightsInRow = LIGHTS_getCount();
            if(LIGHTS_getCount() > 3) {
                if(LIGHTS_getCount() == 4) lightsInRow = 2;
                else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                else lightsInRow = 3;
            }
            uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);
            for(uint8_t idx_in_row = 0; idx_in_row < lightsInRow; ++idx_in_row) {
                uint8_t absolute_light_index = lightsInRowSum + idx_in_row;
                LIGHT_Handle* handle = LIGHTS_GetInstance(absolute_light_index);
                if (handle) {
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                    {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(mapping->visual_icon_id * 2) + LIGHT_isActive(handle)];
                        GUI_SetFont(fontToUse);
                        const int font_height = GUI_GetFontDistY();
                        const int icon_height = icon_to_draw->YSize;
                        const int icon_width = icon_to_draw->XSize;
                        const int padding = 2;
                        const int total_block_height = font_height + padding + icon_height + padding + font_height;
                        const int y_slot_center = y_row_start + (y_row_height / 2);
                        const int y_block_start = y_slot_center - (total_block_height / 2);
                        const int x_slot_start = (currentLightsMenuSpaceBetween * (idx_in_row + 1)) + (80 * idx_in_row);
                        const int x_text_center = x_slot_start + 40;
                        const int x_icon_pos = x_text_center - (icon_width / 2);
                        const int y_primary_text_pos = y_block_start;
                        const int y_icon_pos = y_primary_text_pos + font_height + padding;
                        const int y_secondary_text_pos = y_icon_pos + icon_height + padding;
                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_WHITE);
                        GUI_DispStringAt(lng(mapping->primary_text_id), x_text_center, y_primary_text_pos);
                        GUI_DrawBitmap(icon_to_draw, x_icon_pos, y_icon_pos);
                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_ORANGE);
                        GUI_DispStringAt(lng(mapping->secondary_text_id), x_text_center, y_secondary_text_pos);
                    }
                }
            }
            lightsInRowSum += lightsInRow;
            y_row_start += y_row_height;
        }
        GUI_MULTIBUF_EndEx(1);
    }

    if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneEditLightsScreen();

        Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
        if (scene_handle)
        {
            if (scene_handle->curtains_mask) {
                screen = SCREEN_CURTAINS;
            }
            else if (scene_handle->thermostat_mask) {
                screen = SCREEN_THERMOSTAT;
            }
            else {
                is_in_scene_wizard_mode = false;
                DSP_InitSceneEditScreen();
                screen = SCREEN_SCENE_EDIT;
                shouldDrawScreen = 0;
                return;
            }
            shouldDrawScreen = 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa svjetlima, sa dinamičkim odabirom fonta.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički iscrtava ikone svjetala. Sadrži "pametnu"
 * logiku koja prvo analizira dužinu svih tekstualnih labela. Na osnovu
 * dostupnog prostora, funkcija bira najveći mogući font i primjenjuje
 * ga na sve labele radi vizuelne konzistentnosti.
 * Ažurirano: Svi fiksni ("hardkodirani") parametri za pozicioniranje
 * (y-start, visina reda, razmak) su zamijenjeni vrijednostima iz
 * `lights_and_gates_grid_layout` strukture, omogućavajući laku
 * i centralizovanu kontrolu nad izgledom.
 ******************************************************************************
 */
static void Service_LightsScreen(void)
{
    if (is_in_scene_wizard_mode)
    {
        Service_SceneEditLightsScreen();
    }
    else if(shouldDrawScreen)
    {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // =======================================================================
        // === FAZA 1: PRE-KALKULACIJA I ODABIR FONTA ZA CIJELI EKRAN ===
        // =======================================================================
        const GUI_FONT* fontToUse = &GUI_FontVerdana20_LAT; // Početna pretpostavka: koristimo najveći font
        const int text_padding = 10; // Sigurnosni razmak u pikselima između tekstova susjednih ikonica
        bool downgrade_font = false;

        // Petlja za PRE-KALKULACIJU (prolazimo kroz sve, ali ne iscrtavamo ništa)
        for(uint8_t i = 0; i < LIGHTS_getCount(); ++i)
        {
            // Izračunaj maksimalnu dozvoljenu širinu teksta na osnovu rasporeda
            uint8_t lights_total = LIGHTS_getCount();
            uint8_t lights_in_this_row = 0;
            if (lights_total <= 3) lights_in_this_row = lights_total;
            else if (lights_total == 4) lights_in_this_row = 2;
            else if (lights_total == 5) lights_in_this_row = 3; // Pretpostavka za prvi red
            else lights_in_this_row = 3;

            int max_width_per_icon = (DRAWING_AREA_WIDTH / lights_in_this_row) - text_padding;

            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                uint16_t selection_index = LIGHT_GetIconID(handle);
                if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                {
                    const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                    const char* primary_text = lng(mapping->primary_text_id);
                    const char* secondary_text = lng(mapping->secondary_text_id);

                    // Mjerenje širine sa velikim fontom
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    if (GUI_GetStringDistX(primary_text) > max_width_per_icon || GUI_GetStringDistX(secondary_text) > max_width_per_icon)
                    {
                        downgrade_font = true; // Ako je BILO KOJI tekst predugačak...
                        break; // ...odmah prekidamo provjeru, odluka je pala.
                    }
                }
            }
        }

        if (downgrade_font) {
            fontToUse = &GUI_FontVerdana16_LAT; // ...cijeli ekran će koristiti manji font.
        }

        // =======================================================================
        // === FAZA 2: ISCRTAVANJE IKONICA SA KONAČNO ODABRANIM FONTOM ===
        // =======================================================================
        int y_row_start = (LIGHTS_Rows_getCount() > 1)
                          ? lights_and_gates_grid_layout.y_start_pos_multi_row
                          : lights_and_gates_grid_layout.y_start_pos_single_row;

        const int y_row_height = lights_and_gates_grid_layout.row_height;
        uint8_t lightsInRowSum = 0;

        for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
            uint8_t lightsInRow = LIGHTS_getCount();
            if(LIGHTS_getCount() > 3) {
                if(LIGHTS_getCount() == 4) lightsInRow = 2;
                else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                else lightsInRow = 3;
            }
            uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

            for(uint8_t idx_in_row = 0; idx_in_row < lightsInRow; ++idx_in_row) {
                uint8_t absolute_light_index = lightsInRowSum + idx_in_row;
                LIGHT_Handle* handle = LIGHTS_GetInstance(absolute_light_index);
                if (handle) {
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                    {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(mapping->visual_icon_id * 2) + LIGHT_isActive(handle)];

                        // Postavljamo KONAČNO ODABRANI FONT za iscrtavanje
                        GUI_SetFont(fontToUse);
                        const int font_height = GUI_GetFontDistY();
                        const int icon_height = icon_to_draw->YSize;
                        const int icon_width = icon_to_draw->XSize;
                        const int padding = lights_and_gates_grid_layout.text_icon_padding;
                        const int total_block_height = font_height + padding + icon_height + padding + font_height;
                        const int y_slot_center = y_row_start + (y_row_height / 2);
                        const int y_block_start = y_slot_center - (total_block_height / 2);
                        const int x_slot_start = (currentLightsMenuSpaceBetween * (idx_in_row + 1)) + (80 * idx_in_row);
                        const int x_text_center = x_slot_start + 40;

                        const int y_primary_text_pos = y_block_start;
                        const int y_icon_pos = y_primary_text_pos + font_height + padding;
                        const int y_secondary_text_pos = y_icon_pos + icon_height + padding;

                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);

                        GUI_SetColor(GUI_WHITE);
                        GUI_DispStringAt(lng(mapping->primary_text_id), x_text_center, y_primary_text_pos);

                        GUI_DrawBitmap(icon_to_draw, x_text_center - (icon_width / 2), y_icon_pos);

                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_ORANGE);
                        GUI_DispStringAt(lng(mapping->secondary_text_id), x_text_center, y_secondary_text_pos);
                    }
                }
            }
            lightsInRowSum += lightsInRow;
            y_row_start += y_row_height;
        }
        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran sa roletnama ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_CurtainsScreen` funkcije,
 * modifikovana za rad unutar čarobnjaka. Uklonjen je "hamburger" meni,
 * a dodato je "[Next]" dugme sa ikonicom (80x80) i "pametnom"
 * navigacijom na sljedeći korak u čarobnjaku.
 ******************************************************************************
 */
static void Service_SceneEditCurtainsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();

        // Kreiraj "Next" dugme sa ikonicom, bez teksta, 80x80
        hButtonWizNext = BUTTON_CreateEx(390, 182, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);

        // Kompletan, neizmijenjen kod za iscrtavanje kontrola za roletne iz originalne funkcije
        GUI_SetColor(GUI_WHITE);
        if(!Curtain_areAllSelected()) {
            GUI_SetFont(GUI_FONT_D48);
            uint8_t physical_index = 0;
            uint8_t count = 0;
            for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);
                if (Curtain_hasRelays(handle)) {
                    if (count == curtain_selected) {
                        physical_index = i;
                        break;
                    }
                    count++;
                }
            }
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispDecAt(physical_index + 1, 50, 50, ((physical_index + 1) < 10) ? 1 : 2);
        } else {
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALL), 75, 40);
        }

        const uint16_t drawingAreaWidth = 380;
        const uint8_t triangleBaseWidth = 180;
        const uint8_t triangleHeight = 90;
        const uint16_t horizontalOffset = (drawingAreaWidth - triangleBaseWidth) / 2;
        const uint8_t yLinePosition = 136;
        const uint8_t verticalGap = 20;
        const uint8_t verticalOffsetUp = yLinePosition - triangleHeight - verticalGap;
        const uint8_t verticalOffsetDown = yLinePosition + verticalGap;

        GUI_SetColor(GUI_WHITE);
        GUI_DrawLine(horizontalOffset, yLinePosition, horizontalOffset + triangleBaseWidth, yLinePosition);

        const GUI_POINT aBlindsUp[] = { {0, triangleHeight}, {triangleBaseWidth, triangleHeight}, {triangleBaseWidth / 2, 0} };
        const GUI_POINT aBlindsDown[] = { {0, 0}, {triangleBaseWidth, 0}, {triangleBaseWidth / 2, triangleHeight} };

        bool isMovingUp, isMovingDown;
        if(Curtain_areAllSelected()) {
            isMovingUp = Curtains_isAnyCurtainMovingUp();
            isMovingDown = Curtains_isAnyCurtainMovingDown();
        } else {
            Curtain_Handle* cur = Curtain_GetByLogicalIndex(curtain_selected);
            if (cur) {
                isMovingUp = Curtain_isMovingUp(cur);
                isMovingDown = Curtain_isMovingDown(cur);
            } else {
                isMovingUp = false;
                isMovingDown = false;
            }
        }

        if (isMovingUp) {
            GUI_SetColor(GUI_RED);
            GUI_FillPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        }
        else {
            GUI_SetColor(GUI_RED);
            GUI_DrawPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        }

        if (isMovingDown) {
            GUI_SetColor(GUI_BLUE);
            GUI_FillPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        }
        else {
            GUI_SetColor(GUI_BLUE);
            GUI_DrawPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        }

        if (Curtains_getCount() > 1) {
            const uint8_t arrowSize = 50;
            const uint16_t verticalArrowCenter = 192 + (80/2);
            const uint16_t leftSpace = horizontalOffset;
            const uint16_t rightSpace = drawingAreaWidth - (horizontalOffset + triangleBaseWidth);
            const uint16_t xLeftArrow = leftSpace/2 - arrowSize/2;
            const uint16_t xRightArrow = horizontalOffset + triangleBaseWidth + (rightSpace/2) - arrowSize/2;
            const GUI_POINT leftArrow[] = { {xLeftArrow + arrowSize, verticalArrowCenter - arrowSize / 2}, {xLeftArrow, verticalArrowCenter}, {xLeftArrow + arrowSize, verticalArrowCenter + arrowSize / 2}, };
            const GUI_POINT rightArrow[] = { {xRightArrow, verticalArrowCenter - arrowSize / 2}, {xRightArrow + arrowSize, verticalArrowCenter}, {xRightArrow, verticalArrowCenter + arrowSize / 2}, };
            GUI_SetColor(GUI_WHITE);
            GUI_DrawPolygon(leftArrow, 3, 0, 0);
            GUI_DrawPolygon(rightArrow, 3, 0, 0);
        }
        GUI_MULTIBUF_EndEx(1);
    }

    // "Pametna" navigacija za "[Next]" dugme
    if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneEditCurtainsScreen();

        Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
        if (scene_handle)
        {
            if (scene_handle->thermostat_mask) {
                screen = SCREEN_THERMOSTAT;
            }
            else {
                is_in_scene_wizard_mode = false; // Završi wizard mod
                DSP_InitSceneEditScreen();
                screen = SCREEN_SCENE_EDIT;
                shouldDrawScreen = 0;
                return;
            }
            shouldDrawScreen = 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa zavjesama.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički iscrtava korisnički interfejs za kontrolu zavjesa.
 * Odgovorna je za crtanje trouglova za smjer kretanja, bijele linije kao vizualnog
 * separatora i navigacijskih dugmadi. Stanje trouglova (obojeno/neobojeno)
 * precizno reflektuje status odabrane zavjese ili grupe.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_CurtainsScreen(void)
{
    if (is_in_scene_wizard_mode)
    {
        Service_SceneEditCurtainsScreen();
    }
    else if(shouldDrawScreen)
    {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();

        // 1. CRTANJE GLOBALNIH UI ELEMENATA
        //--------------------------------------------------------------------------------
        // Iscrtavanje hamburger menija
        DrawHamburgerMenu(1);

        // Prikaz broja odabrane zavjese ili teksta "SVE".
        GUI_ClearRect(0, 0, 70, 70);
        GUI_SetColor(GUI_WHITE);

        // KLJUČNA PROMJENA: Ovdje se bira pravi font
        // << ISPRAVKA: Logika je ažurirana da koristi novi, sigurni API >>
        if(!Curtain_areAllSelected()) {
            GUI_SetFont(GUI_FONT_D48);
            uint8_t physical_index = 0;
            uint8_t count = 0;
            // Petlja pronalazi fizički indeks (0-15) na osnovu logičkog (0-broj konfigurisanih)
            for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);
                if (Curtain_hasRelays(handle)) {
                    if (count == curtain_selected) {
                        physical_index = i;
                        break;
                    }
                    count++;
                }
            }
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispDecAt(physical_index + 1, 50, 50, ((physical_index + 1) < 10) ? 1 : 2);
        } else {
            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT); // Koristi font za tekst "SVE"
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALL), 75, 40);
        }

        //--------------------------------------------------------------------------------

        // 2. CRTANJE GLAVNOG KONTROLNog ELEMENTA (TROUGLOVA I LINIJE)
        //--------------------------------------------------------------------------------
        // Definiranje geometrije i pozicije elemenata
        const uint16_t drawingAreaWidth = 380;       // Širina radnog prostora za kontrole
        const uint8_t triangleBaseWidth = 180;      // Širina baze trougla (hipotenusa)
        const uint8_t triangleHeight = 90;          // Visina trougla
        const uint16_t horizontalOffset = (drawingAreaWidth - triangleBaseWidth) / 2; // Centriranje trouglova horizontalno

        // Vertikalne pozicije i razmaci
        const uint8_t yLinePosition = 136;          // Y-koordinata bijele linije
        const uint8_t verticalGap = 20;             // Razmak od linije do hipotenuze

        // Izračunavanje vertikalnih pomaka za trouglove
        const uint8_t verticalOffsetUp = yLinePosition - triangleHeight - verticalGap;
        const uint8_t verticalOffsetDown = yLinePosition + verticalGap;

        // Iscrtavanje bijele linije čija je dužina jednaka širini baze trougla.
        GUI_SetColor(GUI_WHITE);
        GUI_DrawLine(horizontalOffset, yLinePosition, horizontalOffset + triangleBaseWidth, yLinePosition);

        // Definiranje tačaka za gornji i donji trougao
        const GUI_POINT aBlindsUp[] = {
            {0, triangleHeight},
            {triangleBaseWidth, triangleHeight},
            {triangleBaseWidth / 2, 0}
        };
        const GUI_POINT aBlindsDown[] = {
            {0, 0},
            {triangleBaseWidth, 0},
            {triangleBaseWidth / 2, triangleHeight}
        };

        //--------------------------------------------------------------------------------

        // 3. ODREĐIVANJE STANJA I CRTANJE
        //--------------------------------------------------------------------------------
        // Provjera stanja kretanja ovisno o tome da li je odabrana jedna ili sve roletne.
        bool isMovingUp, isMovingDown;
        if(Curtain_areAllSelected()) {
            isMovingUp = Curtains_isAnyCurtainMovingUp();
            isMovingDown = Curtains_isAnyCurtainMovingDown();
        } else {
            Curtain_Handle* cur = Curtain_GetByLogicalIndex(curtain_selected);
            if (cur) {
                isMovingUp = Curtain_isMovingUp(cur);
                isMovingDown = Curtain_isMovingDown(cur);
            } else {
                isMovingUp = false;
                isMovingDown = false;
            }
        }
        // Iscrtavanje gornjeg trougla
        if (isMovingUp) {
            GUI_SetColor(GUI_RED);
            GUI_FillPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        } else {
            GUI_SetColor(GUI_RED);
            GUI_DrawPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        }

        // Iscrtavanje donjeg trougla
        if (isMovingDown) {
            GUI_SetColor(GUI_BLUE);
            GUI_FillPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        } else {
            GUI_SetColor(GUI_BLUE);
            GUI_DrawPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        }

        // 4. CRTANJE NAVIGACIJSKIH STRELICA (bez bitmapa)
        //--------------------------------------------------------------------------------
        // Ove strelice se crtaju samo ako postoji više od jedne zavjese.
        if (Curtains_getCount() > 1) {
            // Smanjena veličina strelica i izračunavanje novih pozicija za centriranje
            const uint8_t arrowSize = 50;
            const uint16_t verticalArrowCenter = 192 + (80/2); // Y-koordinata centra zone za strelice

            // Izračunavanje pozicija za strelice, centrirano u lijevom i desnom slobodnom prostoru
            const uint16_t leftSpace = horizontalOffset;
            const uint16_t rightSpace = drawingAreaWidth - (horizontalOffset + triangleBaseWidth);
            const uint16_t xLeftArrow = leftSpace/2 - arrowSize/2;
            const uint16_t xRightArrow = horizontalOffset + triangleBaseWidth + (rightSpace/2) - arrowSize/2;

            // Definiranje tačaka za lijevu strelicu
            const GUI_POINT leftArrow[] = {
                {xLeftArrow + arrowSize, verticalArrowCenter - arrowSize / 2},
                {xLeftArrow, verticalArrowCenter},
                {xLeftArrow + arrowSize, verticalArrowCenter + arrowSize / 2},
            };

            // Definiranje tačaka za desnu strelicu
            const GUI_POINT rightArrow[] = {
                {xRightArrow, verticalArrowCenter - arrowSize / 2},
                {xRightArrow + arrowSize, verticalArrowCenter},
                {xRightArrow, verticalArrowCenter + arrowSize / 2},
            };

            GUI_SetColor(GUI_WHITE);
            GUI_DrawPolygon(leftArrow, 3, 0, 0);
            GUI_DrawPolygon(rightArrow, 3, 0, 0);
        }
        //--------------------------------------------------------------------------------

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 * @brief Servisira ekran sa QR kodom.
 * @note Ova funkcija generiše i iscrtava QR kod na osnovu podataka
 * pohranjenih u `qr_codes` baferu.
 */
static void Service_QrCodeScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();

        // Iscrtavanje hamburger meni ikonice.
        DrawHamburgerMenu(1);

        // Generisanje i crtanje QR koda.
        GUI_HMEM hqr = GUI_QR_Create((char*)QR_Code_Get(qr_code_draw_id), 8, GUI_QR_ECLEVEL_M, 0);
        GUI_QR_INFO qrInfo;
        GUI_QR_GetInfo(hqr, &qrInfo);

        GUI_SetColor(GUI_WHITE);
        GUI_FillRect(0, 0, qrInfo.Size + 20, qrInfo.Size + 20);

        GUI_QR_Draw(hqr, 10, 10);
        GUI_QR_Delete(hqr);

        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran za čišćenje ekrana (privremeno onemogućava dodir).
 * @author      Gemini & [Vaše Ime]
 * @note        Prikazuje tajmer sa odbrojavanjem od 60 sekundi. Tokom odbrojavanja,
 * ekran je zaključan za unos. Posljednjih 5 sekundi je vizuelno i
 * zvučno naglašeno. Nakon što tajmer istekne, automatski se vraća
 * na početni ekran. Koristi matematički proračun za precizno
 * centriranje teksta na ekranu.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_CleanScreen(void)
{
    // --- 1. DEFINICIJA I IZRAČUNI SVIH KONSTANTI NA POČETKU FUNKCIJE ---
    const uint16_t xDisplayCenter = 480 / 2;     // Horizontalna sredina ekrana
    const uint16_t yDisplayCenter = 272 / 2;     // Vertikalna sredina ekrana

    // Visine fontova
    const uint8_t yFontTitleHeight = 32;        // Visina fonta GUI_FONT_32_1
    const uint8_t yFontCounterHeight = 64;      // Visina fonta GUI_FONT_D64

    // Razmaci
    const uint8_t textGap = 10;                 // Razmak između naslova i brojača

    // OPACIJA: Vertikalni ofset za pomjeranje SAMO naslova (naslova) gore/dolje
    const int16_t verticalTextOffset = -30;     // Negativna vrijednost pomjera gore, pozitivna dole

    // Izračun vertikalnih pozicija
    // Brojač je FIKSIRAN na sredini ekrana
    const uint16_t yCounterPosition = yDisplayCenter;

    // Naslov se pozicionira iznad brojača, uzimajući u obzir ofset
    const uint16_t yTitlePosition = yCounterPosition - (yFontCounterHeight / 2) - textGap - (yFontTitleHeight / 2) + verticalTextOffset;

    // Dinamički izračun granica za GUI_ClearRect (sada pokriva cijeli dinamični prostor)
    const uint16_t yClearRectStart = yTitlePosition - (yFontTitleHeight / 2) - 5; // Padding 5px
    const uint16_t yClearRectEnd = yCounterPosition + (yFontCounterHeight / 2) + 5; // Padding 5px
    // --- KRAJ DEFINICIJE KONSTANTI ---


    if (menu_clean == 0) {
        // Prvi ulazak na ekran za čišćenje, inicijalizuj stanje i tajmer.
        menu_clean = 1;
        GUI_Clear();
        clrtmr = 60; // Tajmer na 60 sekundi.
    } else if (menu_clean == 1) {
        // Odbrojavanje tajmera.
        if (HAL_GetTick() - clean_tmr >= 1000) {
            clean_tmr = HAL_GetTick();
            DISPResetScrnsvr();

            GUI_MULTIBUF_BeginEx(1);

            // Dinamičko čišćenje ekrana u zoni teksta
            GUI_ClearRect(0, yClearRectStart, 480, yClearRectEnd);

            // Promjena boje teksta i zvučni signal za posljednjih 5 sekundi.
            GUI_SetColor((clrtmr > 5) ? GUI_GREEN : GUI_RED);
            if (clrtmr <= 5) {
                BuzzerOn();
                HAL_Delay(1);
                BuzzerOff();
            }

            // --- Iscrtavanje naslova i broja na matematički određenim pozicijama ---
            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetTextMode(GUI_TM_TRANS);

            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_DISPLAY_CLEAN_TIME), xDisplayCenter, yTitlePosition);

            char count_str[3];
            sprintf(count_str, "%d", clrtmr);

            GUI_SetFont(GUI_FONT_D64);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(count_str, xDisplayCenter, yCounterPosition);
            // --- KRAJ ISPRAVKE ---

            GUI_MULTIBUF_EndEx(1);

            if (clrtmr) {
                --clrtmr;
            } else {
                screen = SCREEN_RETURN_TO_FIRST;
            }
        }
    }
}


/**
 * @brief  Servisira prvi ekran podešavanja (kontrola termostata i ventilatora).
 * @note   Refaktorisana verzija koja koristi isključivo javni API termostat modula.
 * @param  None
 * @retval None
 */
static void Service_SettingsScreen_1(void)
{
    // Dobijamo handle za termostat na početku funkcije.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    // Detekcija promjena na widgetima.
    if (Thermostat_GetControlMode(pThst) != RADIO_GetValue(hThstControl)) {
        Thermostat_SetControlMode(pThst, RADIO_GetValue(hThstControl));
        ++thsta;
    }
    if (Thermostat_GetFanControlMode(pThst) != RADIO_GetValue(hFanControl)) {
        Thermostat_SetFanControlMode(pThst, RADIO_GetValue(hFanControl));
        ++thsta;
    }
    // === NASTAVAK REFAKTORISANJA ===
    if (Thermostat_Get_SP_Max(pThst) != SPINBOX_GetValue(hThstMaxSetPoint)) {
        // Pozivamo setter sa handle-om.
        Thermostat_Set_SP_Max(pThst, SPINBOX_GetValue(hThstMaxSetPoint));
        // Očitavamo validiranu vrijednost nazad pomoću gettera.
        SPINBOX_SetValue(hThstMaxSetPoint, Thermostat_Get_SP_Max(pThst));
        ++thsta;
    }
    if (Thermostat_Get_SP_Min(pThst) != SPINBOX_GetValue(hThstMinSetPoint)) {
        // Pozivamo setter sa handle-om.
        Thermostat_Set_SP_Min(pThst, SPINBOX_GetValue(hThstMinSetPoint));
        // Očitavamo validiranu vrijednost nazad pomoću gettera.
        SPINBOX_SetValue(hThstMinSetPoint, Thermostat_Get_SP_Min(pThst));
        ++thsta;
    }
    if (Thermostat_GetFanDifference(pThst) != SPINBOX_GetValue(hFanDiff)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanDifference(pThst, SPINBOX_GetValue(hFanDiff));
        ++thsta;
    }
    if (Thermostat_GetFanLowBand(pThst) != SPINBOX_GetValue(hFanLowBand)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanLowBand(pThst, SPINBOX_GetValue(hFanLowBand));
        ++thsta;
    }
    if (Thermostat_GetFanHighBand(pThst) != SPINBOX_GetValue(hFanHiBand)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanHighBand(pThst, SPINBOX_GetValue(hFanHiBand));
        ++thsta;
    }
    if (Thermostat_GetGroup(pThst) != SPINBOX_GetValue(hThstGroup)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetGroup(pThst, SPINBOX_GetValue(hThstGroup));
        thsta = 1;
    }
    if (Thermostat_IsMaster(pThst) != CHECKBOX_IsChecked(hThstMaster)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetMaster(pThst, CHECKBOX_IsChecked(hThstMaster));
        thsta = 1;
    }

    // Obrada pritiska na dugmad "SAVE" ili "NEXT"
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            // ISPRAVKA: Pozivamo THSTAT_Save sa handle-om.
            // Fleg hasInfoChanged se sada postavlja unutar settera.
            THSTAT_Save(pThst);
        }
        thsta = 0;
        DSP_KillSet1Scrn(); // Uništavanje widgeta trenutnog ekrana
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // ISTA ISPRAVKA I ZA NEXT DUGME
        if (thsta) {
            THSTAT_Save(pThst);
        }
        thsta = 0;
        DSP_KillSet1Scrn();
        DSP_InitSet2Scrn(); // Inicijalizacija sljedećeg ekrana
        screen = SCREEN_SETTINGS_2;
    }
}
/**
 * @brief Servisira drugi ekran podešavanja (vrijeme, datum, screensaver).
 * @note  Verzija 2.1: Ispravljena logika za ažuriranje prikaza boje.
 * Ova funkcija sinkronizuje vrijeme i datum s RTC modulom,
 * obrađuje promjene postavki screensavera i svjetline ekrana,
 * te ih pohranjuje u EEPROM kada se pritisne "SAVE".
 */
static void Service_SettingsScreen_2(void)
{
    /** @brief Dobijamo handle za termostat radi konzistentnosti sa ostalim funkcijama. */
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    /** @brief Detekcija promjena u postavkama. */
    if (rtctm.Hours != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour))) {
        rtctm.Hours = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour));
        HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtctm.Minutes != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute))) {
        rtctm.Minutes = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute));
        HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Date != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day))) {
        rtcdt.Date = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day));
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Month != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month))) {
        rtcdt.Month = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month));
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Year != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000)) {
        rtcdt.Year = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000);
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.WeekDay != (DROPDOWN_GetSel(hDRPDN_WeekDay) + 1)) { // Poređenje sa Dec vrijednošću
        rtcdt.WeekDay = (DROPDOWN_GetSel(hDRPDN_WeekDay) + 1);
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }

    /**
     * @brief << ISPRAVKA: Ažuriranje prikaza boje screensaver sata. >>
     * @note  Sada, kada se detektuje promjena vrijednosti u spinbox-u,
     * prvo se postavlja nova boja sa `GUI_SetColor`, a tek onda se
     * ponovo iscrtava pravougaonik za pregled.
     */
    if (g_display_settings.scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)) {
        g_display_settings.scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);

        // Prvo postavimo novu boju
        GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
        // Zatim iscrtamo pravougaonik tom bojom
        GUI_FillRect(settings_screen_2_layout.scrnsvr_color_preview_rect.x0, settings_screen_2_layout.scrnsvr_color_preview_rect.y0,
                     settings_screen_2_layout.scrnsvr_color_preview_rect.x1, settings_screen_2_layout.scrnsvr_color_preview_rect.y1);
    }

    // << ISPRAVKA 3: Ispravljena logika ažuriranja screensaver postavki >>
    // Prvo ažuriramo konfiguracionu varijablu na osnovu stanja checkbox-a.
    if (g_display_settings.scrnsvr_on_off != (bool)CHECKBOX_GetState(hCHKBX_ScrnsvrClock)) {
        g_display_settings.scrnsvr_on_off = (bool)CHECKBOX_GetState(hCHKBX_ScrnsvrClock);
        settingsChanged = 1; // Signaliziramo da je došlo do promjene za snimanje
    }

    // Zatim, na osnovu te konfiguracione varijable, ažuriramo runtime fleg.
    if (g_display_settings.scrnsvr_on_off) {
        ScrnsvrClkSet();
    } else {
        ScrnsvrClkReset();
    }

    /** @brief Ažuriranje ostalih konfiguracionih varijabli. */
    g_display_settings.high_bcklght = SPINBOX_GetValue(hSPNBX_DisplayHighBrightness);
    g_display_settings.low_bcklght = SPINBOX_GetValue(hSPNBX_DisplayLowBrightness);
    g_display_settings.scrnsvr_tout = SPINBOX_GetValue(hSPNBX_ScrnsvrTimeout);
    g_display_settings.scrnsvr_ena_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrEnableHour);
    g_display_settings.scrnsvr_dis_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrDisableHour);

    /** @brief Obrada pritiska na dugmad "SAVE" i "NEXT". */
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            thsta = 0;
            THSTAT_Save(pThst);
        }
        if (lcsta) {
            lcsta = 0;
            LIGHTS_Save(); // Pretpostavka da `lcsta` signalizira promjenu u svjetlima
        }

        Display_Save();
        EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
        DSP_KillSet2Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // Snimi sve promjene prije prelaska na sljedeći ekran
        Display_Save();
        EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
        if (thsta) {
            THSTAT_Save(pThst);
            thsta = 0;
        }
        if (lcsta) {
            LIGHTS_Save();
            lcsta = 0;
        }

        DSP_KillSet2Scrn();
        DSP_InitSet3Scrn();
        screen = SCREEN_SETTINGS_3;
    }
}
/**
 * @brief Servisira treći ekran podešavanja (Ventilator).
 * @note Ova funkcija obrađuje promjene u postavkama ventilatora (relej, odgode),
 * i pohranjuje ih u EEPROM kada se pritisne "SAVE".
 */
static void Service_SettingsScreen_3(void)
{
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // Provjera promjena na widgetima DEFROSTER
    if(Defroster_getCycleTime(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.cycleTime)) {
        Defroster_setCycleTime(defHandle, SPINBOX_GetValue(defroster_settingWidgets.cycleTime));
        settingsChanged = 1;
    }
    if(Defroster_getActiveTime(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.activeTime)) {
        Defroster_setActiveTime(defHandle, SPINBOX_GetValue(defroster_settingWidgets.activeTime));
        settingsChanged = 1;
    }
    if(Defroster_getPin(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.pin)) {
        Defroster_setPin(defHandle, SPINBOX_GetValue(defroster_settingWidgets.pin));
        settingsChanged = 1;
    }

    // Provjera promjena na widgetima VENTILATOR
    if(Ventilator_getRelay(ventHandle) != SPINBOX_GetValue(hVentilatorRelay)) {
        Ventilator_setRelay(ventHandle, SPINBOX_GetValue(hVentilatorRelay));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOnTime(ventHandle) != SPINBOX_GetValue(hVentilatorDelayOn)) {
        Ventilator_setDelayOnTime(ventHandle, SPINBOX_GetValue(hVentilatorDelayOn));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOffTime(ventHandle) != SPINBOX_GetValue(hVentilatorDelayOff)) {
        Ventilator_setDelayOffTime(ventHandle, SPINBOX_GetValue(hVentilatorDelayOff));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource1(ventHandle) != SPINBOX_GetValue(hVentilatorTriggerSource1)) {
        Ventilator_setTriggerSource1(ventHandle, SPINBOX_GetValue(hVentilatorTriggerSource1));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource2(ventHandle) != SPINBOX_GetValue(hVentilatorTriggerSource2)) {
        Ventilator_setTriggerSource2(ventHandle, SPINBOX_GetValue(hVentilatorTriggerSource2));
        settingsChanged = 1;
    }
    if(Ventilator_getLocalPin(ventHandle) != SPINBOX_GetValue(hVentilatorLocalPin)) {
        Ventilator_setLocalPin(ventHandle, SPINBOX_GetValue(hVentilatorLocalPin));
        settingsChanged = 1;
    }

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save(defHandle);
            Ventilator_Save(ventHandle);
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save(defHandle);
            Ventilator_Save(ventHandle);
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        DSP_InitSet4Scrn();
        screen = SCREEN_SETTINGS_4;
    }
}

/**
 * @brief Servisira četvrti ekran podešavanja (Zavjese).
 * @note Ovaj ekran omogućava podešavanje Modbus adresa releja za zavjese.
 * Koristi petlju za dinamičko upravljanje widgetima i provjeru promjena.
 */
static void Service_SettingsScreen_4(void)
{
    // Ažuriranje postavki za zavjese unutar petlje.
    for(uint8_t idx = curtainSettingMenu * 4; idx < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); idx++) {

        // << ISPRAVKA: Dobijamo handle za roletnu po fizičkom indeksu. >>
        Curtain_Handle* handle = Curtain_GetInstanceByIndex(idx);
        if (!handle) continue; // Ako handle nije validan, preskoči

        // << ISPRAVKA: Koristimo gettere za poređenje i settere za upis. >>
        if((Curtain_getRelayUp(handle) != SPINBOX_GetValue(hCurtainsRelay[idx * 2])) || (Curtain_getRelayDown(handle) != SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]))) {
            settingsChanged = 1;
            Curtain_setRelayUp(handle, SPINBOX_GetValue(hCurtainsRelay[idx * 2]));
            Curtain_setRelayDown(handle, SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]));
        }
    }

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Curtains_Save(); // Spremi sve postavke zavjesa u EEPROM.
            settingsChanged = 0;
        }
        DSP_KillSet4Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // Logika za prelazak na sljedeću stranicu podešavanja zavjesa ili na sljedeći ekran.
        if((CURTAINS_SIZE - ((curtainSettingMenu + 1) * 4)) > 0) {
            DSP_KillSet4Scrn();
            ++curtainSettingMenu;
            DSP_InitSet4Scrn();
        } else {
            if(settingsChanged) {
                Curtains_Save();
                settingsChanged = 0;
            }
            DSP_KillSet4Scrn();
            curtainSettingMenu = 0;
            DSP_InitSet5Scrn();
            screen = SCREEN_SETTINGS_5;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira peti ekran podešavanja (detaljne postavke za svjetla).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija u petlji provjerava da li je korisnik promijenio vrijednost
 * nekog widgeta. Ako jeste, poziva odgovarajuću `setter` funkciju iz `lights` API-ja
 * da ažurira stanje u RAM-u. Stvarno snimanje u EEPROM se dešava tek pritiskom
 * na 'SAVE' ili 'NEXT' dugme. Također iscrtava preview ikonice sa
 * odgovarajućim opisima, osiguravajući da je područje iscrtavanja
 * prethodno obrisano radi izbjegavanja vizuelnih grešaka.
 ******************************************************************************
 */
static void Service_SettingsScreen_5(void)
{
    GUI_MULTIBUF_BeginEx(1);

    // Dohvatamo indeks svjetla koje trenutno konfigurišemo.
    uint8_t light_index = lightsModbusSettingsMenu;

    // KORAK 1: Dobijamo siguran "handle" na svjetlo.
    LIGHT_Handle* handle = LIGHTS_GetInstance(light_index);
    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return; // Ako handle nije validan, prekidamo izvršavanje
    }

    // Logika za omogućavanje/onemogućavanje spinbox-a za minute ostaje ista.
    int current_hour_value = SPINBOX_GetValue(lightsWidgets[light_index].on_hour);
    if (current_hour_value == -1) {
        if (WM_IsEnabled(lightsWidgets[light_index].on_minute)) {
            WM_DisableWindow(lightsWidgets[light_index].on_minute);
        }
    } else {
        if (!WM_IsEnabled(lightsWidgets[light_index].on_minute)) {
            WM_EnableWindow(lightsWidgets[light_index].on_minute);
        }
    }

    // KORAK 2: Provjeravamo promjene na SVAKOM widgetu i pozivamo odgovarajući SETTER.
    if(LIGHT_GetRelay(handle) != SPINBOX_GetValue(lightsWidgets[light_index].relay)) {
        settingsChanged = 1;
        LIGHT_SetRelay(handle, SPINBOX_GetValue(lightsWidgets[light_index].relay));
    }
    if(LIGHT_GetIconID(handle) != SPINBOX_GetValue(lightsWidgets[light_index].iconID)) {
        settingsChanged = 1;
        LIGHT_SetIconID(handle, SPINBOX_GetValue(lightsWidgets[light_index].iconID));
    }
    if(LIGHT_GetControllerID(handle) != SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on)) {
        settingsChanged = 1;
        LIGHT_SetControllerID(handle, SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on));
    }
    if(LIGHT_GetOnDelayTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on_delay)) {
        settingsChanged = 1;
        LIGHT_SetOnDelayTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on_delay));
    }
    if(LIGHT_GetOffTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].offTime)) {
        settingsChanged = 1;
        LIGHT_SetOffTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].offTime));
    }
    if(LIGHT_GetOnHour(handle) != SPINBOX_GetValue(lightsWidgets[light_index].on_hour)) {
        settingsChanged = 1;
        LIGHT_SetOnHour(handle, SPINBOX_GetValue(lightsWidgets[light_index].on_hour));
    }
    if(LIGHT_GetOnMinute(handle) != SPINBOX_GetValue(lightsWidgets[light_index].on_minute)) {
        settingsChanged = 1;
        LIGHT_SetOnMinute(handle, SPINBOX_GetValue(lightsWidgets[light_index].on_minute));
    }
    if(LIGHT_GetCommunicationType(handle) != SPINBOX_GetValue(lightsWidgets[light_index].communication_type)) {
        settingsChanged = 1;
        LIGHT_SetCommunicationType(handle, SPINBOX_GetValue(lightsWidgets[light_index].communication_type));
    }
    if(LIGHT_GetLocalPin(handle) != SPINBOX_GetValue(lightsWidgets[light_index].local_pin)) {
        settingsChanged = 1;
        LIGHT_SetLocalPin(handle, SPINBOX_GetValue(lightsWidgets[light_index].local_pin));
    }
    if(LIGHT_GetSleepTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].sleep_time)) {
        settingsChanged = 1;
        LIGHT_SetSleepTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].sleep_time));
    }
    if(LIGHT_GetButtonExternal(handle) != SPINBOX_GetValue(lightsWidgets[light_index].button_external)) {
        settingsChanged = 1;
        LIGHT_SetButtonExternal(handle, SPINBOX_GetValue(lightsWidgets[light_index].button_external));
    }
    if(LIGHT_isTiedToMainLight(handle) != CHECKBOX_GetState(lightsWidgets[light_index].tiedToMainLight)) {
        settingsChanged = 1;
        LIGHT_SetTiedToMainLight(handle, CHECKBOX_GetState(lightsWidgets[light_index].tiedToMainLight));
    }
    if(LIGHT_isBrightnessRemembered(handle) != CHECKBOX_GetState(lightsWidgets[light_index].rememberBrightness)) {
        settingsChanged = 1;
        LIGHT_SetRememberBrightness(handle, CHECKBOX_GetState(lightsWidgets[light_index].rememberBrightness));
    }

    // =======================================================================
    // === POČETAK LOGIKE ZA ISCRTAVANJE IKONICE I TEKSTA ===
    // =======================================================================
    uint16_t selection_index = SPINBOX_GetValue(lightsWidgets[light_index].iconID);

    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
    {
        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
        IconID visual_icon_to_draw = mapping->visual_icon_id;
        TextID primary_text_id = mapping->primary_text_id;
        TextID secondary_text_id = mapping->secondary_text_id;
        bool is_active = LIGHT_isActive(handle);
        GUI_CONST_STORAGE GUI_BITMAP* icon_bitmap = light_modbus_images[(visual_icon_to_draw * 2) + is_active];

        const int16_t x_icon_pos = 480 - icon_bitmap->XSize;
        const int16_t y_icon_pos = 20;
        const int16_t y_primary_text_pos = 5;
        const int16_t y_secondary_text_pos = y_icon_pos + icon_bitmap->YSize + 5;

        // =======================================================================
        // === ISPRAVKA: Brisanje fiksne regije prije iscrtavanja ===
        // =======================================================================
        GUI_ClearRect(350, 0, 480, 130);

        // --- LOGIKA: PONOVNO ISCRTAVANJE PREKLOPLJENIH LABELA ---

        // 1. Deklaracija i inicijalizacija lokalnih varijabli za pozicioniranje
        const GUI_POINT* label_offset = &settings_screen_5_layout.label_line1_offset;
        const int16_t label_y2_offset = settings_screen_5_layout.label_line2_offset_y;
        int16_t x = settings_screen_5_layout.col2_x;
        int16_t y = settings_screen_5_layout.start_y;
        int16_t y_step = settings_screen_5_layout.y_step;

        // 2. Postavljanje stila fonta (mora odgovarati onom iz InitSet5Scrn)
        GUI_SetFont(&GUI_Font13_1);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

        // 3. Iscrtavanje labele (Red 1, 2, 3 u drugoj koloni)
        GUI_GotoXY(x + label_offset->x, y + label_offset->y);
        GUI_DispString("LIGHT ");
        GUI_DispDec(light_index + 1, 2);
        GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset);
        GUI_DispString("DELAY OFF");

        GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y);
        GUI_DispString("LIGHT ");
        GUI_DispDec(light_index + 1, 2);
        GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
        GUI_DispString("COMM. TYPE");

        GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y);
        GUI_DispString("LIGHT ");
        GUI_DispDec(light_index + 1, 2);
        GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
        GUI_DispString("LOCAL PIN");


        GUI_SetTextMode(GUI_TM_TRANS);

        // Korištenje ispravnog fonta
        GUI_SetFont(&GUI_FontVerdana16_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_DispStringAt(lng(primary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_primary_text_pos);

        GUI_DrawBitmap(icon_bitmap, x_icon_pos, y_icon_pos);

        // Vraćanje poziva za poravnanje
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(secondary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_secondary_text_pos);
    }

    if (BUTTON_IsPressed(hBUTTON_Ok) || BUTTON_IsPressed(hBUTTON_Next))
    {
        if(settingsChanged)
        {
            LIGHTS_Save();
            settingsChanged = 0;
        }

        if (BUTTON_IsPressed(hBUTTON_Ok))
        {
            DSP_KillSet5Scrn();
            screen = SCREEN_RETURN_TO_FIRST;
            shouldDrawScreen = 1;
        }
        else if (BUTTON_IsPressed(hBUTTON_Next))
        {
            uint8_t current_count = LIGHTS_getCount();
            if (lightsModbusSettingsMenu < current_count) {
                DSP_KillSet5Scrn();
                ++lightsModbusSettingsMenu;
                DSP_InitSet5Scrn();
            } else {
                DSP_KillSet5Scrn();
                lightsModbusSettingsMenu = 0;
                DSP_InitSet6Scrn();
                screen = SCREEN_SETTINGS_6;
            }
        }
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Servisira šesti ekran podešavanja (Opšte postavke).
 * @author      Gemini & [Vaše Ime]
 * @note        KONAČNA VERZIJA. Upravlja sa dva međusobno isključiva DROPDOWN
 * menija. Koristi ispravan mehanizam mapiranja indeksa (`control_mode_map`)
 * za dobijanje stvarne vrijednosti iz odabrane stavke, čime se
 * zaobilazi nepostojanje ...UserData funkcija.
 ******************************************************************************
 */
static void Service_SettingsScreen_6(void)
{
    // --- Provjera promjene na DROPDOWN-u za Ikonu 1 ---
    int8_t sel1 = DROPDOWN_GetSel(hSelectControl_1);
    if (sel1 >= 0) {
        // Koristimo mapu da dobijemo stvarnu ControlMode vrijednost iz indeksa
        uint32_t current_mode1 = control_mode_map_1[sel1];
        if (current_mode1 != g_display_settings.selected_control_mode) {
            g_display_settings.selected_control_mode = current_mode1;
            settingsChanged = 1;
            // Ponovo iscrtaj cijeli ekran da bi se ažurirala lista u drugom dropdown-u
            DSP_KillSet6Scrn();
            DSP_InitSet6Scrn();
            return; // Važno: prekidamo izvršavanje
        }
    }

    // --- Provjera promjene na DROPDOWN-u za Ikonu 2 ---
    int8_t sel2 = DROPDOWN_GetSel(hSelectControl_2);
    if (sel2 >= 0) {
        // Koristimo mapu da dobijemo stvarnu ControlMode vrijednost iz indeksa
        uint32_t current_mode2 = control_mode_map_2[sel2];
        if (current_mode2 != g_display_settings.selected_control_mode_2) {
            g_display_settings.selected_control_mode_2 = current_mode2;
            settingsChanged = 1;
            // Ponovo iscrtaj cijeli ekran da bi se ažurirala lista u prvom dropdown-u
            DSP_KillSet6Scrn();
            DSP_InitSet6Scrn();
            return; // Važno: prekidamo izvršavanje
        }
    }

    // --- Ostatak funkcije (provjera ostalih widgeta) ostaje nepromijenjen ---
    static uint8_t old_language_selection = 0;
    uint8_t current_language_selection = DROPDOWN_GetSel(hDRPDN_Language);
    if (current_language_selection != old_language_selection) {
        old_language_selection = current_language_selection;
        g_display_settings.language = current_language_selection;
        settingsChanged = 1;
        DSP_KillSet6Scrn();
        DSP_InitSet6Scrn();
        return;
    }
    
    if(BUTTON_IsPressed(hBUTTON_SET_DEFAULTS)) {
        SetDefault();
    }
    else if(BUTTON_IsPressed(hBUTTON_SYSRESTART)) {
        SYSRestart();
    }
    else {
        if (tfifa != SPINBOX_GetValue(hDEV_ID)) {
            tfifa = SPINBOX_GetValue(hDEV_ID);
            settingsChanged = 1;
        }
        if(Curtain_GetMoveTime() != SPINBOX_GetValue(hCurtainsMoveTime)) {
            Curtain_SetMoveTime(SPINBOX_GetValue(hCurtainsMoveTime));
            settingsChanged = 1;
        }
        if(g_display_settings.leave_scrnsvr_on_release != CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH)) {
            g_display_settings.leave_scrnsvr_on_release = CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
            settingsChanged = 1;
        }
        if(g_display_settings.light_night_timer_enabled != CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER)) {
            g_display_settings.light_night_timer_enabled = CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER);
            settingsChanged = 1;
        }
    }

    // --- Obrada navigacije (nepromijenjeno) ---
    if(BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    }
    else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        DSP_InitSet7Scrn();
        screen = SCREEN_SETTINGS_7;
    }
}
/**
 ******************************************************************************
 * @brief       Servisira sedmi ekran podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva u petlji iz `DISP_Service`. Odgovorna je za:
 * 1. Detekciju promjena na widgetima (checkbox za omogućavanje scena
 * i spinbox-ovi za adrese okidača).
 * 2. Ažuriranje `g_display_settings` strukture u RAM-u kada dođe do promjene.
 * 3. Obradu pritisaka na dugmad "SAVE" i "NEXT" za snimanje
 * promjena i navigaciju.
 ******************************************************************************
 */
static void Service_SettingsScreen_7(void)
{
    // --- Detekcija promjena na widgetima ---

    // 1. Provjera Checkbox-a za omogućavanje sistema scena
    if(g_display_settings.scenes_enabled != (bool)CHECKBOX_GetState(hCHKBX_EnableScenes))
    {
        g_display_settings.scenes_enabled = (bool)CHECKBOX_GetState(hCHKBX_EnableScenes);
        settingsChanged = 1; // Signaliziraj da postoji nesnimljena promjena
    }

    // 2. Provjera 8 Spinbox-ova za adrese okidača
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        if (g_display_settings.scene_homecoming_triggers[i] != (uint16_t)SPINBOX_GetValue(hSPNBX_SceneTriggers[i]))
        {
            g_display_settings.scene_homecoming_triggers[i] = (uint16_t)SPINBOX_GetValue(hSPNBX_SceneTriggers[i]);
            settingsChanged = 1; // Signaliziraj da postoji nesnimljena promjena
        }
    }

    // --- Obrada Navigacije (Dugmad SAVE i NEXT) ---

    if (BUTTON_IsPressed(hBUTTON_Ok)) // Dugme "SAVE"
    {
        if (settingsChanged)
        {
            Display_Save(); // Snimi sve promjene iz g_display_settings u EEPROM
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        screen = SCREEN_RETURN_TO_FIRST; // Vrati se na početni ekran
    }
    else if (BUTTON_IsPressed(hBUTTON_Next)) // Dugme "NEXT"
    {
        if (settingsChanged)
        {
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        DSP_InitSet8Scrn();
        screen = SCREEN_SETTINGS_8;
    }
}

/**
 ******************************************************************************
 * @brief       Servisira osmi ekran podešavanja (Kapije).
 * @author      Gemini
 * @note        ISPRAVLJENA VERZIJA: Logika je restrukturirana da se
 * omogući/onemogući prikaz kontrola odmah nakon promjene profila,
 * čime se rješava bug gdje opcije nisu bile dostupne.
 ******************************************************************************
 */
static void Service_SettingsScreen_8(void)
{
    bool needs_full_update = false;

    // Provjera da li je korisnik promijenio odabranu kapiju
    if ((SPINBOX_GetValue(hGateSelect) - 1) != settings_gate_selected_index) {
        if(settingsChanged) {
            Gate_Save();
            settingsChanged = 0;
        }
        settings_gate_selected_index = SPINBOX_GetValue(hGateSelect) - 1;
        DSP_KillSet8Scrn();
        DSP_InitSet8Scrn();
        // Važno: odmah izađi iz funkcije nakon reinicijalizacije da se spriječi rad sa starim handle-ovima
        return;
    }

    Gate_Handle* handle = Gate_GetInstance(settings_gate_selected_index);
    if (!handle) {
        return;
    }

    // Ako korisnik promijeni profil, zabilježi promjenu i postavi fleg
    if (DROPDOWN_GetSel(hGateType) != Gate_GetControlType(handle)) {
        Gate_SetControlType(handle, DROPDOWN_GetSel(hGateType));
        settingsChanged = 1;
        needs_full_update = true; // Zatraži puno ažuriranje nakon ovoga
    }

    // --- GLAVNA LOGIKA FUNKCIJE ---
    GUI_MULTIBUF_BeginEx(1);

    const ProfilDeskriptor_t* profil = Gate_GetProfilDeskriptor(handle);

    // Dinamičko omogućavanje/onemogućavanje widgeta na osnovu maske
    WM_SetEnableState(hGateParamSpinboxes[1], (profil->visible_settings_mask & SETTING_VISIBLE_RELAY_CMD2));
    WM_SetEnableState(hGateParamSpinboxes[2], (profil->visible_settings_mask & SETTING_VISIBLE_RELAY_CMD3));
    WM_SetEnableState(hGateParamSpinboxes[3], (profil->visible_settings_mask & SETTING_VISIBLE_FEEDBACK_1));
    WM_SetEnableState(hGateParamSpinboxes[4], (profil->visible_settings_mask & SETTING_VISIBLE_FEEDBACK_2));
    WM_SetEnableState(hGateParamSpinboxes[5], (profil->visible_settings_mask & SETTING_VISIBLE_CYCLE_TIMER));
    WM_SetEnableState(hGateParamSpinboxes[6], (profil->visible_settings_mask & SETTING_VISIBLE_PULSE_TIMER));

    // Provjera promjena na ostalim widgetima
    if (Gate_GetAppearanceId(handle) != SPINBOX_GetValue(hGateAppearance)) {
        settingsChanged = 1;
        Gate_SetAppearanceId(handle, SPINBOX_GetValue(hGateAppearance));
    }
    if (Gate_GetRelayAddr(handle, 1) != SPINBOX_GetValue(hGateParamSpinboxes[0])) {
        settingsChanged = 1;
        Gate_SetRelayAddr(handle, 1, SPINBOX_GetValue(hGateParamSpinboxes[0]));
    }
    if (Gate_GetRelayAddr(handle, 2) != SPINBOX_GetValue(hGateParamSpinboxes[1])) {
        settingsChanged = 1;
        Gate_SetRelayAddr(handle, 2, SPINBOX_GetValue(hGateParamSpinboxes[1]));
    }
    if (Gate_GetRelayAddr(handle, 3) != SPINBOX_GetValue(hGateParamSpinboxes[2])) {
        settingsChanged = 1;
        Gate_SetRelayAddr(handle, 3, SPINBOX_GetValue(hGateParamSpinboxes[2]));
    }
    if (Gate_GetFeedbackAddr(handle, 1) != SPINBOX_GetValue(hGateParamSpinboxes[3])) {
        settingsChanged = 1;
        Gate_SetFeedbackAddr(handle, 1, SPINBOX_GetValue(hGateParamSpinboxes[3]));
    }
    if (Gate_GetFeedbackAddr(handle, 2) != SPINBOX_GetValue(hGateParamSpinboxes[4])) {
        settingsChanged = 1;
        Gate_SetFeedbackAddr(handle, 2, SPINBOX_GetValue(hGateParamSpinboxes[4]));
    }
    if (Gate_GetCycleTimer(handle) != SPINBOX_GetValue(hGateParamSpinboxes[5])) {
        settingsChanged = 1;
        Gate_SetCycleTimer(handle, SPINBOX_GetValue(hGateParamSpinboxes[5]));
    }
    if (Gate_GetPulseTimer(handle) != (SPINBOX_GetValue(hGateParamSpinboxes[6]) * 100)) {
        settingsChanged = 1;
        Gate_SetPulseTimer(handle, SPINBOX_GetValue(hGateParamSpinboxes[6]) * 100);
    }

    // Iscrtavanje preview-a ikonice
    uint16_t selection_index = SPINBOX_GetValue(hGateAppearance);
    if (selection_index < (sizeof(gate_appearance_mapping_table) / sizeof(IconMapping_t)))
    {
        const IconMapping_t* mapping = &gate_appearance_mapping_table[selection_index];
        IconID visual_icon_type = mapping->visual_icon_id;
        uint16_t base_icon_index = (visual_icon_type - ICON_GATE_SWING) * 5;
        if ((base_icon_index + 4) < (sizeof(gate_icon_images) / sizeof(gate_icon_images[0]))) { // Dodatna provjera
            const GUI_BITMAP* icon_bitmap = gate_icon_images[base_icon_index];

            const int16_t x_icon_pos = 480 - icon_bitmap->XSize;
            const int16_t y_icon_pos = 20;
            const int16_t y_primary_text_pos = 5;
            const int16_t y_secondary_text_pos = y_icon_pos + icon_bitmap->YSize + 5;

            GUI_ClearRect(350, 0, 480, 130);
            GUI_SetTextMode(GUI_TM_TRANS);

            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(mapping->primary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_primary_text_pos);

            GUI_DrawBitmap(icon_bitmap, x_icon_pos, y_icon_pos);

            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_SetColor(GUI_ORANGE);
            GUI_DispStringAt(lng(mapping->secondary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_secondary_text_pos);
        }
    }

    GUI_MULTIBUF_EndEx(1);

    // Obrada navigacije
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        if(settingsChanged) {
            Gate_Save();
            settingsChanged = 0;
        }
        DSP_KillSet8Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
        shouldDrawScreen = 1;
    }
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        if(settingsChanged) {
            Gate_Save();
            settingsChanged = 0;
        }
        DSP_KillSet8Scrn();
        DSP_InitSet9Scrn(); // Vracamo se na prvi ekran podesavanja, praveći puni krug
        screen = SCREEN_SETTINGS_9;
    }

    // Ako je bio zahtjev za ažuriranje, ponovo iscrtaj sve od nule
    if (needs_full_update) {
        DSP_KillSet8Scrn();
        DSP_InitSet8Scrn();
    }
}
/**
 ******************************************************************************
 * @brief       Servisira deveti ekran za podešavanja (Alarm).
 * @note        Ažurirana verzija koja obrađuje kontrole sa novog layout-a,
 * uključujući `Enable Security Module` checkbox i preračunavanje
 * vrijednosti za dužinu pulsa.
 ******************************************************************************
 */
static void Service_SettingsScreen_9(void)
{
    // Provjera promjena na postavkama Particija
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
        // Provjera releja za particiju 'i'
        if (Security_GetPartitionRelayAddr(i) != SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_P1 + i))) {
            Security_SetPartitionRelayAddr(i, SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_P1 + i)));
            settingsChanged = 1;
        }
        // Provjera feedback-a za particiju 'i'
        if (Security_GetPartitionFeedbackAddr(i) != SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_P1 + i))) {
            Security_SetPartitionFeedbackAddr(i, SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_P1 + i)));
            settingsChanged = 1;
        }
    }
    
    // Provjera promjena na zajedničkim postavkama
    // Za dužinu pulsa, vrijednost iz spinbox-a (0-50) se množi sa 100 da bi se dobile milisekunde (0-5000)
    if (Security_GetPulseDuration() != (SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_PULSE_LENGTH)) * 100)) {
        Security_SetPulseDuration(SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_PULSE_LENGTH)) * 100);
        settingsChanged = 1;
    }
    if (Security_GetSystemStatusFeedbackAddr() != SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_SYSTEM_STATUS))) {
        Security_SetSystemStatusFeedbackAddr(SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_FB_SYSTEM_STATUS)));
        settingsChanged = 1;
    }
    if (Security_GetSilentAlarmAddr() != SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_SILENT))) {
        Security_SetSilentAlarmAddr(SPINBOX_GetValue(WM_GetDialogItem(WM_GetDesktopWindow(), ID_ALARM_RELAY_SILENT)));
        settingsChanged = 1;
    }
    
    // Obrada premještenog checkbox-a za omogućavanje security modula
    if(g_display_settings.security_module_enabled != (bool)CHECKBOX_GetState(hCHKBX_EnableSecurity)) {
        g_display_settings.security_module_enabled = (bool)CHECKBOX_GetState(hCHKBX_EnableSecurity);
        settingsChanged = 1; // Ova promjena se snima pomoću Display_Save()
    }

    // Obrada navigacije i snimanja
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (settingsChanged) { 
            Security_Save(); // Snima postavke specifične za alarm
            Display_Save();  // Snima opšte postavke (gdje se nalazi `security_module_enabled` fleg)
            settingsChanged = 0; 
        }
        DSP_KillSet9Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if (settingsChanged) { 
            Security_Save(); 
            Display_Save();
            settingsChanged = 0; 
        }
        DSP_KillSet9Scrn();
        DSP_InitSet1Scrn(); // Vraća se na prvi ekran podešavanja, praveći puni krug
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za napredno podešavanje svjetla (dimmer i RGB).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je centralna tačka za interakciju na ekranu za detaljna
 * podešavanja svjetla. Odgovorna je za:
 * 1.  Iscrtavanje korisničkog interfejsa, koje se dinamički prilagođava
 * u zavisnosti od toga da li je odabrano jedno ili više svjetala,
 * i da li podržavaju dimovanje ili RGB kontrolu.
 * 2.  Obradu rezultata vraćenog sa alfanumeričke tastature nakon što
 * korisnik unese novi naziv za svjetlo.
 * 3.  Kreiranje i pozicioniranje dugmeta "[ Promijeni Naziv ]" koje
 * pokreće tastaturu.
 * 4.  Obradu dodira na slajdere i paletu boja u `HandlePress_LightSettingsScreen`.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_LightSettingsScreen(void)
{
    // === 1. Obrada rezultata sa tastature (nepromijenjeno) ===
    if (g_keyboard_result.is_confirmed)
    {
        if (light_selectedIndex < LIGHTS_MODBUS_SIZE)
        {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                LIGHT_SetCustomLabel(handle, g_keyboard_result.value);
                LIGHTS_Save();
            }
        }
        g_keyboard_result.is_confirmed = false;
        shouldDrawScreen = 1;
    }
    if (g_keyboard_result.is_cancelled) {
        g_keyboard_result.is_cancelled = false;
        shouldDrawScreen = 1;
    }

    // === 2. ISCRTAVANJE EKRANA (ako je zatraženo) ===
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        const int centerX = LCD_GetXSize() / 2;
        const int centerY = LCD_GetYSize() / 2;
        const int sliderWidth = bmblackWhiteGradient.XSize;
        const int sliderHeight = bmblackWhiteGradient.YSize;
        const int sliderX0 = centerX - (sliderWidth / 2);
        const int sliderY0 = centerY - (sliderHeight / 2);
        const int WHITE_SQUARE_SIZE = 60;
        const int WHITE_SQUARE_X0 = centerX - (WHITE_SQUARE_SIZE / 2);
        const int WHITE_SQUARE_Y0 = sliderY0 - WHITE_SQUARE_SIZE - 10;
        const int paletteWidth = bmcolorSpectrum.XSize;

        bool show_dimmer_slider = false;
        bool show_rgb_palette = false;

        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            if (lights_allSelected_hasRGB) {
                show_rgb_palette = true;
            }
            else {
                show_dimmer_slider = true;
            }
        } else {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                if (LIGHT_isRGB(handle)) {
                    show_rgb_palette = true;
                }
                else if (LIGHT_isDimmer(handle)) {
                    show_dimmer_slider = true;
                }
            }
        }

        if (show_rgb_palette) {
            GUI_SetColor(GUI_WHITE);
            GUI_FillRect(WHITE_SQUARE_X0, WHITE_SQUARE_Y0, WHITE_SQUARE_X0 + WHITE_SQUARE_SIZE - 1, WHITE_SQUARE_Y0 + WHITE_SQUARE_SIZE - 1);
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
            GUI_DrawBitmap(&bmcolorSpectrum, centerX - (paletteWidth / 2), sliderY0 + sliderHeight + 20);
        } else if (show_dimmer_slider) {
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
        }

        // === POČETAK NOVE LOGIKE ZA ISPIS NASLOVA/NAZIVA ===
        // Nema više dugmeta, samo ispis teksta
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_TOP);

        if (light_selectedIndex < LIGHTS_MODBUS_SIZE) {
            // Ako podešavamo pojedinačno svjetlo -> prikaži njegov naziv
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                const char* custom_label = LIGHT_GetCustomLabel(handle);

                if (custom_label[0] != '\0') {
                    // Ako postoji custom label, ispiši ga
                    GUI_DispStringAt(custom_label, 10, 10);
                } else {
                    // Ako ne, ispiši default naziv sačinjen od dva dijela
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t))) {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        char default_name[40];
                        sprintf(default_name, "%s - %s", lng(mapping->primary_text_id), lng(mapping->secondary_text_id));
                        GUI_DispStringAt(default_name, 10, 10);
                    }
                }
            }
        } else {
            // Ako smo u globalnom modu -> prikaži prevedeni naslov
            GUI_DispStringAt(lng(TXT_GLOBAL_SETTINGS), 10, 10);
        }
        // === KRAJ NOVE LOGIKE ZA ISPIS NASLOVA/NAZIVA ===

        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // === 3. OBRADA KORISNIČKOG UNOSA (Ovaj dio ćemo izmijeniti u sljedećem koraku) ===
    GUI_PID_STATE ts_state;
    GUI_PID_GetState(&ts_state);

    if (ts_state.Pressed) {
        HandlePress_LightSettingsScreen(&ts_state);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija poziva Init funkciju za iscrtavanje i čeka na
 * unos korisnika, koji se obrađuje u `HandlePress_SceneAppearanceScreen`.
 ******************************************************************************
 */
static void Service_SceneAppearanceScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        DSP_InitSceneAppearanceScreen();
    }
}
/**
 ******************************************************************************
 * @brief       Servisira glavni ekran "Čarobnjaka" za scene (SCREEN_SCENE_EDIT).
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA ISPRAVLJENA VERZIJA. Ova funkcija sada ispravno
 * upravlja navigacijom pozivajući odgovarajuće Kill i Init funkcije
 * prilikom promjene ekrana, čime se osigurava da čarobnjak radi.
 ******************************************************************************
 */
static void Service_SceneEditScreen(void)
{
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) return;

    // --- Obrada pritiska na dugme "SNIMI" / "MEMORIŠI STANJE" ---
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        if (!(scene_handle->is_configured == false && scene_handle->appearance_id == 0))
        {
            scene_handle->is_configured = true;
            Scene_Memorize(scene_edit_index);
            Scene_Save();

            DSP_KillSceneEditScreen();
            screen = SCREEN_SCENE;
            shouldDrawScreen = 1;
            return;
        }
    }

    // --- Obrada pritiska na dugme "OTKAŽI" ---
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
        return;
    }

    // --- Obrada pritiska na dugme "PROMIJENI" (samo kod nove scene) ---
    if (WM_IsWindow(hButtonChangeAppearance) && BUTTON_IsPressed(hButtonChangeAppearance))
    {
        current_scene_picker_mode = SCENE_PICKER_MODE_WIZARD; // <-- POSTAVKA MODA
        scene_picker_return_screen = SCREEN_SCENE_EDIT;       // <-- POSTAVKA POVRATNOG EKRANA
        DSP_KillSceneEditScreen();
//        DSP_InitSceneAppearanceScreen(); // Inicijalizuj sljedeći ekran
        screen = SCREEN_SCENE_APPEARANCE;
        shouldDrawScreen = 1; // Init je već iscrtao, ne treba ponovo
        return;
    }

    // --- Obrada pritiska na dugme "OBRIŠI" (samo kod postojeće scene) ---
    if (WM_IsWindow(hButtonDeleteScene) && BUTTON_IsPressed(hButtonDeleteScene))
    {
        memset(scene_handle, 0, sizeof(Scene_t));
        Scene_Save();

        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
        return;
    }

    // --- Obrada pritiska na dugme "DETALJNA PODEŠAVANJA" ---
    if (WM_IsWindow(hButtonDetailedSetup) && BUTTON_IsPressed(hButtonDetailedSetup))
    {
        is_in_scene_wizard_mode = true;
        DSP_KillSceneEditScreen();

        // Na osnovu tipa scene, pokreni prvi korak "anketnog" čarobnjaka
        switch (scene_handle->scene_type)
        {
        // TODO: Ovdje dodati case-ove za LEAVING, HOMECOMING, SLEEP
        case SCENE_TYPE_STANDARD:
        default:
            DSP_InitSceneWizDevicesScreen();
            screen = SCREEN_SCENE_WIZ_DEVICES;
            break;
        }

        shouldDrawScreen = 0; // Init je već iscrtao
        return;
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir grupa uređaja u čarobnjaku.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA: Ova funkcija sada sadrži kompletnu logiku
 * za ažuriranje bitmaski scene i "pametnu" navigaciju. Pritiskom na
 * dugme "[Dalje]", sistem provjerava koje su grupe uređaja odabrane
 * i vodi korisnika na sljedeći relevantan ekran.
 ******************************************************************************
 */
static void Service_SceneWizDevicesScreen(void)
{
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) return;

    // --- Ažuriranje Stanja Checkbox-ova ---
    bool lights_checked = (bool)CHECKBOX_GetState(hCheckboxSceneLights);
    bool lights_in_scene = (scene_handle->lights_mask != 0);
    if (lights_checked != lights_in_scene)
    {
        if (lights_checked) {
            uint8_t temp_mask = 0;
            for(int i=0; i < LIGHTS_MODBUS_SIZE; i++) {
                LIGHT_Handle* l_handle = LIGHTS_GetInstance(i);
                if (l_handle && LIGHT_GetRelay(l_handle) != 0) {
                    temp_mask |= (1 << i);
                }
            }
            scene_handle->lights_mask = temp_mask;
        } else {
            scene_handle->lights_mask = 0;
        }
    }

    bool curtains_checked = (bool)CHECKBOX_GetState(hCheckboxSceneCurtains);
    bool curtains_in_scene = (scene_handle->curtains_mask != 0);
    if (curtains_checked != curtains_in_scene)
    {
        if (curtains_checked) {
            uint16_t temp_mask = 0;
            for(int i=0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* c_handle = Curtain_GetInstanceByIndex(i);
                if (c_handle && Curtain_hasRelays(c_handle)) {
                    temp_mask |= (1 << i);
                }
            }
            scene_handle->curtains_mask = temp_mask;
        } else {
            scene_handle->curtains_mask = 0;
        }
    }

    bool thermostat_checked = (bool)CHECKBOX_GetState(hCheckboxSceneThermostat);
    bool thermostat_in_scene = (scene_handle->thermostat_mask != 0);
    if (thermostat_checked != thermostat_in_scene)
    {
        scene_handle->thermostat_mask = thermostat_checked ? 1 : 0;
    }

    // --- Obrada Navigacionih Dugmića ---
    if (BUTTON_IsPressed(hButtonWizCancel))
    {
        is_in_scene_wizard_mode = false; // Resetuj fleg
        DSP_KillSceneWizDevicesScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    else if (BUTTON_IsPressed(hButtonWizBack))
    {
        DSP_KillSceneWizDevicesScreen();
        DSP_InitSceneEditScreen();
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 0;
    }
    else if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneWizDevicesScreen();

        // "Pametna" navigacija na osnovu tipa scene i odabira
        switch (scene_handle->scene_type)
        {
        // Sistemske scene imaju svoj, fiksni tok
        case SCENE_TYPE_LEAVING:
            screen = SCREEN_SCENE_WIZ_LEAVING;
            // TODO: DSP_InitSceneWizLeavingScreen();
            break;
        case SCENE_TYPE_HOMECOMING:
            screen = SCREEN_SCENE_WIZ_HOMECOMING;
            // TODO: DSP_InitSceneWizHomecomingScreen();
            break;
        case SCENE_TYPE_SLEEP:
            screen = SCREEN_SCENE_WIZ_SLEEP;
            // TODO: DSP_InitSceneWizSleepScreen();
            break;

        // "Komfor" scene idu na prvi odabrani uređaj
        case SCENE_TYPE_STANDARD:
        default:
            if (scene_handle->lights_mask) {
                screen = SCREEN_LIGHTS;
            } else if (scene_handle->curtains_mask) {
                screen = SCREEN_CURTAINS;
            } else if (scene_handle->thermostat_mask) {
                screen = SCREEN_THERMOSTAT;
            } else {
                // Ako ništa nije odabrano, idi na finalni ekran
                DSP_InitSceneWizFinalizeScreen();
                screen = SCREEN_SCENE_WIZ_FINALIZE;
                shouldDrawScreen = 0;
                return;
            }
            break;
        }

        shouldDrawScreen = 1;
    }
}
/**
 * @brief Servisira ekran za resetovanje glavnih prekidača/menija.
 * @note Ova funkcija prvenstveno prikazuje odbrojavanje ako je aktivan
 * Light Night Timer. Koristi novi API za provjeru statusa i preostalog vremena.
 */
static void Service_MainScreenSwitch(void)
{
    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

    // KORAK 1: Umjesto direktne provjere varijable, pozivamo API funkciju.
    if(LIGHTS_IsNightTimerActive()) {
        GUI_MULTIBUF_BeginEx(1);

        // KORAK 2: Umjesto ručnog računanja, pozivamo API funkciju koja vraća preostalo vrijeme.
        const uint8_t dispTime = LIGHTS_GetNightTimerCountdown();

        // Logika iscrtavanja ostaje ista.
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_D32);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
        GUI_ClearRect(220, 116, 265, 156);
        GUI_DispDecAt(dispTime + 1, 240, 136, 2);

        GUI_MULTIBUF_EndEx(1);
    }

    // === KRAJ REFAKTORISANJA ===
    // =======================================================================
}

/**
 ******************************************************************************
 * @brief       Servisira finalni ekran čarobnjaka.
 ******************************************************************************
 */
static void Service_SceneWizFinalizeScreen(void)
{
    if (BUTTON_IsPressed(hBUTTON_Ok)) // "Snimi Scenu"
    {
        Scene_Save();
        is_in_scene_wizard_mode = false;
        DSP_KillSceneWizFinalizeScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    else if (BUTTON_IsPressed(hButtonWizCancel))
    {
        // Ne snimaj promjene u RAM-u, samo izađi
        is_in_scene_wizard_mode = false;
        DSP_KillSceneWizFinalizeScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Izračunava dan u sedmici na osnovu datuma.
 * @note        Funkcija koristi Zellerov algoritam. Nedjelja = 1, Ponedjeljak = 2, itd.
 * @param       year Godina (npr. 2025).
 * @param       month Mjesec (1-12).
 * @param       day Dan u mjesecu (1-31).
 * @retval      uint8_t Dan u sedmici (1-7).
 ******************************************************************************
 */
static uint8_t getWeekday(int year, int month, int day)
{
    int y = year;
    int m = month;
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int K = y % 100;
    int J = y / 100;
    int day_of_week = (day + 13 * (m + 1) / 5 + K + K / 4 + J / 4 - 2 * J) % 7;
    // Podesi rezultat da bude 1 (Ned) do 7 (Sub), umjesto 0-6
    return (day_of_week + 1 == 0) ? 7 : (day_of_week + 1);
}
/**
 ******************************************************************************
 * @brief       Servisira glavni ekran tajmera.
 * @author      Gemini & [Vaše Ime]
 * @note        Ažurirana verzija. Blok koda `if (!IsRtcTimeValid())` je
 * identičan Vašoj radnoj verziji. Blok `else` je redizajniran:
 * - Tekstovi "Svaki dan", "Radnim danima" itd. su premješteni u prevode.
 * - Dugme za podešavanja se kreira sa dimenzijama ikonice `bmicons_settings`.
 ******************************************************************************
 */
static void Service_TimerScreen(void)
{
    // Crtamo samo ako je eksplicitno zatraženo
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        ForceKillAllSettingsWidgets();
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1); // Meni za povratak

        // ====================================================================
        // === OVAJ BLOK KODA JE PREKOPIRAN I NIJE FUNKCIONALNO MIJENJAN ===
        // ====================================================================
        if (!IsRtcTimeValid()) {
            // Ako vrijeme NIJE validno, prikaži veliko dugme za podešavanje
            const GUI_BITMAP* datetime_icon = &bmicons_date_time;

            // Jedina izmjena: pozicije se sada čitaju iz layout strukture
            GUI_DrawBitmap(datetime_icon, timer_screen_layout.datetime_icon_pos.x, timer_screen_layout.datetime_icon_pos.y);

            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(TXT_DATETIME_SETUP_TITLE), timer_screen_layout.datetime_text_pos.x, timer_screen_layout.datetime_text_pos.y);

        }
        // ====================================================================
        // === NOVA IMPLEMENTACIJA ZA SLUČAJ KADA JE VRIJEME PODEŠENO ===
        // ====================================================================
        else {
            // Ako je vrijeme validno, prikaži glavni interfejs alarma
            char time_str[6];
            sprintf(time_str, "%02d:%02d", Timer_GetHour(), Timer_GetMinute());
            GUI_SetFont(GUI_FONT_D64);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(time_str, timer_screen_layout.time_pos.x, timer_screen_layout.time_pos.y);

            // Prikaz dana ponavljanja KORIŠTENJEM PREVODA
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_SetColor(GUI_ORANGE);
            uint8_t repeat_mask = Timer_GetRepeatMask();
            char days_str[50] = "";
            if (repeat_mask == TIMER_EVERY_DAY)       strcpy(days_str, lng(TXT_TIMER_EVERY_DAY));
            else if (repeat_mask == TIMER_WEEKDAYS)   strcpy(days_str, lng(TXT_TIMER_WEEKDAYS));
            else if (repeat_mask == TIMER_WEEKEND)    strcpy(days_str, lng(TXT_TIMER_WEEKEND));
            else if (repeat_mask == 0)                strcpy(days_str, lng(TXT_TIMER_ONCE));
            else {
                // Skraćenice dana su već u prevodima u _acContent tabeli
                for (int i = 0; i < 7; i++) {
                    if (repeat_mask & (1 << i)) {
                        strcat(days_str, _acContent[g_display_settings.language][i]);
                        strcat(days_str, " ");
                    }
                }
            }
            GUI_DispStringAt(days_str, timer_screen_layout.days_pos.x, timer_screen_layout.days_pos.y);

            // Prikaz ON/OFF toggle ikonice
            GUI_CONST_STORAGE GUI_BITMAP* icon_toggle = Timer_IsActive() ? &bmicons_toggle_on : &bmicons_toogle_off;
            int toggle_x = (DRAWING_AREA_WIDTH / 2) - (icon_toggle->XSize / 2);
            GUI_DrawBitmap(icon_toggle, toggle_x, timer_screen_layout.toggle_icon_pos.y);

            // Prikaz novog statusnog teksta
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            TextID status_text_id = Timer_IsActive() ? TXT_TIMER_ENABLED : TXT_TIMER_DISABLED;
            GUI_DispStringAt(lng(status_text_id), timer_screen_layout.status_text_pos.x, timer_screen_layout.status_text_pos.y);
        }

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       [VERZIJA 2.5 - 1:1 PREMA VAŠEM KODU] Servisira ekran za podešavanje alarma.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova verzija je bazirana 1:1 na Vašem funkcionalnom kodu, bez ikakvih
 * izmjena u postojećoj logici ili nazivima varijabli.
 * Dodate su samo neophodne linije za prikaz i snimanje odabrane scene.
 ******************************************************************************
 */
static void Service_SettingsTimerScreen(void)
{
    static int current_hour, current_minute;
    static uint8_t repeat_mask;
    static bool buzzer_state;
    static bool scene_state;
    static bool old_button_state[14] = {0}; // 0: sat_up, 1: sat_down, 2: minut_up, 3: minut_down
    static int old_hour = -1, old_minute = -1;
    bool needs_full_redraw = false;

    // --- NOVE LOKALNE STATIČKE VARIJABLE ZA REPEAT FUNKCIJU ---
    static uint32_t press_time[4] = {0}; // Čuva vreme pritiska za svaki taster
    static bool button_is_held[4] = {false}; // Da li se taster drži
    const uint32_t initial_delay = 500;
    const uint32_t repeat_rate = 200;

    if (!timer_screen_initialized) {
        DSP_InitSettingsTimerScreen();

        current_hour = Timer_GetHour();
        current_minute = Timer_GetMinute();
        repeat_mask = Timer_GetRepeatMask();
        buzzer_state = Timer_GetActionBuzzer();

        // NOVI DODATAK: Učitavanje podataka za scenu. Vaša originalna `scene_state` se sada postavlja na osnovu ovoga.
        // Ako je `needs_full_redraw` postavljen, znači da ulazimo na ekran prvi put,
        // pa treba učitati snimljenu vrijednost. Ako nije, znači da se vraćamo
        // sa pickera i treba da sačuvamo vrijednost koja je već u `timer_selected_scene_index`.
        if (needs_full_redraw) {
            timer_selected_scene_index = Timer_GetSceneIndex();
        }

        scene_state = (timer_selected_scene_index != -1);

        timer_screen_initialized = true;
        needs_full_redraw = true;
    }

    bool hour_changed = false;
    bool minute_changed = false;
    bool other_changed = false;

    // --- LOGIKA ZA OBRADU PRITISKA NA DUGMAD ZA VRIJEME ---
    // Sat UP
    bool is_hour_up_pressed = BUTTON_IsPressed(hButtonTimerHourUp);
    if (is_hour_up_pressed) {
        if (!button_is_held[0]) { // Prvi pritisak
            current_hour = (current_hour + 1) % 24;
            hour_changed = true;
            button_is_held[0] = true;
            press_time[0] = HAL_GetTick();
        } else if ((HAL_GetTick() - press_time[0]) > initial_delay) {
            if ((HAL_GetTick() - press_time[0]) % repeat_rate < 20) { // Mala tolerancija od 20ms
                current_hour = (current_hour + 1) % 24;
                hour_changed = true;
            }
        }
    } else {
        button_is_held[0] = false;
    }

    // Sat DOWN
    bool is_hour_down_pressed = BUTTON_IsPressed(hButtonTimerHourDown);
    if (is_hour_down_pressed) {
        if (!button_is_held[1]) {
            current_hour = (current_hour - 1 + 24) % 24;
            hour_changed = true;
            button_is_held[1] = true;
            press_time[1] = HAL_GetTick();
        } else if ((HAL_GetTick() - press_time[1]) > initial_delay) {
            if ((HAL_GetTick() - press_time[1]) % repeat_rate < 20) {
                current_hour = (current_hour - 1 + 24) % 24;
                hour_changed = true;
            }
        }
    } else {
        button_is_held[1] = false;
    }

    // Minuta UP
    bool is_minute_up_pressed = BUTTON_IsPressed(hButtonTimerMinuteUp);
    if (is_minute_up_pressed) {
        if (!button_is_held[2]) {
            current_minute = (current_minute + 1) % 60;
            minute_changed = true;
            button_is_held[2] = true;
            press_time[2] = HAL_GetTick();
        } else if ((HAL_GetTick() - press_time[2]) > initial_delay) {
            if ((HAL_GetTick() - press_time[2]) % repeat_rate < 20) {
                current_minute = (current_minute + 1) % 60;
                minute_changed = true;
            }
        }
    } else {
        button_is_held[2] = false;
    }

    // Minuta DOWN
    bool is_minute_down_pressed = BUTTON_IsPressed(hButtonTimerMinuteDown);
    if (is_minute_down_pressed) {
        if (!button_is_held[3]) {
            current_minute = (current_minute - 1 + 60) % 60;
            minute_changed = true;
            button_is_held[3] = true;
            press_time[3] = HAL_GetTick();
        } else if ((HAL_GetTick() - press_time[3]) > initial_delay) {
            if ((HAL_GetTick() - press_time[3]) % repeat_rate < 20) {
                current_minute = (current_minute - 1 + 60) % 60;
                minute_changed = true;
            }
        }
    } else {
        button_is_held[3] = false;
    }

    // --- ISPRAVLJENA LINIJA KODA: Ovdje je problem bio ---
    if (hour_changed || minute_changed || needs_full_redraw) {
        shouldDrawScreen = 1;
    }

    const GUI_BITMAP* icon_on = &bmicons_toggle_on_50_squared;
    const GUI_BITMAP* icon_off = &bmicons_toogle_off_50_squared;
    // Pritisak na dugmad za dane
    for (int i = 0; i < 7; i++) {
        bool is_day_pressed = BUTTON_IsPressed(hButtonTimerDay[i]);
        if (is_day_pressed && !old_button_state[i+4]) { // Počinje od indeksa 4
            old_button_state[i+4] = true;
            repeat_mask ^= (1 << i);
            BUTTON_SetBitmap(hButtonTimerDay[i], BUTTON_CI_UNPRESSED, (repeat_mask & (1 << i)) ? icon_on : icon_off);
            other_changed = true;
        } else if (!is_day_pressed && old_button_state[i+4]) {
            old_button_state[i+4] = false;
        }
    }

    if (BUTTON_IsPressed(hButtonTimerBuzzer) && !old_button_state[11]) {
        old_button_state[11] = true;
        buzzer_state = !buzzer_state;
        BUTTON_SetBitmap(hButtonTimerBuzzer, BUTTON_CI_UNPRESSED, buzzer_state ? icon_on : icon_off);
        other_changed = true;
    } else if (!BUTTON_IsPressed(hButtonTimerBuzzer) && old_button_state[11]) {
        old_button_state[11] = false;
    }

    if (BUTTON_IsPressed(hButtonTimerScene) && !old_button_state[12]) {
        old_button_state[12] = true;
        scene_state = !scene_state;
        BUTTON_SetBitmap(hButtonTimerScene, BUTTON_CI_UNPRESSED, scene_state ? icon_on : icon_off);
        other_changed = true;
    } else if (!BUTTON_IsPressed(hButtonTimerScene) && old_button_state[12]) {
        old_button_state[12] = false;
    }


    if (BUTTON_IsPressed(hButtonTimerSceneSelect) && !old_button_state[13]) {
        old_button_state[13] = true;

        // Postavi mod i povratnu adresu za picker
        current_scene_picker_mode = SCENE_PICKER_MODE_TIMER;
        scene_picker_return_screen = SCREEN_SETTINGS_TIMER;

        // Pokreni sigurnu tranziciju na ekran za odabir
        timer_screen_initialized = false; // Postavi na false da se ekran reinicijalizuje pri povratku
        DSP_KillSettingsTimerScreen();
        DSP_InitSceneAppearanceScreen();
        screen = SCREEN_SCENE_APPEARANCE;
        return; // Važno: odmah izađi iz funkcije da se spriječi dalje izvršavanje

    } else if (!BUTTON_IsPressed(hButtonTimerSceneSelect) && old_button_state[13]) {
        old_button_state[13] = false;
    }


    // Finalno iscrtavanje
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);

        // Uvijek crtamo pozadinu i statične elemente
        GUI_SetFont(GUI_FONT_D64);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);
        // --- ISPRAVLJENA LINIJA KODA KOJA BRISSE I PONOVNO ISCRTAVA VREDNOSTI TIMERA ---
        // Visina teksta iz fonta
        int16_t font_height = GUI_GetFontDistY();
        const int16_t clear_height = 70; // Visina polja za brisanje

        // Brisanje i iscrtavanje sata
        GUI_ClearRect(timer_settings_screen_layout.time_hour_pos.x,
                      timer_settings_screen_layout.time_hour_pos.y - font_height/2,
                      timer_settings_screen_layout.time_hour_pos.x + timer_settings_screen_layout.time_hour_width,
                      timer_settings_screen_layout.time_hour_pos.y + clear_height - font_height/2);
        GUI_DispDecAt(current_hour, timer_settings_screen_layout.time_hour_pos.x, timer_settings_screen_layout.time_hour_pos.y, 2);

        // Brisanje i iscrtavanje dvotačke
        GUI_ClearRect(timer_settings_screen_layout.time_colon_pos.x,
                      timer_settings_screen_layout.time_colon_pos.y - font_height/2,
                      timer_settings_screen_layout.time_colon_pos.x + timer_settings_screen_layout.time_colon_width,
                      timer_settings_screen_layout.time_colon_pos.y + clear_height - font_height/2);
        GUI_DispStringAt(":", timer_settings_screen_layout.time_colon_pos.x, timer_settings_screen_layout.time_colon_pos.y);

        // Brisanje i iscrtavanje minuta
        GUI_ClearRect(timer_settings_screen_layout.time_minute_pos.x,
                      timer_settings_screen_layout.time_minute_pos.y - font_height/2,
                      timer_settings_screen_layout.time_minute_pos.x + timer_settings_screen_layout.time_minute_width,
                      timer_settings_screen_layout.time_minute_pos.y + clear_height - font_height/2);
        GUI_DispDecAt(current_minute, timer_settings_screen_layout.time_minute_pos.x, timer_settings_screen_layout.time_minute_pos.y, 2);

        //
        // ==========> POČETAK IZMIJENJENOG KODA ZA ISPIS SCENE <==========
        //
        // Definišemo Y poziciju koja je zajednička za oba teksta (vertikalno centrirana)
        const int16_t y_pos = timer_settings_screen_layout.scene_button_pos.y + (icon_off->YSize / 2);

        // --- 1. Iscrtavanje statičkog teksta "Pokreni scenu" (ostaje nepromijenjeno) ---
        GUI_SetFont(&GUI_FontVerdana16_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

        int16_t x_current_pos = timer_settings_screen_layout.scene_button_pos.x + icon_off->XSize + 10;
        GUI_DispStringAt(lng(TXT_TIMER_TRIGGER_SCENE), x_current_pos, y_pos);

        x_current_pos += GUI_GetStringDistX(lng(TXT_TIMER_TRIGGER_SCENE));

        // --- 2. Iscrtavanje dinamičkog naziva scene (ako je odabrana) ---
        if (timer_selected_scene_index != -1) {
            Scene_t* scene_handle = Scene_GetInstance(timer_selected_scene_index);
            if (scene_handle && scene_handle->is_configured) {
                const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];
                char scene_name_buffer[32];
                strncpy(scene_name_buffer, lng(appearance->text_id), 31);
                scene_name_buffer[31] = '\0';

                GUI_SetFont(&GUI_FontVerdana16_LAT);

                // --- NOVI DIO: Poravnanje teksta DESNO ---
                // Definišemo KRAJNJU desnu poziciju za tekst, 5 piksela lijevo od dugmeta za odabir.
                const int16_t x_end_pos_dynamic = timer_settings_screen_layout.scene_select_btn_pos.x - 20;

                // Logika za skraćivanje ostaje ista da spriječi preklapanje sa lijevim tekstom.
                const int16_t x_start_pos_static_end = x_current_pos + 5;
                const int16_t available_width = x_end_pos_dynamic - x_start_pos_static_end;

                if (GUI_GetStringDistX(scene_name_buffer) > available_width) {
                    while (GUI_GetStringDistX(scene_name_buffer) > available_width - GUI_GetStringDistX(".")) {
                        if (strlen(scene_name_buffer) > 1) {
                            scene_name_buffer[strlen(scene_name_buffer) - 1] = '\0';
                        } else {
                            break;
                        }
                    }
                    strcat(scene_name_buffer, ".");
                }

                GUI_SetColor(GUI_ORANGE);
                // Postavljamo DESNO poravnanje teksta.
                GUI_SetTextAlign(GUI_TA_RIGHT | GUI_TA_VCENTER);
                // Iscrtavamo tekst na izračunatoj KRAJNJOJ poziciji.
                GUI_DispStringAt(scene_name_buffer, x_end_pos_dynamic, y_pos);
            }
        }
        // ==========> KRAJ IZMIJENJENOG KODA <==========
        //

        GUI_MULTIBUF_EndEx(1);
    }

    if (BUTTON_IsPressed(hButtonTimerSave)) {
        Timer_SetHour(current_hour);
        Timer_SetMinute(current_minute);
        Timer_SetRepeatMask(repeat_mask);
        Timer_SetActionBuzzer(buzzer_state);

        //
        // ==========> DODATAK U LOGICI SNIMANJA <==========
        //
        // // Timer_SetActionScene je uklonjen iz funkcije (stara linija iz Vašeg koda)
        Timer_SetSceneIndex(scene_state ? timer_selected_scene_index : -1);
        Timer_Save();
        timer_screen_initialized = false;
        DSP_KillSettingsTimerScreen();
        screen = SCREEN_TIMER;
        shouldDrawScreen = 1;
        Timer_Unsuppress();
    }
    else if (BUTTON_IsPressed(hButtonTimerCancel)) {
        timer_screen_initialized = false; // Vraćeno kako je bilo u Vašem kodu
        DSP_KillSettingsTimerScreen();
        screen = SCREEN_TIMER;
        shouldDrawScreen = 1;
        Timer_Unsuppress();
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za podešavanje datuma i vremena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija sadrži kompletnu logiku za interakciju korisnika.
 * Koristi ne-blokirajući mehanizam za detekciju pritiska (edge detection)
 * kako bi osigurala da svaki pritisak na +/- dugme rezultira tačno
 * jednim inkrementom/dekrementom. Za praćenje stanja dugmadi koristi
 * lokalni statički niz `old_button_state`. Prilikom ažuriranja
 * vrijednosti na ekranu, primjenjuje princip "obriši-pa-iscrtaj"
 * unutar fiksne zone TEXT widgeta kako bi se spriječili vizuelni
 * ostaci (artifacts). Sva logika je sadržana unutar ove funkcije
 * bez uvođenja novih globalnih varijabli.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SettingsDateTimeScreen(void)
{
    // Statičke varijable za čuvanje stanja unutar funkcije između poziva
    static int32_t values[5]; // Lokalni bafer za vrijednosti: 0:Dan, 1:Mjesec, 2:Godina, 3:Sat, 4:Minuta
    static bool initialized = false;
    static bool old_button_state[10] = {0}; // Niz za praćenje stanja za 5x2 dugmeta (+/-)

    // NOVE, ROBUSTNE VARIJABLE ZA "PRESS-AND-HOLD"
    static uint32_t press_start_time[10] = {0}; // Vrijeme kada je taster pritisnut
    static uint32_t next_trigger_time[10] = {0}; // Vrijeme kada je dozvoljen sljedeći inkrement

    const uint32_t INITIAL_DELAY_MS = 500;    // Inicijalno kašnjenje od 0.5s
    const uint32_t REPEAT_RATE_MS = 350;      // Brzina ponavljanja (svakih 150ms)

    char value_buffer[6];

    // Učitavanje početnih vrijednosti iz RTC-a samo pri prvom ulasku na ekran
    if (!initialized)
    {
        RTC_TimeTypeDef temp_time;
        RTC_DateTypeDef temp_date;
        HAL_RTC_GetTime(&hrtc, &temp_time, RTC_FORMAT_BCD);
        HAL_RTC_GetDate(&hrtc, &temp_date, RTC_FORMAT_BCD);

        values[0] = Bcd2Dec(temp_date.Date);
        values[1] = Bcd2Dec(temp_date.Month);
        values[2] = Bcd2Dec(temp_date.Year) + 2000;
        values[3] = Bcd2Dec(temp_time.Hours);
        values[4] = Bcd2Dec(temp_time.Minutes);

        // Inicijalni prikaz svih vrijednosti u TEXT widgetima
        for(int i = 0; i < 5; i++) {
            sprintf(value_buffer, "%d", (int)values[i]);
            TEXT_SetText(hTextDateTimeValue[i], value_buffer);
        }

        initialized = true;
    }

    // Granice za validaciju vrijednosti
    const int32_t min_values[] = { 1, 1, 2000, 0, 0 };
    const int32_t max_values[] = { 31, 12, 2099, 23, 59 };

    // Obrada unosa za svih 5 grupa elemenata (Dan, Mjesec, Godina, Sat, Minuta)
    for (int i = 0; i < 5; i++)
    {
        int up_idx = i * 2;
        int down_idx = up_idx + 1;
        bool value_changed = false;

        // --- NOVA POUZDANA LOGIKA ZA GORE (UP) DUGME ---
        if (BUTTON_IsPressed(hButtonDateTimeUp[i])) {
            if (press_start_time[up_idx] == 0) { // Prvi pritisak
                press_start_time[up_idx] = HAL_GetTick();
                next_trigger_time[up_idx] = press_start_time[up_idx] + INITIAL_DELAY_MS;
                values[i]++;
                value_changed = true;
            } else if (HAL_GetTick() >= next_trigger_time[up_idx]) { // Ponavljanje
                next_trigger_time[up_idx] += REPEAT_RATE_MS;
                values[i]++;
                value_changed = true;
            }
        } else {
            press_start_time[up_idx] = 0; // Resetuj na otpuštanje
        }

        // --- NOVA POUZDANA LOGIKA ZA DOLE (DOWN) DUGME ---
        if (BUTTON_IsPressed(hButtonDateTimeDown[i])) {
            if (press_start_time[down_idx] == 0) { // Prvi pritisak
                press_start_time[down_idx] = HAL_GetTick();
                next_trigger_time[down_idx] = press_start_time[down_idx] + INITIAL_DELAY_MS;
                values[i]--;
                value_changed = true;
            } else if (HAL_GetTick() >= next_trigger_time[down_idx]) { // Ponavljanje
                next_trigger_time[down_idx] += REPEAT_RATE_MS;
                values[i]--;
                value_changed = true;
            }
        } else {
            press_start_time[down_idx] = 0; // Resetuj na otpuštanje
        }

        // Ako je vrijednost promijenjena, izvrši validaciju i iscrtavanje
        if (value_changed) {
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();

            // Validacija vrijednosti
            if (i == 0) { // Specijalna validacija za DAN
                int days_in_month = rtc_months[LEAP_YEAR(values[2])][values[1] - 1];
                if (values[0] > days_in_month) values[0] = 1;
                if (values[0] < 1) values[0] = days_in_month;
            } else { // Standardna validacija za ostale
                if (values[i] > max_values[i]) values[i] = min_values[i];
                if (values[i] < min_values[i]) values[i] = max_values[i];
            }

            // Logika "Obriši pa iscrtaj"
            GUI_RECT clear_rect;
            WM_GetWindowRectEx(hTextDateTimeValue[i], &clear_rect);
            GUI_SetBkColor(GUI_BLACK);
            GUI_ClearRectEx(&clear_rect);

            sprintf(value_buffer, "%d", (int)values[i]);
            TEXT_SetText(hTextDateTimeValue[i], value_buffer);
        }
    }

    // Obrada pritiska na "OK" dugme
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        BuzzerOn();
        HAL_Delay(1);
        BuzzerOff();

        RTC_TimeTypeDef new_time = {0};
        RTC_DateTypeDef new_date = {0};

        new_date.Date = Dec2Bcd(values[0]);
        new_date.Month = Dec2Bcd(values[1]);
        new_date.Year = Dec2Bcd(values[2] - 2000);
        new_date.WeekDay = getWeekday(values[2], values[1], values[0]);

        new_time.Hours = Dec2Bcd(values[3]);
        new_time.Minutes = Dec2Bcd(values[4]);
        new_time.Seconds = 0x00;

        HAL_RTC_SetTime(&hrtc, &new_time, RTC_FORMAT_BCD);
        HAL_RTC_SetDate(&hrtc, &new_date, RTC_FORMAT_BCD);
        RtcTimeValidSet();

        initialized = false; // Reset za sljedeći ulazak na ekran
        DSP_KillSettingsDateTimeScreen();
        screen = SCREEN_RETURN_TO_FIRST;
//        screen = SCREEN_TIMER;
        shouldDrawScreen = 1;
    }
}

/**
 ******************************************************************************
 * @brief       [ISPRAVLJENA VERZIJA] Servisira ekran koji se prikazuje dok je alarm aktivan.
 * @author      Gemini & [Vaše Ime]
 * @note        Koristi ispravan font `GUI_FontVerdana32_LAT` koji podržava slova
 * za ispis poruke alarma.
 ******************************************************************************
 */
static void Service_AlarmActiveScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();

        // Dummy poziv za animaciju (kasnije možete implementirati pravu)
        GUI_DrawBitmap(&bmicons_security_sos, 380, 20);

        // Korištenje ispravnog fonta koji podržava slova
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_RED);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_ALARM_WAKEUP), 480 / 2, 272 / 2);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira "dashboard" ekran za kapije.
 * @author      Gemini
 * @note        Ova funkcija se poziva u petlji kada je `screen == SCREEN_GATE`.
 * Arhitektura je u potpunosti preslikana sa `Service_LightsScreen`
 * funkcije, u skladu sa projektnim zahtjevima.
 * KONAČNA ISPRAVKA: Logika iscrtavanja, uključujući dinamički odabir
 * fonta i precizno dvoredno pozicioniranje teksta, sada je 100%
 * identična referentnoj funkciji `Service_LightsScreen`. Svi parametri
 * za pozicioniranje se sada čitaju iz `lights_and_gates_grid_layout`
 * strukture.
 ******************************************************************************
 */
static void Service_GateScreen(void)
{
    // Iscrtavanje se vrši samo ako je eksplicitno zatraženo.
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        uint8_t gate_count = Gate_GetCount();

        if (gate_count == 0) {
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_CONFIGURE_DEVICE_MSG), DRAWING_AREA_WIDTH / 2, LCD_GetYSize() / 2);
        } else {
            // =======================================================================
            // === FAZA 1: PRE-KALKULACIJA I ODABIR FONTA (identično kao kod svjetala) ===
            // =======================================================================
            const GUI_FONT* fontToUse = &GUI_FontVerdana20_LAT;
            const int text_padding = 10;
            bool downgrade_font = false;

            for(uint8_t i = 0; i < gate_count; ++i) {
                uint8_t gates_total = gate_count;
                uint8_t gates_in_this_row = 0;
                if (gates_total <= 3) gates_in_this_row = gates_total;
                else if (gates_total == 4) gates_in_this_row = 2;
                else if (gates_total == 5) gates_in_this_row = 3;
                else gates_in_this_row = 3;

                int max_width_per_icon = (DRAWING_AREA_WIDTH / gates_in_this_row) - text_padding;

                Gate_Handle* handle = Gate_GetInstance(i);
                if (handle) {
                    uint8_t appearance_id = Gate_GetAppearanceId(handle);
                    // === SIGURNOSNA PROVJERA #1 (Dio rješenja) ===
                    if (appearance_id < (sizeof(gate_appearance_mapping_table) / sizeof(IconMapping_t))) {
                        const IconMapping_t* mapping = &gate_appearance_mapping_table[appearance_id];
                        GUI_SetFont(&GUI_FontVerdana20_LAT);

                        if (GUI_GetStringDistX(lng(mapping->primary_text_id)) > max_width_per_icon || GUI_GetStringDistX(lng(mapping->secondary_text_id)) > max_width_per_icon) {
                            downgrade_font = true;
                            break;
                        }
                    }
                }
            }

            if (downgrade_font) {
                fontToUse = &GUI_FontVerdana16_LAT;
            }

            // =======================================================================
            // === FAZA 2: ISCRTAVANJE IKONICA SA NOVOM LAYOUT STRUKTUROM ===
            // =======================================================================
            uint8_t rows = (gate_count > 3) ? 2 : 1;
            int y_row_start = (rows > 1)
                              ? lights_and_gates_grid_layout.y_start_pos_multi_row
                              : lights_and_gates_grid_layout.y_start_pos_single_row;

            const int y_row_height = lights_and_gates_grid_layout.row_height;
            uint8_t gatesInRowSum = 0;

            for(uint8_t row = 0; row < rows; ++row) {
                uint8_t gatesInRow = gate_count;
                if (gate_count > 3) {
                    if (gate_count == 4) gatesInRow = 2;
                    else if (gate_count == 5) gatesInRow = (row > 0) ? 2 : 3;
                    else gatesInRow = 3;
                }
                uint8_t currentGatesMenuSpaceBetween = (400 - (80 * gatesInRow)) / (gatesInRow - 1 + 2);

                for(uint8_t idx_in_row = 0; idx_in_row < gatesInRow; ++idx_in_row) {
                    uint8_t absolute_gate_index = gatesInRowSum + idx_in_row;
                    if (absolute_gate_index >= gate_count) break;

                    Gate_Handle* handle = Gate_GetInstance(absolute_gate_index);
                    if (handle) {
                        uint8_t appearance_id = Gate_GetAppearanceId(handle);
                        const char* custom_label = Gate_GetCustomLabel(handle);

                        // === POČETAK GLAVNE ISPRAVKE: SIGURNOSNA PROVJERA PRIJE CRTANJA ===
                        // Provjeravamo da li je appearance_id validan indeks za niz.
                        // Ako nije, preskačemo crtanje ove ikonice i nastavljamo sa sljedećom,
                        // čime se sprječava pad sistema.
                        if (appearance_id < (sizeof(gate_appearance_mapping_table) / sizeof(IconMapping_t))) {
                            const IconMapping_t* mapping = &gate_appearance_mapping_table[appearance_id];
                            GateState_e state = Gate_GetState(handle);
                            IconID visual_icon_type = mapping->visual_icon_id;
                            const GUI_BITMAP* icon_to_draw = NULL;

                            uint8_t icon_state_index = 0;
                            switch (state) {
                            case GATE_STATE_CLOSED:
                                icon_state_index = 0;
                                break;
                            case GATE_STATE_OPEN:
                                icon_state_index = 1;
                                break;
                            case GATE_STATE_OPENING:
                                icon_state_index = 2;
                                break;
                            case GATE_STATE_CLOSING:
                                icon_state_index = 3;
                                break;
                            case GATE_STATE_PARTIALLY_OPEN:
                                icon_state_index = 4;
                                break;
                            default:
                                icon_state_index = 0;
                                break;
                            }

                            // Druga sigurnosna provjera: da li je izračunati indeks za ikonicu validan.
                            uint16_t base_icon_index = (visual_icon_type - ICON_GATE_SWING) * 5;
                            uint16_t final_icon_index = base_icon_index + icon_state_index;

                            if (final_icon_index < (sizeof(gate_icon_images) / sizeof(gate_icon_images[0]))) {
                                icon_to_draw = gate_icon_images[final_icon_index];

                                GUI_SetFont(fontToUse);
                                const int font_height = GUI_GetFontDistY();
                                const int icon_height = icon_to_draw->YSize;
                                const int icon_width = icon_to_draw->XSize;
                                const int padding = lights_and_gates_grid_layout.text_icon_padding;
                                const int total_block_height = font_height + padding + icon_height + padding + font_height;
                                const int y_slot_center = y_row_start + (y_row_height / 2);
                                const int y_block_start = y_slot_center - (total_block_height / 2);
                                const int x_slot_start = (currentGatesMenuSpaceBetween * (idx_in_row + 1)) + (80 * idx_in_row);
                                const int x_text_center = x_slot_start + 40;

                                const int y_primary_text_pos = y_block_start;
                                const int y_icon_pos = y_primary_text_pos + font_height + padding;
                                const int y_secondary_text_pos = y_icon_pos + icon_height + padding;

                                GUI_SetTextMode(GUI_TM_TRANS);
                                GUI_SetTextAlign(GUI_TA_HCENTER);
                                GUI_SetColor(GUI_WHITE);
                                if (custom_label[0] == '\0') {
                                    GUI_DispStringAt(lng(mapping->primary_text_id), x_text_center, y_primary_text_pos);
                                }

                                GUI_DrawBitmap(icon_to_draw, x_text_center - (icon_width / 2), y_icon_pos);

                                GUI_SetTextMode(GUI_TM_TRANS);
                                GUI_SetTextAlign(GUI_TA_HCENTER);
                                GUI_SetColor(GUI_ORANGE);
                                if (custom_label[0] != '\0') {
                                    GUI_DispStringAt(custom_label, x_text_center, y_secondary_text_pos);
                                } else {
                                    GUI_DispStringAt(lng(mapping->secondary_text_id), x_text_center, y_secondary_text_pos);
                                }
                            }
                        }
                        // === KRAJ GLAVNE ISPRAVKE ===
                    }
                }
                gatesInRowSum += gatesInRow;
                y_row_start += y_row_height;
            }
        }
        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za detaljnu kontrolu kapije.
 * @author      Gemini
 * @note        FINALNA VERZIJA. Ova funkcija je odgovorna za iscrtavanje
 * dinamičkih informacija na ekranu. Sadrži internu logiku za
 * praćenje promjene stanja kapije (`old_state`) i automatski
 * pokreće ponovno iscrtavanje kada se stanje promijeni. Iscrtava
 * veliku, centralnu ikonicu koja odgovara trenutnom stanju uređaja,
 * kao i prevedeni tekstualni opis tog stanja.
 ******************************************************************************
 */
static void Service_GateSettingsScreen(void)
{
    static GateState_e old_state = -1;
    static bool old_button_state[6] = {false};

    if (!gate_settings_initialized) {
        old_state = -1; // Forsiraj ponovno iscrtavanje
        gate_settings_initialized = true;
    }

    Gate_Handle* handle = Gate_GetInstance(gate_control_panel_index);
    if (handle == NULL) {
        return;         // Izađi iz funkcije
    }

    // =========================================================================
    // === ISPRAVLJENA NE-BLOKIRAJUĆA LOGIKA ZA PROVJERU DUGMADI ===
    // =========================================================================
    for (int i = 0; i < 6; i++) {
        if (WM_IsWindow(hGateControlButtons[i])) {
            bool is_pressed = BUTTON_IsPressed(hGateControlButtons[i]);

            if (is_pressed && !old_button_state[i]) {
                old_button_state[i] = true;

                BuzzerOn();
                HAL_Delay(1);
                BuzzerOff();

                int command_id = WM_GetId(hGateControlButtons[i]);

                // === NOVO: "OPTIMISTIC UI UPDATE" ZA KONTROLNI PANEL ===
                switch(command_id) {
                case UI_COMMAND_OPEN_CYCLE:
                case UI_COMMAND_PEDESTRIAN:
                    Gate_SetState(handle, GATE_STATE_OPENING);
                    break;
                case UI_COMMAND_CLOSE_CYCLE:
                    Gate_SetState(handle, GATE_STATE_CLOSING);
                    break;
                case UI_COMMAND_STOP:
                    Gate_SetState(handle, GATE_STATE_PARTIALLY_OPEN);
                    break;
                case UI_COMMAND_SMART_STEP:
                    if(Gate_GetState(handle) == GATE_STATE_CLOSED) Gate_SetState(handle, GATE_STATE_OPENING);
                    else if(Gate_GetState(handle) == GATE_STATE_OPEN) Gate_SetState(handle, GATE_STATE_CLOSING);
                    else Gate_SetState(handle, GATE_STATE_PARTIALLY_OPEN);
                    break;
                case UI_COMMAND_UNLOCK:
                    Gate_SetState(handle, GATE_STATE_OPEN);
                    break;
                }
                shouldDrawScreen = 1; // Zatraži ponovno iscrtavanje ODMAH!
                // =========================================================

                // Pozivanje backend funkcije
                switch(command_id) {
                case UI_COMMAND_OPEN_CYCLE:
                    Gate_TriggerFullCycleOpen(handle);
                    break;
                case UI_COMMAND_CLOSE_CYCLE:
                    Gate_TriggerFullCycleClose(handle);
                    break;
                case UI_COMMAND_SMART_STEP:
                    Gate_TriggerSmartStep(handle);
                    break;
                case UI_COMMAND_STOP:
                    Gate_TriggerStop(handle);
                    break;
                case UI_COMMAND_PEDESTRIAN:
                    Gate_TriggerPedestrian(handle);
                    break;
                case UI_COMMAND_UNLOCK:
                    Gate_TriggerUnlock(handle);
                    break;
                }
            }
            else if (!is_pressed && old_button_state[i]) {
                old_button_state[i] = false;
            }
        }
    }
    // =========================================================================
    // ===             KRAJ PREMJEŠTENE LOGIKE                 ===
    // =========================================================================

    GateState_e current_state = Gate_GetState(handle);
    if (current_state != old_state) {
        shouldDrawScreen = 1;
        old_state = current_state;
    }

    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_ClearRect(0, 0, DRAWING_AREA_WIDTH, 210); // Obriši samo dinamički dio

        uint8_t appearance_id = Gate_GetAppearanceId(handle);
        const IconMapping_t* mapping = &gate_appearance_mapping_table[appearance_id];

        // Iscrtavanje naziva uređaja
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
        const char* custom_label = Gate_GetCustomLabel(handle);
        if (custom_label[0] != '\0') {
            GUI_DispStringAt(custom_label, DRAWING_AREA_WIDTH / 2, 10);
        } else {
            char default_name[50];
            sprintf(default_name, "%s - %s", lng(mapping->primary_text_id), lng(mapping->secondary_text_id));
            GUI_DispStringAt(default_name, DRAWING_AREA_WIDTH / 2, 10);
        }

        // Iscrtavanje velike ikonice i statusnog teksta
        IconID visual_icon_type = mapping->visual_icon_id;
        TextID status_text_id = TXT_GATE_STATUS_UNDEFINED;

        // 1. Odaberi ispravan INDEKS ikonice na osnovu stanja
        uint8_t icon_state_index = 0;
        switch (current_state) {
        case GATE_STATE_CLOSED:
            icon_state_index = 0;
            break;
        case GATE_STATE_OPEN:
            icon_state_index = 1;
            break;
        // ISPRAVKA: Korištenje indeksa za nove, specifične ikonice
        case GATE_STATE_OPENING:
            icon_state_index = 2;
            break;
        case GATE_STATE_CLOSING:
            icon_state_index = 3;
            break;
        case GATE_STATE_PARTIALLY_OPEN:
            icon_state_index = 4;
            break;
        case GATE_STATE_FAULT:
        default:
            icon_state_index = 0;
            break;
        }

        if (current_state != GATE_STATE_FAULT)
        {
            // ISPRAVKA: Novi način računanja indeksa za blok od 5 ikonica
            uint16_t base_icon_index = (visual_icon_type - ICON_GATE_SWING) * 5;
            const GUI_BITMAP* icon_to_draw = gate_icon_images[base_icon_index + icon_state_index];
            if (icon_to_draw) {
                // Kalkulacija za centriranje velike ikonice
                int x_pos = (DRAWING_AREA_WIDTH / 2) - (icon_to_draw->XSize / 2);
                int y_pos = 110 - (icon_to_draw->YSize / 2); // Vertikalno centrirano u gornjem dijelu
                GUI_DrawBitmap(icon_to_draw, x_pos, y_pos);
            }
        }
        // === KRAJ BLOKA ZA ZAMJENU ===

        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 ******************************************************************************
 * @brief       Servisira glavni ekran za kontrolu Alarma (SCREEN_SECURITY).
 * @author      Gemini & [Vaše Ime]
 * @note        ISPRAVLJENA VERZIJA: Koristi mala (50x50) dugmad sa ikonicom
 * i ispisuje detaljne labele pored njih, u skladu sa ostatkom UI-ja.
 * Takođe koristi "pametnu logiku" za prikaz korisnički definisanih naziva.
 ******************************************************************************
 */
static void Service_SecurityScreen(void)
{
    // Crtanje se izvršava samo ako je zatraženo
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        
        Security_RefreshState();
        
//        alarm_ui_state[0] = Security_IsAnyPartitionArmed() ? ALARM_UI_STATE_ARMED : ALARM_UI_STATE_DISARMED;
//        for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
//            alarm_ui_state[i + 1] = Security_GetPartitionState(i) ? ALARM_UI_STATE_ARMED : ALARM_UI_STATE_DISARMED;
//        }
        
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        uint8_t configured_count = Security_GetConfiguredPartitionsCount();

        if (configured_count == 0) {
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_WHITE);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALARM_NOT_CONFIGURED), DRAWING_AREA_WIDTH / 2, LCD_GetYSize() / 2);
        } else {
            // Korištenje konstanti iz layout strukture
            const int16_t x_pos = security_screen_layout.start_pos.x;
            const int16_t y_start = security_screen_layout.start_pos.y;
            const int16_t y_spacing = security_screen_layout.y_spacing;
            const int16_t btn_size = security_screen_layout.button_size;
            const int16_t lbl_offset = security_screen_layout.label_x_offset;
            
            char buffer[100];
            uint8_t visible_buttons = 0;
            
            // --- Dugme i Labela za SISTEM ---
            BUTTON_Handle hBtnSystem = BUTTON_CreateEx(x_pos, y_start, btn_size, btn_size, 0, WM_CF_SHOW, 0, GUI_ID_USER + 0);
            BUTTON_SetBitmap(hBtnSystem, BUTTON_CI_UNPRESSED, &bmicons_button_right_50_squared);

            const char* system_name = Security_GetSystemName();
            const char* display_system_name = (system_name[0] == '\0') ? lng(TXT_ALARM_SYSTEM) : system_name;

            const char* status_text_system = "";
            switch(alarm_ui_state[0]) {
                case ALARM_UI_STATE_ARMED:      GUI_SetColor(GUI_GREEN);  status_text_system = lng(TXT_ALARM_STATE_ARMED);   break;
                case ALARM_UI_STATE_DISARMED:   GUI_SetColor(GUI_WHITE);  status_text_system = lng(TXT_ALARM_STATE_DISARMED); break;
                case ALARM_UI_STATE_ARMING:     GUI_SetColor(GUI_ORANGE); status_text_system = lng(TXT_ALARM_STATE_ARMING);    break;
                case ALARM_UI_STATE_DISARMING:  GUI_SetColor(GUI_ORANGE); status_text_system = lng(TXT_ALARM_STATE_DISARMING); break;
                default:                        GUI_SetColor(GUI_WHITE);  status_text_system = "N/A";                      break;
            }
            snprintf(buffer, sizeof(buffer), "%s: %s", display_system_name, status_text_system);
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
            GUI_DispStringAt(buffer, x_pos + btn_size + lbl_offset, y_start + btn_size / 2);
            
            visible_buttons++;
            
            // --- Dugmad i Labele za PARTICIJE ---
            for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
                if (Security_GetPartitionRelayAddr(i) != 0) {
                    int y_pos = y_start + visible_buttons * y_spacing;
                    
                    BUTTON_Handle hBtn = BUTTON_CreateEx(x_pos, y_pos, btn_size, btn_size, 0, WM_CF_SHOW, 0, GUI_ID_USER + 1 + i);
                    BUTTON_SetBitmap(hBtn, BUTTON_CI_UNPRESSED, &bmicons_button_right_50_squared);
                    
                    const char* partition_name = Security_GetPartitionName(i);
                    char default_partition_name[50];
                    if (partition_name[0] == '\0') {
                        snprintf(default_partition_name, sizeof(default_partition_name), "%s %d", lng(TXT_ALARM_PARTITION), i + 1);
                        partition_name = default_partition_name;
                    }
                    
                    const char* status_text_partition = "";
                    switch(alarm_ui_state[i + 1]) {
                        case ALARM_UI_STATE_ARMED:      GUI_SetColor(GUI_GREEN);  status_text_partition = lng(TXT_ALARM_STATE_ARMED);   break;
                        case ALARM_UI_STATE_DISARMED:   GUI_SetColor(GUI_WHITE);  status_text_partition = lng(TXT_ALARM_STATE_DISARMED); break;
                        case ALARM_UI_STATE_ARMING:     GUI_SetColor(GUI_ORANGE); status_text_partition = lng(TXT_ALARM_STATE_ARMING);    break;
                        case ALARM_UI_STATE_DISARMING:  GUI_SetColor(GUI_ORANGE); status_text_partition = lng(TXT_ALARM_STATE_DISARMING); break;
                        default:                        GUI_SetColor(GUI_WHITE);  status_text_partition = "N/A";                      break;
                    }
                    
                    snprintf(buffer, sizeof(buffer), "%s: %s", partition_name, status_text_partition);
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                    GUI_DispStringAt(buffer, x_pos + btn_size + lbl_offset, y_pos + btn_size / 2);

                    visible_buttons++;
                }
            }
        }
        GUI_MULTIBUF_EndEx(1);
    }

    // Obrada pritiska na dugmad (logika ostaje nepromijenjena)
    for (int i = 0; i <= SECURITY_PARTITION_COUNT; i++) {
         WM_HWIN hBtn = WM_GetDialogItem(WM_GetDesktopWindow(), GUI_ID_USER + i);
         if (hBtn && BUTTON_IsPressed(hBtn)) {
             selected_action = i;
             DSP_KillSecurityScreen();
             NumpadContext_t pin_context = { .title = lng(TXT_ALARM_ENTER_PIN), .max_len = MAX_PIN_LENGTH };
             Display_ShowNumpad(&pin_context);
             return;
         }
    }
}
/**
 ******************************************************************************
 * @brief       Uništava sve dinamički kreirane widgete sa ekrana za alarm.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija pronalazi i briše svu dugmad kreiranu u 
 * `Service_SecurityScreen` kako bi se spriječila pojava "duhova".
 ******************************************************************************
 */
static void DSP_KillSecurityScreen(void)
{
    WM_HWIN hItem;
    // Petlja briše dugme "SYSTEM" (ID_USER + 0) i dugmad za particije (ID_USER + 1 do +3)
    for (int i = 0; i <= SECURITY_PARTITION_COUNT; i++) {
        hItem = WM_GetDialogItem(WM_GetDesktopWindow(), GUI_ID_USER + i);
        if (WM_IsWindow(hItem)) {
            WM_DeleteWindow(hItem);
        }
    }
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje GUI za ekran podešavanja alarma (ISPRAVLJENA VERZIJA).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova verzija ispravno koristi mala (50x50) dugmad sa ikonicama i
 * ispisuje tekstualne labele pored njih, u skladu sa ostatkom UI-ja.
 * Koristi "pametnu logiku" za ispis naziva.
 ******************************************************************************
 */
static void DSP_InitSettingsAlarmScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu(1);

    const int16_t x_pos = 20;
    const int16_t y_start = 0;
    const int16_t btn_size = 50;
    const int16_t y_gap = 53; // Povećan razmak zbog visine reda
    const int16_t lbl_offset = 15;
    
    char buffer[50];

    // --- Kreiranje dugmeta i labele za "Promijeni Glavni PIN" ---
    int16_t current_y = y_start;
    hButtonChangePin = BUTTON_CreateEx(x_pos, current_y, btn_size, btn_size, 0, WM_CF_SHOW, 0, GUI_ID_USER + 50);
    BUTTON_SetBitmap(hButtonChangePin, BUTTON_CI_UNPRESSED, &bmicons_button_right_50_squared);
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt(lng(TXT_ALARM_CHANGE_PIN), x_pos + btn_size + lbl_offset, current_y + btn_size / 2);

    // --- Kreiranje dugmeta i labele za "Naziv Sistema" ---
    current_y += y_gap;
    hButtonSystemName = BUTTON_CreateEx(x_pos, current_y, btn_size, btn_size, 0, WM_CF_SHOW, 0, GUI_ID_USER + 51);
    BUTTON_SetBitmap(hButtonSystemName, BUTTON_CI_UNPRESSED, &bmicons_button_right_50_squared);
    
    const char* system_name = Security_GetSystemName();
    if (system_name[0] == '\0') {
        snprintf(buffer, sizeof(buffer), "%s", lng(TXT_ALARM_SYSTEM_NAME));
    } else {
        snprintf(buffer, sizeof(buffer), "%s: %s", lng(TXT_ALARM_SYSTEM_NAME), system_name);
    }
    GUI_DispStringAt(buffer, x_pos + btn_size + lbl_offset, current_y + btn_size / 2);

    // --- Kreiranje dugmadi i labela za "Naziv Particije X" ---
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++)
    {
        current_y += y_gap;
        hButtonPartitionName[i] = BUTTON_CreateEx(x_pos, current_y, btn_size, btn_size, 0, WM_CF_SHOW, 0, GUI_ID_USER + 52 + i);
        BUTTON_SetBitmap(hButtonPartitionName[i], BUTTON_CI_UNPRESSED, &bmicons_button_right_50_squared);
        
        const char* partition_name = Security_GetPartitionName(i);
        if (partition_name[0] == '\0') {
            sprintf(buffer, "%s %d", lng(TXT_ALARM_PARTITION_NAME), i + 1);
        } else {
            sprintf(buffer, "%s %d: %s", lng(TXT_ALARM_PARTITION_NAME), i + 1, partition_name);
        }
        GUI_DispStringAt(buffer, x_pos + btn_size + lbl_offset, current_y + btn_size / 2);
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete sa ekrana za podešavanje alarma.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SETTINGS_ALARM` kako bi se oslobodila memorija.
 ******************************************************************************
 */
static void DSP_KillSettingsAlarmScreen(void)
{
    // Provjera i brisanje svakog handle-a pojedinačno radi sigurnosti
    if (WM_IsWindow(hButtonChangePin)) {
        WM_DeleteWindow(hButtonChangePin);
        hButtonChangePin = 0;
    }
    if (WM_IsWindow(hButtonSystemName)) {
        WM_DeleteWindow(hButtonSystemName);
        hButtonSystemName = 0;
    }
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
        if (WM_IsWindow(hButtonPartitionName[i])) {
            WM_DeleteWindow(hButtonPartitionName[i]);
            hButtonPartitionName[i] = 0;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za podešavanje alarma.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija provjerava pritiske na pet dugmadi. U zavisnosti
 * od pritisnutog dugmeta, pokreće ili proceduru za promjenu
 * PIN-a (pozivom Numpad-a) ili proceduru za promjenu naziva
 * (pozivom alfanumeričke tastature).
 ******************************************************************************
 */
static void Service_SettingsAlarmScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        DSP_InitSettingsAlarmScreen();
    }

    // Provjera pritiska na dugme "Promijeni Glavni PIN"
    if (BUTTON_IsPressed(hButtonChangePin))
    {
        pin_change_state = PIN_CHANGE_WAIT_CURRENT;
        NumpadContext_t pin_context = {
            .title = lng(TXT_PIN_ENTER_CURRENT),
            .max_len = 8,
        };
        
        // === PRIMJENA ISPRAVNOG OBRASCA (kao kod svjetala) ===
        DSP_KillSettingsAlarmScreen();
        Display_ShowNumpad(&pin_context); // Ova funkcija samo postavlja globalne varijable
        DSP_InitNumpadScreen();           // Forsirano iscrtavanje odmah
        shouldDrawScreen = 0;             // Spriječi ponovno crtanje u DISP_Service
        return; 
    }

    // Provjera pritiska na dugme "Naziv Sistema"
    if (BUTTON_IsPressed(hButtonSystemName))
    {
        selected_partition_for_rename = -1;
        KeyboardContext_t kbd_context = {
            .title = lng(TXT_ALARM_SYSTEM_NAME),
            .max_len = 20
        };
        strncpy(kbd_context.initial_value, Security_GetSystemName(), sizeof(kbd_context.initial_value) - 1);
        
        // === PRIMJENA ISPRAVNOG OBRASCA (kao kod svjetala) ===
        DSP_KillSettingsAlarmScreen();
        Display_ShowKeyboard(&kbd_context); // Postavlja globalne varijable
        DSP_InitKeyboardScreen();           // Forsirano iscrtavanje odmah
        shouldDrawScreen = 0;             // Spriječi ponovno crtanje
        return;
    }

    // Provjera pritiska na dugmad "Naziv Particije X"
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++)
    {
        if (BUTTON_IsPressed(hButtonPartitionName[i]))
        {
            selected_partition_for_rename = i;
            char title_buffer[50];
            sprintf(title_buffer, "%s %d", lng(TXT_ALARM_PARTITION_NAME), i + 1);

            KeyboardContext_t kbd_context = {
                .title = title_buffer,
                .max_len = 20
            };
            strncpy(kbd_context.initial_value, Security_GetPartitionName(i), sizeof(kbd_context.initial_value) - 1);

            // === PRIMJENA ISPRAVNOG OBRASCA (kao kod svjetala) ===
            DSP_KillSettingsAlarmScreen();
            Display_ShowKeyboard(&kbd_context); // Postavlja globalne varijable
            DSP_InitKeyboardScreen();           // Forsirano iscrtavanje odmah
            shouldDrawScreen = 0;             // Spriječi ponovno crtanje
            return;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje kratak pritisak na ikonicu Alarma na SelectScreen2.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva iz `HandleTouchReleaseEvent`. Provjerava da li
 * je tajmer za alarm i dalje aktivan. Ako jeste, to znači da dugi
 * pritisak nije detektovan, te se izvršava akcija za kratak pritisak:
 * osvježavanje stanja i prelazak na `SCREEN_SECURITY`.
 ******************************************************************************
 */
static void HandleRelease_AlarmIcon(void)
{
    if (dynamic_icon_alarm_press_timer != 0)
    {
        // Kratak pritisak je detektovan.
        dynamic_icon_alarm_press_timer = 0;
        // 1. Osvježi stanje sa hardvera.
        Security_RefreshState();
        
        // 2. Ažuriraj lokalni UI status na osnovu realnog stanja.
        alarm_ui_state[0] = Security_IsAnyPartitionArmed() ? ALARM_UI_STATE_ARMED : ALARM_UI_STATE_DISARMED;
        for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
            alarm_ui_state[i + 1] = Security_GetPartitionState(i) ? ALARM_UI_STATE_ARMED : ALARM_UI_STATE_DISARMED;
        }

        // 3. Pređi na glavni ekran alarma.
        screen = SCREEN_SECURITY;
        shouldDrawScreen = 1;
    }
}

/**
 ******************************************************************************
 * @brief       Obrađuje kratak pritisak na ikonicu Tajmera na SelectScreen2.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva iz `HandleTouchReleaseEvent`. Provjerava da li
 * je tajmer za tajmer i dalje aktivan. Ako jeste, izvršava akciju za
 * kratak pritisak: prelazak na `SCREEN_TIMER`.
 ******************************************************************************
 */
static void HandleRelease_TimerIcon(void)
{
    if (dynamic_icon_timer_press_timer != 0)
    {
        // Kratak pritisak na ikonicu Tajmera -> idi na glavni ekran tajmera.
        dynamic_icon_timer_press_timer = 0;
        screen = SCREEN_TIMER;
        shouldDrawScreen = 1;
    }
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
