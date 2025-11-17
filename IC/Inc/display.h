/**
 ******************************************************************************
 * @file    display.h
 * @author  Edin & Gemini
 * @version V2.1.0
 * @date    18.08.2025.
 * @brief   Javni API za GUI Display Modul.
 *
 * @note
 * Ovaj modul je zadužen za kompletnu logiku iscrtavanja korisnickog
 * interfejsa (GUI) i obradu unosa putem ekrana osjetljivog na dodir.
 * Koristi STemWin biblioteku za kreiranje grafickih elemenata.
 *
 * Upravlja razlicitim ekranima definisanim u `eScreen` enumu i pruža
 * podršku za internacionalizaciju (i18n) putem `lng()` funkcije.
 *
 * U skladu sa novom arhitekturom, ovaj modul više ne pristupa direktno
 * internim podacima drugih modula (npr. `lights_modbus`), vec iskljucivo
 * koristi njihove javne API funkcije.
 *
 * Definicija `Display_EepromSettings_t` strukture je javna kako bi
 * drugi moduli mogli da citaju globalne postavke displeja.
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


#ifndef __DISP_H__
#define __DISP_H__                           FW_BUILD // version

/* Includes ------------------------------------------------------------------*/
#include "main.h" // Ukljucen samo 'main.h' za osnovne tipove

/*============================================================================*/
/* JAVNE, GLOBALNE DEFINICIJE I ENUMERATORI                                   */
/*============================================================================*/

/**
 * @brief Definiše podržane komunikacione protokole za aktuatore i senzore.
 * @note  Ovo je globalna sistemska postavka.
 */
typedef enum {
    PROTOCOL_TINYFRAME,     /**< Koristi jedinstvenu, apsolutnu adresu za svaki I/O. */
    PROTOCOL_MODBUS         /**< Koristi par: Adresa Modula + Adresa Registra/Coil-a. */
} ProtocolType_e;


#define TS_LAYER    1       ///< Svrha: GUI "sloj" (layer) na kojem se obraduju dodiri. Vrijednost: 1 (sloj iznad pozadinskog).

/**
 * @brief Definiše sve podržane jezike u sistemu.
 * @note BSHC je skracenica za bosanski/srpski/hrvatski/crnogorski.
 * Redoslijed mora tacno odgovarati redoslijedu kolona u `language_strings`
 * i `_acContent` tabelama u `translations.h`.
 */
enum Languages { BSHC = 0, ENG, GER, FRA, ITA, SPA, RUS, UKR, POL, CZE, SLO, LANGUAGE_COUNT };

/**
 * @brief Definiše jedinstveni ID za svaki tekstualni string u aplikaciji.
 * @note  Redoslijed je logicki organizovan radi lakšeg održavanja i
 * mora se 1-na-1 poklapati sa redoslijedom u `language_strings` tabeli.
 */
typedef enum {
    TXT_DUMMY = 0,
    // --- Glavni meniji ---
    TXT_LIGHTS,
    TXT_THERMOSTAT,
    TXT_BLINDS,
    TXT_DEFROSTER,
    TXT_VENTILATOR,
    TXT_CLEAN,
    TXT_WIFI,
    TXT_APP,
    // --- NOVI Glavni meniji (SELECT_2, SELECT_3) ---
    TXT_GATE,                       /**< Tekst za ikonicu "Kapija" na `SCREEN_SELECT_2`. */
    TXT_TIMER,                      /**< Tekst za ikonicu "Tajmer" na `SCREEN_SELECT_2`. */
    TXT_SECURITY,                   /**< Tekst za ikonicu "Security/Alarm" na `SCREEN_SELECT_2`. */
    TXT_SCENES,                     /**< Tekst za naslov ekrana sa scenama i za ikonicu menija. */
    TXT_LANGUAGE_SOS_ALL_OFF,       /**< Tekst za multi-funkcionalnu ikonicu na `SCREEN_SELECT_2`. */
    // --- Opšti tekstovi ---
    TXT_ALL,
    TXT_SETTINGS,
    TXT_GLOBAL_SETTINGS,
    TXT_SAVE,                       /**< Tekst za "Snimi" dugme u menijima za podešavanja. */
    TXT_ENTER_NEW_NAME,             /**< Naslov za tastaturu pri izmjeni naziva. */
    TXT_CANCEL,                     /**< Tekst za "Otkaži" dugme u menijima za podešavanja. */
    TXT_DELETE,                     /**< Tekst za "Obriši" dugme u menijima za podešavanja. */
    TXT_CONFIGURE_DEVICE_MSG,       /**< Poruka koja se prikazuje ako uredaj nije uopšte konfigurisan. */
    TXT_SCENE_SAVED_MSG,            /**< Poruka potvrde nakon snimanja scene. */
    TXT_PLEASE_CONFIGURE_SCENE_MSG, /**< Poruka koja poziva na konfigurisanje prve, defaultne scene. */
    TXT_TIMER_ENABLED,              /**< Poruka "Tajmer je ukljucen" na glavnom ekranu tajmera. */
    TXT_TIMER_DISABLED,             /**< Poruka "Tajmer je iskljucen" na glavnom ekranu tajmera. */
    TXT_TIMER_EVERY_DAY,            /**< Tekst "Svaki dan" za ponavljanje tajmera. */
    TXT_TIMER_WEEKDAYS,             /**< Tekst "Radnim danima" za ponavljanje tajmera. */
    TXT_TIMER_WEEKEND,              /**< Tekst "Vikendom" za ponavljanje tajmera. */
    TXT_TIMER_ONCE,                 /**< Tekst "Jednokratno" za ponavljanje tajmera. */
    TXT_TIMER_USE_BUZZER,           /**< Tekst "Koristi buzzer" na ekranu za podešavanje tajmera. */
    TXT_TIMER_TRIGGER_SCENE,        /**< Tekst "Pokreni scenu" na ekranu za podešavanje tajmera. */
    TXT_ALARM_WAKEUP,               /**< TEKST: Velika poruka za budenje. */
    // --- Poruke i dugmad ---
    TXT_DISPLAY_CLEAN_TIME,
    TXT_FIRMWARE_UPDATE,
    TXT_UPDATE_IN_PROGRESS,
    // --- Dani u sedmici ---
    TXT_MONDAY, TXT_TUESDAY, TXT_WEDNESDAY, TXT_THURSDAY, TXT_FRIDAY, TXT_SATURDAY, TXT_SUNDAY,
    // --- Mjeseci ---
    TXT_MONTH_JAN, TXT_MONTH_FEB, TXT_MONTH_MAR, TXT_MONTH_APR, TXT_MONTH_MAY, TXT_MONTH_JUN,
    TXT_MONTH_JUL, TXT_MONTH_AUG, TXT_MONTH_SEP, TXT_MONTH_OCT, TXT_MONTH_NOV, TXT_MONTH_DEC,
    // --- Nazivi jezika ---
    TXT_LANGUAGE_NAME,
    // --- Tekstovi za ekran podesavanja vremena ---
    TXT_DATETIME_SETUP_TITLE,
    TXT_TIMER_SETTINGS_TITLE,
    TXT_DAY,
    TXT_MONTH,
    TXT_YEAR,
    TXT_HOUR,
    TXT_MINUTE,
    // --- Primarni tekstovi za ikonice (postojeci) ---
    TXT_LUSTER, TXT_SPOT, TXT_VISILICA, TXT_PLAFONJERA, TXT_ZIDNA, TXT_SLIKA,
    TXT_PODNA, TXT_STOLNA, TXT_LED_TRAKA, TXT_VENTILATOR_IKONA, TXT_FASADA, TXT_STAZA, TXT_REFLEKTOR,
    // --- NOVI Nazivi za Scene ---
    TXT_SCENE_WIZZARD,
    TXT_SCENE_MORNING,
    TXT_SCENE_SLEEP,
    TXT_SCENE_LEAVING,
    TXT_SCENE_HOMECOMING,
    TXT_SCENE_MOVIE,
    TXT_SCENE_DINNER,
    TXT_SCENE_READING,
    TXT_SCENE_RELAXING,
    TXT_SCENE_GATHERING,
    // --- Tekstovi za Gate modul ---
    // -- Primarni tipovi --
    TXT_GATE_SWING,
    TXT_GATE_SLIDING,
    TXT_GATE_GARAGE,
    TXT_GATE_RAMP,
    TXT_GATE_PEDESTRIAN_LOCK,
    TXT_GATE_SECURITY_DOOR,         /**< NOVO: Primarni naziv za sigurnosna vrata. */
    TXT_GATE_UNDERGROUND_RAMP,      /**< NOVO: Primarni naziv za podzemnu rampu. */
    // -- Opšti tekstovi menija --
    TXT_GATE_CONTROL_PROFILE,
    TXT_GATE_APPEARANCE,
    // -- Nazivi komandi (za UI) --
    TXT_GATE_CMD_OPEN,
    TXT_GATE_CMD_CLOSE,
    TXT_GATE_CMD_STOP,
    TXT_GATE_CMD_PEDESTRIAN,
    TXT_GATE_CMD_UNLOCK,
    // -- Postojece sekundarne odrednice --
    TXT_GATE_MAIN_SECONDARY,
    TXT_GATE_YARD_SECONDARY,
    TXT_GATE_ENTRANCE_SECONDARY,

    // NOVO: Tekstovi za status kapije
    TXT_GATE_STATUS_CLOSED,
    TXT_GATE_STATUS_OPENING,
    TXT_GATE_STATUS_OPEN,
    TXT_GATE_STATUS_CLOSING,
    TXT_GATE_STATUS_PARTIAL,
    TXT_GATE_STATUS_FAULT,
    TXT_GATE_STATUS_UNDEFINED,
    
    // === NOVE SEKUNDARNE ODREDNICE ZA KAPIJE ===
    // -- Numericke --
    TXT_GATE_SECONDARY_1,
    TXT_GATE_SECONDARY_2,
    TXT_GATE_SECONDARY_3,
    TXT_GATE_SECONDARY_4,
    TXT_GATE_SECONDARY_5,
    TXT_GATE_SECONDARY_6,
    TXT_GATE_SECONDARY_7,
    TXT_GATE_SECONDARY_8,
    // -- Pozicijske --
    TXT_GATE_SECONDARY_DONJA,
    TXT_GATE_SECONDARY_SREDNJA,
    TXT_GATE_SECONDARY_GORNJA,
    TXT_GATE_SECONDARY_LIJEVA,
    TXT_GATE_SECONDARY_DESNA,
    TXT_GATE_SECONDARY_PREDNJA,
    TXT_GATE_SECONDARY_ZADNJA,
    TXT_GATE_SECONDARY_ISTOK,
    TXT_GATE_SECONDARY_ZAPAD,
    TXT_GATE_SECONDARY_SJEVER,
    TXT_GATE_SECONDARY_JUG,
    // -- Funkcionalne --
    TXT_GATE_SECONDARY_ULAZ,
    TXT_GATE_SECONDARY_IZLAZ,
    TXT_GATE_SECONDARY_PROLAZ,
    TXT_GATE_SECONDARY_GLAVNI,
    TXT_GATE_SECONDARY_SPOREDNI,
    TXT_GATE_SECONDARY_SERVISNI,
    TXT_GATE_SECONDARY_PRIVATNI,
    TXT_GATE_SECONDARY_DOSTAVA,
    // -- Lokacijske (specificno za vile) --
    TXT_GATE_SECONDARY_KUCA_ZA_GOSTE,
    TXT_GATE_SECONDARY_BAZEN,
    TXT_GATE_SECONDARY_TENISKI_TEREN,
    TXT_GATE_SECONDARY_VINARIJA,
    TXT_GATE_SECONDARY_KONJUSNICA,
    TXT_GATE_SECONDARY_VRT,
    TXT_GATE_SECONDARY_PARK,
    TXT_GATE_SECONDARY_JEZERO,
    // -- Stil/Materijal --
    TXT_GATE_SECONDARY_KOVANA,
    TXT_GATE_SECONDARY_DRVENA,
    TXT_GATE_SECONDARY_MODERNA,
    TXT_GATE_SECONDARY_KAMENA,
    
    // --- Sekundarni tekstovi za ikonice (postojeci) ---
    TXT_GLAVNI_SECONDARY, TXT_AMBIJENT_SECONDARY, TXT_TRPEZARIJA_SECONDARY, TXT_DNEVNA_SOBA_SECONDARY,
    TXT_LIJEVI_SECONDARY, TXT_DESNI_SECONDARY, TXT_CENTRALNI_SECONDARY, TXT_PREDNJI_SECONDARY,
    TXT_ZADNJI_SECONDARY, TXT_HODNIK_SECONDARY, TXT_KUHINJA_SECONDARY, TXT_IZNAD_SANKA_SECONDARY,
    TXT_IZNAD_STola_SECONDARY, TXT_PORED_KREVETA_1_SECONDARY, TXT_PORED_KREVETA_2_SECONDARY,
    TXT_GLAVNA_SECONDARY, TXT_SOBA_1_SECONDARY, TXT_SOBA_2_SECONDARY, TXT_KUPATILO_SECONDARY,
    TXT_LIJEVA_SECONDARY, TXT_DESNA_SECONDARY, TXT_GORE_SECONDARY, TXT_DOLE_SECONDARY,
    TXT_CITANJE_SECONDARY, TXT_OGLEDALO_SECONDARY, TXT_UGAO_SECONDARY, TXT_PORED_FOTELJE_SECONDARY,
    TXT_RADNI_STO_SECONDARY, TXT_NOCNA_1_SECONDARY, TXT_NOCNA_2_SECONDARY,
    TXT_ISPOD_ELEMENTA_SECONDARY, TXT_IZNAD_ELEMENTA_SECONDARY, TXT_ORMAR_SECONDARY,
    TXT_STEPENICE_SECONDARY, TXT_TV_SECONDARY, TXT_ULAZ_SECONDARY, TXT_TERASA_SECONDARY,
    TXT_BALKON_SECONDARY, TXT_ZADNJA_SECONDARY, TXT_PRILAZ_SECONDARY, TXT_DVORISTE_SECONDARY,
    TXT_DRVO_SECONDARY,
    
    // === NOVI TEKSTOVI ZA ALARM MODUL ===
    TXT_ALARM_SETTINGS_TITLE,
    TXT_ALARM_SYSTEM_ARM_DISARM,
    TXT_ALARM_PARTITION_1,
    TXT_ALARM_PARTITION_2,
    TXT_ALARM_PARTITION_3,
    TXT_ALARM_RELAY_ADDRESS,
    TXT_ALARM_FEEDBACK_ADDRESS,
    TXT_ALARM_SYSTEM_STATUS_FB,
    TXT_ALARM_PULSE_LENGTH,
    TXT_ALARM_SILENT_ALARM,
    TXT_ALARM_STATE_ARMED,
    TXT_ALARM_STATE_DISARMED,
    TXT_ALARM_STATE_ARMING,         /**< Tekst: Naoružavanje... */
    TXT_ALARM_STATE_DISARMING,      /**< Tekst: Razoružavanje... */
    TXT_ALARM_SYSTEM,               /**< Tekst: SISTEM */
    TXT_ALARM_PARTITION,
    TXT_ALARM_CMD_ARM,
    TXT_ALARM_CMD_DISARM,
    TXT_ALARM_ENTER_PIN,
    
     // === NOVI TEKSTOVI ZA PODEŠAVANJE ALARMA I PROMJENU PIN-a ===
    TXT_PIN_ENTER_CURRENT,      /**< Naslov: "TRENUTNI PIN" */
    TXT_PIN_ENTER_NEW,          /**< Naslov: "UNESITE NOVI PIN" */
    TXT_PIN_CONFIRM_NEW,        /**< Naslov: "POTVRDITE NOVI PIN" */
    TXT_PIN_WRONG,              /**< Poruka: "POGREŠAN PIN" */
    TXT_PINS_DONT_MATCH,        /**< Poruka: "PIN-ovi se ne podudaraju" */
    TXT_PIN_CHANGE_SUCCESS,     /**< Poruka: "PIN uspješno promijenjen" */
    TXT_ALARM_CHANGE_PIN,       /**< Labela dugmeta: "Promijeni Glavni PIN" */
    TXT_ALARM_SYSTEM_NAME,      /**< Labela dugmeta: "Naziv Sistema" */
    TXT_ALARM_PARTITION_NAME,   /**< Labela dugmeta (koristice se kao prefiks, npr. "Naziv Particije 1") */
    TXT_ALARM_NOT_CONFIGURED,   /**< Poruka: "Alarmni sistem nije konfigurisan" */
    TXT_OK,                     // << NOVI ID
    TXT_DEL,                    // << NOVI ID
    TXT_OFF_SHORT,              // << NOVI ID
    TXT_ERROR,                  // << NOVI ID
    TEXT_COUNT // Uvijek na kraju!
} TextID;


/**
 * @brief Definiše jedinstveni ID za svaki VIZUELNI TIP ikonice.
 * @note  Nazivi su izvedeni iz imena bitmap varijabli u Resource.h.
 * Redoslijed u ovom enumu MORA odgovarati redoslijedu u `light_modbus_images` nizu u `display.c`.
 */
typedef enum {
    ICON_BULB = 0,                  // Vrijednost = 0
    ICON_VENTILATOR_ICON,           // Vrijednost = 1
    ICON_CEILING_LED_FIXTURE,       // Vrijednost = 2
    ICON_CHANDELIER,                // Vrijednost = 3
    ICON_HANGING,                   // Vrijednost = 4
    ICON_LED_STRIP,                 // Vrijednost = 5
    ICON_SPOT_CONSOLE,              // Vrijednost = 6
    ICON_SPOT_SINGLE,               // Vrijednost = 7
    ICON_STAIRS,                    // Vrijednost = 8
    ICON_WALL,                      // Vrijednost = 9
    // Ovdje se mogu dodati i ostale ikonice iz todo.txt kada za njih budu kreirane bitmape
    // Npr. ICON_PICTURE_LIGHT, ICON_FLOOR_LAMP, itd.
    
    // --- NOVO: Ikonice za Scene ---
    ICON_SCENE_WIZZARD,
    ICON_SCENE_MORNING,
    ICON_SCENE_SLEEP,
    ICON_SCENE_LEAVING,
    ICON_SCENE_HOMECOMING,
    ICON_SCENE_MOVIE,
    ICON_SCENE_DINNER,
    ICON_SCENE_READING,
    ICON_SCENE_RELAXING,
    ICON_SCENE_GATHERING,
    
    // --- NOVO: Ikonice za Kapije ---
    ICON_GATE_SWING,
    ICON_GATE_SLIDING,
    ICON_GATE_GARAGE,
    ICON_GATE_RAMP,
    ICON_GATE_PEDESTRIAN_LOCK,
    ICON_GATE_SECURITY_DOOR,       /**< NOVO: Vizuelni ID za sigurnosna vrata. */
    ICON_GATE_UNDERGROUND_RAMP,    /**< NOVO: Vizuelni ID za podzemnu rampu. */
    // Ovdje se mogu dodavati nove ikone za buduce module...
    
    ICON_COUNT // Ukupan broj definisanih vizuelnih tipova ikonica
} IconID;


/**
 * @brief Struktura koja mapira jedan unos iz SPINBOX-a na vizuelni prikaz.
 * @note  Ovo je kljucna struktura za novi, fleksibilni sistem prikaza ikonica.
 * Niz ovih struktura ce biti definisan u `translations.h`.
 */
typedef struct {
    IconID visual_icon_id;      /**< Koju slicicu (bitmapu) treba iscrtati. */
    TextID primary_text_id;     /**< ID za gornji (primarni) tekstualni opis. */
    TextID secondary_text_id;   /**< ID za donji (sekundarni) tekstualni opis. */
} IconMapping_t;


#pragma pack(push, 1)
/**
 * @brief  Sadrži sve EEPROM postavke vezane za displej i GUI.
 * @note   Ova struktura se cuva i cita kao jedan blok iz EEPROM-a i zašticena
 * je magicnim brojem i CRC-om radi integriteta podataka.
 */
typedef struct
{
    uint16_t magic_number;              /**< "Potpis" za validaciju podataka. */
    uint8_t  low_bcklght;               /**< Vrijednost niske svjetline ekrana (1-90). */
    uint8_t  high_bcklght;              /**< Vrijednost visoke svjetline ekrana (1-90). */
    uint8_t  scrnsvr_tout;              /**< Timeout za screensaver u sekundama. */
    uint8_t  scrnsvr_ena_hour;          /**< Sat (0-23) kada se screensaver automatski aktivira. */
    uint8_t  scrnsvr_dis_hour;          /**< Sat (0-23) kada se screensaver automatski deaktivira. */
    uint8_t  scrnsvr_clk_clr;           /**< Indeks boje sata na screensaver-u. */
    bool     scrnsvr_on_off;            /**< Fleg da li je screensaver sat ukljucen. */
    bool     leave_scrnsvr_on_release;  /**< Fleg, ako je true, screensaver se gasi samo nakon otpuštanja dodirom. */
    uint8_t  language;                  /**< Odabrani jezik (BSHC = 0, ENG = 1, ...). */
    ProtocolType_e rs485_protocol;      /**< Globalno odabrani protokol za cijeli sistem (TinyFrame ili Modbus). */
    uint8_t  rs485_baud_rate_index;     /**< Indeks odabrane brzine u nizu `bps[]` iz `common.c`. */
    uint8_t  selected_control_mode;     /**< Odabrani mod za dinamicku ikonu na SelectScreen1. */
    uint8_t  selected_control_mode_2;   /**< NOVO: Odabrani mod za dinamicku ikonu na SelectScreen2. */
    bool     light_night_timer_enabled; /**< Fleg za nocni tajmer svjetala. */
    bool     scenes_enabled;            /**< Fleg (true/false) koji omogucava/onemogucava napredne funkcije (scene). */
    bool     security_module_enabled;   /**< Fleg (true/false) koji omogucava/onemogucava security modul. */
    uint16_t scene_homecoming_triggers[SCENE_MAX_TRIGGERS]; /**< Niz koji cuva adrese (npr. Modbus) za 8 logickih okidaca "Povratak" scene. */
    uint16_t crc;                       /**< CRC za provjeru integriteta. */
} Display_EepromSettings_t;
#pragma pack(pop)

/// =======================================================================
// === NOVO: Automatsko generisanje `enum`-a za ID-jeve ===
//
// ULOGA: Ovaj blok koda koristi X-Macro tehniku da automatski kreira
// jedinstveni `enum` tip koji sadrži sve ID-jeve iz `settings_widgets.def`.
// Ovo je cistiji i sigurniji nacin od gomile `#define`-ova.
//
typedef enum {
    // 1. Privremeno redefinišemo makro WIDGET da generiše `ID = VRIJEDNOST,`
    #define WIDGET(id, val, comment) id = val,
    // 2. Ukljucujemo našu glavnu listu
    #include "settings_widgets.def"
    // 3. Odmah poništavamo našu privremenu definiciju
    #undef WIDGET
} SettingsWidgetID_e;


// =======================================================================
/* Exported types ------------------------------------------------------------*/

typedef enum{
    SCREEN_RESET_MENU_SWITCHES = 0,
    SCREEN_MAIN = 1,
    SCREEN_SELECT_1,
    SCREEN_SELECT_2,                /**< NOVI EKRAN: Sadrži ikonice za Kapiju, Tajmer, Alarm i Multi-funkciju. */
    SCREEN_SELECT_3,                /**< NOVI EKRAN: Rezervisan za buduci razvoj (npr. Scene, Potrošnja). */
    SCREEN_SELECT_LAST,             /**< PREIMENOVAN (bivši SCREEN_SELECT_2): Sadrži pomocne funkcije (Cišcenje, WiFi, App, Podešavanja). */
    SCREEN_THERMOSTAT,
    SCREEN_LIGHTS,
    SCREEN_CURTAINS,
    SCREEN_GATE,                    /**< NOVI EKRAN (placeholder): Prikaz statusa i osnovne kontrole za kapije. */
    SCREEN_GATE_SETTINGS,           /**< NOVI EKRAN: Detaljni kontrolni panel za jedan uredaj, poziva se dugim pritiskom. */
    SCREEN_TIMER,                   /**< NOVI EKRAN (placeholder): Prikaz i podešavanje tajmera. */
    SCREEN_SECURITY,                /**< NOVI EKRAN (placeholder): Prikaz i kontrola alarmnog sistema. */
    SCREEN_SCENE,                   /**< NOVI EKRAN: Prikaz i aktivacija korisnicki definisanih scena. */
    SCREEN_SCENE_EDIT,              /**< NOVI EKRAN ("Carobnjak"): Vodi korisnika kroz kreiranje/editovanje scene. */
    SCREEN_SCENE_APPEARANCE,
    SCREEN_SCENE_CONFIRM_DIALOG,    /**< NOVI EKRAN: Prikazuje genericki dijalog za potvrdu (Da/Ne) za akcije poput brisanja ili snimanja. */
    SCREEN_SCENE_WIZ_DEVICES,       /**< NOVI EKRAN: Korak u carobnjaku za odabir grupa uredaja (svjetla, roletne...) koje scena obuhvata. */
    SCREEN_SCENE_WIZ_LEAVING,       /**< NOVI EKRAN: Korak u carobnjaku sa specificnim podešavanjima za "Odlazak" scenu (odgoda, simulacija...). */
    SCREEN_SCENE_WIZ_HOMECOMING,    /**< NOVI EKRAN: Korak u carobnjaku za konfiguraciju automatskih okidaca za "Povratak" scenu. */
    SCREEN_SCENE_WIZ_SLEEP,         /**< NOVI EKRAN: Korak u carobnjaku sa specificnim podešavanjima za "Spavanje" scenu (alarm, budenje...). */
    SCREEN_SCENE_WIZ_FINALIZE,      /**< NOVI EKRAN: Finalni ekran za potvrdu snimanja scene. */
    SCREEN_GATE_CONTROL_PANEL,      /**< NOVI EKRAN (pop-up): Prikazuje napredne, "custom" kontrole za kapiju. */
    SCREEN_LIGHT_SETTINGS,
    SCREEN_QR_CODE,
    SCREEN_CLEAN,
    SCREEN_KEYBOARD_ALPHA,          /**< NOVI EKRAN: Univerzalna alfanumericka tastatura za unos teksta. */
    SCREEN_NUMPAD,                  /**< NOVI EKRAN: Univerzalni numericki keypad za unos brojeva. */
    SCREEN_CONFIGURE_DEVICE,        /**< NOVI EKRAN: Prikazuje poruku ako uredaj nije uopšte konfigurisan. */
    SCREEN_RETURN_TO_FIRST,
    SCREEN_SETTINGS_ALARM,          /**< NOVI EKRAN: Meni za podešavanje naziva alarma i promjenu PIN-a. */
    SCREEN_SETTINGS_1,
    SCREEN_SETTINGS_2,
    SCREEN_SETTINGS_3,
    SCREEN_SETTINGS_4,
    SCREEN_SETTINGS_5,
    SCREEN_SETTINGS_6,
    SCREEN_SETTINGS_7,
    SCREEN_SETTINGS_8,              /**< NOVI EKRAN: Meni za detaljno podešavanje do 6 kapija/garažnih vrata. */
    SCREEN_SETTINGS_9,
    SCREEN_SETTINGS_TIMER,          /**< NOVI EKRAN: Meni za detaljno podešavanje parametara Pametnog Alarma. */
    SCREEN_SETTINGS_DATETIME,       /**< NOVI EKRAN: Prikazuje se samo ako RTC vrijeme nije validno, za podešavanje datuma i vremena. */
    SCREEN_SETTINGS_HELP,           /**< NOVI EKRAN: Prikazuje tekstualnu pomoc i uputstva za korištenje. */
    SCREEN_ALARM_ACTIVE,
    SCREEN_LANGUAGE_SELECT,         // << NOVO
    SCREEN_THEME_SELECT,            // << NOVO
    SCREEN_OUTDOOR_TIMER,           // << NOVO
    SCREEN_OUTDOOR_SETTINGS         // << NOVO (za adrese vanjske rasvjete)
}eScreen;

typedef enum{
	RELEASED    = 0,
	PRESSED     = 1,
    BUTTON_SHIT = 2
}BUTTON_StateTypeDef;

// Enumeracija za opcije u DROPDOWN meniju
typedef enum {
    MODE_OFF = 0,
    MODE_DEFROSTER,
    MODE_VENTILATOR,
    MODE_LANGUAGE,
    MODE_THEME,
    MODE_SOS,
    MODE_ALL_OFF,
    MODE_OUTDOOR,
    MODE_COUNT
} ControlMode;

/* Exported variables  -------------------------------------------------------*/
extern uint32_t dispfl;
extern uint8_t curtain_selected;
extern uint8_t screen, shouldDrawScreen;
extern Display_EepromSettings_t g_display_settings;

/* Expoted Macro   --------------------------------------------------------- */
#define DISPUpdateSet()                     (dispfl |=  (1U<<0))
#define DISPUpdateReset()                   (dispfl &=(~(1U<<0)))
#define IsDISPUpdateActiv()                 (dispfl &   (1U<<0))
#define DISPBldrUpdSet()                    (dispfl |=  (1U<<1))
#define DISPBldrUpdReset()		            (dispfl &=(~(1U<<1)))
#define IsDISPBldrUpdSetActiv()		        (dispfl &   (1U<<1))
#define DISPBldrUpdFailSet()			    (dispfl |=  (1U<<2))
#define DISPBldrUpdFailReset()	            (dispfl &=(~(1U<<2)))
#define IsDISPBldrUpdFailActiv()	        (dispfl &   (1U<<2))
#define DISPUpdProgMsgSet()                 (dispfl |=  (1U<<3))
#define DISPUpdProgMsgDel()                 (dispfl &=(~(1U<<3)))
#define IsDISPUpdProgMsgActiv()             (dispfl &   (1U<<3))
#define DISPFwrUpd()                        (dispfl |=  (1U<<4))
#define DISPFwrUpdDelete()                  (dispfl &=(~(1U<<4)))
#define IsDISPFwrUpdActiv()                 (dispfl &   (1U<<4))
#define DISPFwrUpdFail()                    (dispfl |=  (1U<<5))
#define DISPFwrUpdFailDelete()              (dispfl &=(~(1U<<5)))
#define IsDISPFwrUpdFailActiv()             (dispfl &   (1U<<5))
#define DISPFwUpdSet()                      (dispfl |=  (1U<<6))
#define DISPFwUpdReset()                    (dispfl &=(~(1U<<6)))
#define IsDISPFwUpdActiv()                  (dispfl &   (1U<<6))
#define DISPFwUpdFailSet()                  (dispfl |=  (1U<<7))
#define DISPFwUpdFailReset()                (dispfl &=(~(1U<<7)))
#define IsDISPFwUpdFailActiv()              (dispfl &   (1U<<7))
#define PWMErrorSet()                       (dispfl |=  (1U<<8)) 
#define PWMErrorReset()                     (dispfl &=(~(1U<<8)))
#define IsPWMErrorActiv()                   (dispfl &   (1U<<8))
#define DISPKeypadSet()                     (dispfl |=  (1U<<9))
#define DISPKeypadReset()                   (dispfl &=(~(1U<<9)))
#define IsDISPKeypadActiv()                 (dispfl &   (1U<<9))
#define DISPUnlockSet()                     (dispfl |=  (1U<<10))
#define DISPUnlockReset()                   (dispfl &=(~(1U<<10)))
#define IsDISPUnlockActiv()                 (dispfl &   (1U<<10))
#define DISPLanguageSet()                   (dispfl |=  (1U<<11))
#define DISPLanguageReset()                 (dispfl &=(~(1U<<11)))
#define IsDISPLanguageActiv()               (dispfl &   (1U<<11))
#define DISPSettingsInitSet()               (dispfl |=  (1U<<12))	
#define DISPSettingsInitReset()             (dispfl &=(~(1U<<12)))
#define IsDISPSetInitActiv()                (dispfl &   (1U<<12))
#define DISPRefreshSet()					(dispfl |=  (1U<<13))
#define DISPRefreshReset()                  (dispfl &=(~(1U<<13)))
#define IsDISPRefreshActiv()			    (dispfl &   (1U<<13))
#define ScreenInitSet()                     (dispfl |=  (1U<<14))
#define ScreenInitReset()                   (dispfl &=(~(1U<<14)))
#define IsScreenInitActiv()                 (dispfl &   (1U<<14))
#define RtcTimeValidSet()                   (dispfl |=  (1U<<15))
#define RtcTimeValidReset()                 (dispfl &=(~(1U<<15)))
#define IsRtcTimeValid()                    (dispfl &   (1U<<15))
#define SPUpdateSet()                       (dispfl |=  (1U<<16)) 
#define SPUpdateReset()                     (dispfl &=(~(1U<<16)))
#define IsSPUpdateActiv()                   (dispfl &   (1U<<16))
#define ScrnsvrSet()                        (dispfl |=  (1U<<17)) 
#define ScrnsvrReset()                      (dispfl &=(~(1U<<17)))
#define IsScrnsvrActiv()                    (dispfl &   (1U<<17))
#define ScrnsvrClkSet()                     (dispfl |=  (1U<<18))
#define ScrnsvrClkReset()                   (dispfl &=(~(1U<<18)))
#define IsScrnsvrClkActiv()                 (dispfl &   (1U<<18))
#define ScrnsvrSemiClkSet()                 (dispfl |=  (1U<<19))
#define ScrnsvrSemiClkReset()               (dispfl &=(~(1U<<19)))
#define IsScrnsvrSemiClkActiv()             (dispfl &   (1U<<19))
#define MVUpdateSet()                       (dispfl |=  (1U<<20)) 
#define MVUpdateReset()                     (dispfl &=(~(1U<<20)))
#define IsMVUpdateActiv()                   (dispfl &   (1U<<20))
#define ScrnsvrEnable()                     (dispfl |=  (1U<<21)) 
#define ScrnsvrDisable()                    (dispfl &=(~(1U<<21)))
#define IsScrnsvrEnabled()                  (dispfl &   (1U<<21))
#define ScrnsvrInitSet()                    (dispfl |=  (1U<<22)) 
#define ScrnsvrInitReset()                  (dispfl &=(~(1U<<22)))
#define IsScrnsvrInitActiv()                (dispfl &   (1U<<22))
#define BTNUpdSet()                         (dispfl |=  (1U<<23))
#define BTNUpdReset()                       (dispfl &=(~(1U<<23)))
#define IsBTNUpdActiv()                     (dispfl &   (1U<<23))
#define DISPCleaningSet()                   (dispfl |=  (1U<<24))
#define DISPCleaningReset()                 (dispfl &=(~(1U<<24)))
#define IsDISPCleaningActiv()               (dispfl &   (1U<<24))

/* Exported functions  -------------------------------------------------------*/
void DISP_Init(void);
void DISP_Service(void);
void DISPSetPoint(void);
const char* lng(uint8_t t);
void DISPResetScrnsvr(void);
void DISP_UpdateLog(const char *pbuf);
void DISP_SignalDynamicIconUpdate(void);
uint8_t DISP_GetThermostatMenuState(void);
uint8_t* QR_Code_Get(const uint8_t qrCodeID);
bool QR_Code_willDataFit(const uint8_t *data);
void DISP_SetThermostatMenuState(uint8_t state);
bool QR_Code_isDataLengthShortEnough(uint8_t dataLength);
void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data);

#endif
