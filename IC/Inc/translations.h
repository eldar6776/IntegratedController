/**
 ******************************************************************************
 * @file    translations.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Centralizovana datoteka za sve prevodive stringove i mapiranja u aplikaciji.
 *
 * @note    Ova datoteka sadrži tabelu `language_strings` koja je ključna za
 * internacionalizaciju (i18n), kao i `icon_mapping_table` i
 * `scene_appearance_table` koje fleksibilno povezuju ID izbora
 * korisnika sa vizuelnim prikazom ikonica i njihovim opisima.
 ******************************************************************************
 */

#ifndef __TRANSLATIONS_H__
#define __TRANSLATIONS_H__

#include "display.h" 
#include "scene.h"

/**
 * @brief Glavna tabela za mapiranje ikonica svjetala.
 * @note  Ova tabela je "Jedinstveni Izvor Istine" za prikaz ikonica u meniju za podešavanje.
 * Vrijednost iz SPINBOX-a (`iconID`) se koristi kao direktan indeks u ovom nizu.
 * Svaki element niza definiše koju sličicu (bitmapu) treba prikazati i koje
 * tekstualne opise koristiti, omogućavajući višestruke opise za istu sličicu.
 */
static const IconMapping_t icon_mapping_table[] = {
    // --- Plafonska rasvjeta ---
    /* LUSTER */
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_GLAVNI_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_AMBIJENT_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_TRPEZARIJA_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_DNEVNA_SOBA_SECONDARY },
    /* SPOT (Ugradbeno svjetlo) */
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_LIJEVI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_DESNI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_CENTRALNI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_PREDNJI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_ZADNJI_SECONDARY },
    { ICON_SPOT_CONSOLE,        TXT_SPOT,       TXT_HODNIK_SECONDARY },
    { ICON_SPOT_CONSOLE,        TXT_SPOT,       TXT_KUHINJA_SECONDARY },
    /* VISILICA */
    { ICON_HANGING,             TXT_VISILICA,   TXT_IZNAD_SANKA_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_IZNAD_STola_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_PORED_KREVETA_1_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_PORED_KREVETA_2_SECONDARY },
    /* PLAFONJERA */
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_GLAVNA_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_SOBA_1_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_SOBA_2_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_KUPATILO_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_HODNIK_SECONDARY },
    
    // --- Zidna rasvjeta ---
    /* ZIDNA LAMPA */
    { ICON_WALL,                TXT_ZIDNA,      TXT_LIJEVA_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_DESNA_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_GORE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_DOLE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_CITANJE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_AMBIJENT_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_OGLEDALO_SECONDARY },

    // --- Specijalizovana rasvjeta ---
    /* LED TRAKA */
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_ISPOD_ELEMENTA_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_IZNAD_ELEMENTA_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_ORMAR_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_STEPENICE_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_TV_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_AMBIJENT_SECONDARY },
    /* VENTILATOR SA SVJETLOM */
    { ICON_VENTILATOR_ICON,     TXT_VENTILATOR, TXT_DUMMY }
};

/**
 ******************************************************************************
 * @brief       Tabela predefinisanih izgleda za scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova tabela služi kao biblioteka izgleda koje korisnik može
 * odabrati prilikom kreiranja ili editovanja scene. Povezuje
 * jedinstveni vizuelni ID ikonice (`IconID`) sa odgovarajućim
 * nazivom scene (`TextID`). Koristi se u "čarobnjaku" za scene.
 ******************************************************************************
 */
static const SceneAppearance_t scene_appearance_table[] = {
    { ICON_SCENE_WIZZARD,     TXT_SCENE_WIZZARD     },
    { ICON_SCENE_MORNING,     TXT_SCENE_MORNING     },
    { ICON_SCENE_SLEEP,       TXT_SCENE_SLEEP       },
    { ICON_SCENE_LEAVING,     TXT_SCENE_LEAVING     },
    { ICON_SCENE_HOMECOMING,  TXT_SCENE_HOMECOMING  },
    { ICON_SCENE_MOVIE,       TXT_SCENE_MOVIE       },
    { ICON_SCENE_DINNER,      TXT_SCENE_DINNER      },
    { ICON_SCENE_READING,     TXT_SCENE_READING     },
    { ICON_SCENE_RELAXING,    TXT_SCENE_RELAXING    },
    { ICON_SCENE_GATHERING,   TXT_SCENE_GATHERING   }
};

/**
 ******************************************************************************
 * @brief       Sveobuhvatna tabela koja mapira ID izgleda kapije na ikonicu i tekstove.
 * @author      Gemini
 * @note        Ovo je centralna biblioteka predefinisanih naziva za sve tipove
 * uređaja za kontrolu pristupa. Korisnik u meniju za podešavanja
 * (`SCREEN_SETTINGS_8`) bira jedan od ovih unosa preko `SPINBOX`
 * kontrole. Svaki unos kombinuje jedan osnovni tip uređaja (npr. Krilna
 * kapija) sa jednom sekundarnom odrednicom (npr. Glavni, Ulaz, 1).
 * Ovakva struktura omogućava stotine logičkih i deskriptivnih
 * kombinacija bez potrebe za izmjenom koda.
 ******************************************************************************
 */
static const IconMapping_t gate_appearance_mapping_table[] = {
    // === 1. KRILNA KAPIJA (ICON_GATE_SWING) ===
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_GLAVNI },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_SPOREDNI },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_SERVISNI },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_KUCA_ZA_GOSTE  },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_ULAZ },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_IZLAZ },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_PROLAZ },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_PREDNJA },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_ZADNJA },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_LIJEVA },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_DESNA },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_SJEVER },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_JUG },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_ISTOK },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_ZAPAD },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_KOVANA },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_1 },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_2 },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_3 },
    { ICON_GATE_SWING, TXT_GATE_SWING, TXT_GATE_SECONDARY_4 },
    
    // === 2. KLIZNA KAPIJA (ICON_GATE_SLIDING) ===
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_GLAVNI },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_SPOREDNI },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_SERVISNI },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_KUCA_ZA_GOSTE  },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_ULAZ },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_IZLAZ },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_PROLAZ },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_PREDNJA },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_ZADNJA },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_ISTOK },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_ZAPAD },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_KOVANA },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_MODERNA },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_1 },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_2 },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_3 },
    { ICON_GATE_SLIDING, TXT_GATE_SLIDING, TXT_GATE_SECONDARY_4 },

    // === 3. GARAŽA (ICON_GATE_GARAGE) ===
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_GLAVNI },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_SPOREDNI },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_LIJEVA },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_DESNA },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_GORNJA },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_DONJA },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_KUCA_ZA_GOSTE  },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_SERVISNI },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_1 },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_2 },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_3 },
    { ICON_GATE_GARAGE, TXT_GATE_GARAGE, TXT_GATE_SECONDARY_4 },
    
    // === 4. RAMPA (ICON_GATE_RAMP) ===
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_ULAZ },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_IZLAZ },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_GLAVNI },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_SPOREDNI },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_KUCA_ZA_GOSTE  },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_SERVISNI },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_1 },
    { ICON_GATE_RAMP, TXT_GATE_RAMP, TXT_GATE_SECONDARY_2 },
    
    // === 5. BRAVA (ICON_GATE_PEDESTRIAN_LOCK) ===
    { ICON_GATE_PEDESTRIAN_LOCK, TXT_GATE_PEDESTRIAN_LOCK, TXT_GATE_SECONDARY_ULAZ },
    { ICON_GATE_PEDESTRIAN_LOCK, TXT_GATE_PEDESTRIAN_LOCK, TXT_GATE_SECONDARY_SERVISNI },
    { ICON_GATE_PEDESTRIAN_LOCK, TXT_GATE_PEDESTRIAN_LOCK, TXT_GATE_SECONDARY_BAZEN },
    { ICON_GATE_PEDESTRIAN_LOCK, TXT_GATE_PEDESTRIAN_LOCK, TXT_GATE_SECONDARY_VRT },
    { ICON_GATE_PEDESTRIAN_LOCK, TXT_GATE_PEDESTRIAN_LOCK, TXT_GATE_SECONDARY_VINARIJA },

    // === 6. SIGURNOSNA VRATA (ICON_GATE_SECURITY_DOOR) ===
    { ICON_GATE_SECURITY_DOOR, TXT_GATE_SECURITY_DOOR, TXT_GATE_SECONDARY_GLAVNI },
    { ICON_GATE_SECURITY_DOOR, TXT_GATE_SECURITY_DOOR, TXT_GATE_SECONDARY_ULAZ },
    
    // === 7. PODZEMNA RAMPA (ICON_GATE_UNDERGROUND_RAMP) ===
    { ICON_GATE_UNDERGROUND_RAMP, TXT_GATE_UNDERGROUND_RAMP, TXT_GATE_SECONDARY_ULAZ },
    { ICON_GATE_UNDERGROUND_RAMP, TXT_GATE_UNDERGROUND_RAMP, TXT_GATE_SECONDARY_IZLAZ },
    { ICON_GATE_UNDERGROUND_RAMP, TXT_GATE_UNDERGROUND_RAMP, TXT_GATE_SECONDARY_1 },
    { ICON_GATE_UNDERGROUND_RAMP, TXT_GATE_UNDERGROUND_RAMP, TXT_GATE_SECONDARY_2 },
};

//__attribute__((section(".flash_rom")))
/**
 * @brief Glavna tabela sa prevodima.
 * @note  ISPRAVLJENA VERZIJA: Dodan nedostajući red prevoda za TXT_GATE_SECURITY_DOOR
 * kako bi se svi TextID-jevi i stringovi ponovo uskladili.
 */
static const char* language_strings[TEXT_COUNT][LANGUAGE_COUNT] = {
    /* TXT_DUMMY */                     { "", "", "","", "", "", "", "", "", "", "" },
    /* TXT_LIGHTS */                    { "SVJETLA", "LIGHTS", "LICHTER", "LUMIÈRES", "LUCI", "LUCES", "СВЕТ", "СВІТЛО", "ŚWIATŁA", "SVĚTLA", "SVETLÁ" },
    /* TXT_THERMOSTAT */                { "TERMOSTAT", "THERMOSTAT", "THERMOSTAT", "THERMOSTAT", "TERMOSTATO", "TERMOSTATO", "ТЕРМОСТАТ", "ТЕРМОСТАТ", "TERMOSTAT", "TERMOSTAT", "TERMOSTAT" },
    /* TXT_BLINDS */                    { "ROLETNE", "BLINDS", "JALOUSIEN", "STORES", "PERSIANE", "PERSIANAS", "ЖАЛЮЗИ", "ЖАЛЮЗІ", "ROLETY", "ŽALUZIE", "ŽALÚZIE" },
    /* TXT_DEFROSTER */                 { "ODMRZIVAČ", "DEFROSTER", "ENTEISER", "DÉGIVREUR", "SBRINATORE", "DESCONGELADOR", "РАЗМОРОЗКА", "РОЗМОРОЖУВАННЯ", "ODSZRANIACZ", "ODMRAZOVAČ", "ODMRAZOVAČ" },
    /* TXT_VENTILATOR */                { "VENTILATOR", "FAN", "LÜFTER", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_CLEAN */                     { "ČIŠĆENJE", "CLEAN", "REINIGEN", "NETTOYER", "PULIRE", "LIMPIAR", "ЧИСТКА", "ЧИЩЕННЯ", "CZYŚĆ", "ČISTIT", "ČISTIŤ" },
    /* TXT_WIFI */                      { "Wi-Fi", "Wi-Fi", "WLAN", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI" },
    /* TXT_APP */                       { "APP", "APP", "APP", "APPLI", "APP", "APP", "ПРИЛ.", "ДОДАТОК", "APLIKACJA", "APLIKACE", "APLIKÁCIA" },
    /* TXT_GATE */                      { "KAPIJA", "GATE", "TOR", "PORTAIL", "CANCELLO", "PUERTA", "ВОРОТА", "ВОРОТА", "BRAMA", "BRÁNA", "BRÁNA" },
    /* TXT_TIMER */                     { "TAJMER", "TIMER", "TIMER", "MINUTERIE", "TIMER", "TEMPORIZADOR", "ТАЙМЕР", "ТАЙМЕР", "MINUTNIK", "ČASOVAČ", "ČASOVAČ" },
    /* TXT_SECURITY */                  { "ALARM", "SECURITY", "SICHERHEIT", "SÉCURITÉ", "SICUREZZA", "SEGURIDAD", "ОХРАНА", "ОХОРОНА", "ALARM", "ZABEZPEČENÍ", "ZABEZPEČENIE" },
    /* TXT_SCENES */                    { "SCENE", "SCENES", "SZENEN", "SCÈNES", "SCENE", "ESCENAS", "СЦЕНЫ", "СЦЕНИ", "SCENY", "SCÉNY", "SCÉNY" },
    /* TXT_LANGUAGE_SOS_ALL_OFF */      { "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS" },
    /* TXT_ALL */                       { "SVE", "ALL", "ALLE", "TOUT", "TUTTI", "TODOS", "ВСЕ", "ВСІ", "WSZYSTKO", "VŠECHNY", "VŠETKY" },
    /* TXT_SETTINGS */                  { "POSTAVKE", "SETTINGS", "EINSTELLUNGEN", "RÉGLAGES", "IMPOSTAZIONI", "AJUSTES", "НАСТРОЙКИ", "НАЛАШТУВАННЯ", "USTAWIENIA", "NASTAVENÍ", "NASTAVENIA" },
    /* TXT_GLOBAL_SETTINGS */           { "Globalno Podešavanje", "Global Settings", "Globale Einstellungen", "Réglages Globaux", "Impostazioni Globali", "Ajustes Globales", "Глобальные Настройки", "Глобальні Налаштування", "Ustawienia Globalne", "Globální Nastavení", "Globálne Nastavenia" },
    /* TXT_SAVE */                      { "SNIMI", "SAVE", "SPEICHERN", "SAUVEGARDER", "SALVA", "GUARDAR", "СОХРАНИТЬ", "ЗБЕРЕГТИ", "ZAPISZ", "ULOŽIT", "ULOŽIŤ" },
    /* TXT_ENTER_NEW_NAME */            { "UNESITE NOVI NAZIV", "ENTER NEW NAME", "NEUEN NAMEN EINGEBEN", "ENTREZ LE NOUVEAU NOM", "INSERIRE NUOVO NOME", "INTRODUZCA NUEVO NOMBRE", "ВВЕДИТЕ НОВОЕ ИМЯ", "ВВЕДІТЬ НОВУ НАЗВУ", "WPROWADŹ NOWĄ NAZWĘ", "ZADEJTE NOVÝ NÁZEV", "ZADAJTE NOVÝ NÁZOV" },
    /* TXT_CANCEL */                    { "OTKAŽI", "CANCEL", "ABBRECHEN", "ANNULER", "ANNULLA", "CANCELAR", "ОТМЕНА", "СКАСУВАТИ", "ANULUJ", "ZRUŠIT", "ZRUŠIŤ" },
    /* TXT_DELETE */                    { "OBRIŠI", "DELETE", "LÖSCHEN", "SUPPRIMER", "ELIMINA", "ELIMINAR", "УДАЛИТЬ", "ВИДАЛИТИ", "USUŃ", "SMAZAT", "VYMAZAŤ" },
    /* TXT_CONFIGURE_DEVICE_MSG */      { "Uređaj nije konfigurisan.", "Device not configured.", "Gerät nicht konfiguriert.", "Appareil non configuré.", "Dispositivo non configurato.", "Dispositivo no configurado.", "Устройство не настроено.", "Пристрій не налаштовано.", "Urządzenie nie jest skonfigurowane.", "Zařízení není nakonfigurováno.", "Zariadenie nie je nakonfigurované." },
    /* TXT_SCENE_SAVED_MSG */           { "Scena snimljena.", "Scene saved.", "Szene gespeichert.", "Scène enregistrée.", "Scena salvata.", "Escena guardada.", "Сцена сохранена.", "Сцену збережено.", "Scena zapisana.", "Scéna uložena.", "Scéna uložená." },
    /* TXT_PLEASE_CONFIGURE_SCENE_MSG */{ "Molimo konfigurišite scenu.", "Please configure the scene.", "Bitte konfigurieren Sie die Szene.", "Veuillez configurer la scène.", "Si prega di configurare la scena.", "Por favor, configure la escena.", "Пожалуйста, настройте сцену.", "Будь ласка, налаштуйте сцену.", "Proszę skonfigurować scenę.", "Nakonfigurujte prosím scénu.", "Nakonfigurujte prosím scénu." },
    /* TXT_TIMER_ENABLED */             { "Tajmer je uključen", "Timer is enabled", "Timer ist aktiviert", "Minuterie activée", "Timer abilitato", "Temporizador activado", "Таймер включен", "Таймер увімкнено", "Timer jest włączony", "Časovač je zapnutý", "Časovač je zapnutý" },
    /* TXT_TIMER_DISABLED */            { "Tajmer je isključen", "Timer is disabled", "Timer ist deaktiviert", "Minuterie désactivée", "Timer disabilitato", "Temporizador desactivado", "Таймер выключен", "Таймер вимкнено", "Timer jest wyłączony", "Časovač je vypnutý", "Časovač je vypnutý" },
    /* TXT_TIMER_EVERY_DAY */           { "Svaki dan", "Every day", "Jeden Tag", "Chaque jour", "Ogni giorno", "Cada día", "Каждый день", "Кожен день", "Codziennie", "Každý den", "Každý deň" },
    /* TXT_TIMER_WEEKDAYS */            { "Radnim danima", "Weekdays", "Wochentags", "En semaine", "Giorni feriali", "De lunes a viernes", "По будням", "У будні", "W dni robocze", "Všední dny", "Pracovné dni" },
    /* TXT_TIMER_WEEKEND */             { "Vikendom", "Weekends", "Wochenende", "Le week-end", "Fine settimana", "Fines de semana", "По выходным", "На вихідних", "W weekendy", "Víkendy", "Víkendy" },
    /* TXT_TIMER_ONCE */                { "Jednokratno", "Once", "Einmal", "Une fois", "Una volta", "Una vez", "Один раз", "Один раз", "Jednorazowo", "Jednou", "Jedenkrát" },
    /* TXT_TIMER_USE_BUZZER */          { "BUZZER", "BUZZER", "SUMMER", "BUZZER", "CICALINO", "ZUMBADOR", "ЗУММЕР", "ЗУМЕР", "BRZĘCZYK", "BZUČÁK", "BZUČIAK" },
    /* TXT_TIMER_TRIGGER_SCENE */       { "SCENA", "SCENE", "SZENE", "SCÈNE", "SCENA", "ESCENA", "СЦЕНА", "СЦЕНА", "SCENA", "SCÉNA", "SCÉNA" },
    /* TXT_ALARM_WAKEUP */              { "BUĐENJE!!!", "WAKE UP!!!", "AUFWACHEN!!!", "RÉVEILLEZ-VOUS!!!", "SVEGLIA!!!", "¡DESPIERTA!", "ПОДЪЕМ!!!", "ПРОКИДАЙТЕСЬ!!!", "POBUDKA!!!", "VSTÁVAT!!!", "VSTÁVAŤ!!!" },
    /* TXT_DISPLAY_CLEAN_TIME */        { "VRIJEME BRISANJA EKRANA:", "DISPLAY CLEAN TIME:", "BILDSCHIRMREINIGUNGSZEIT:", "TEMPS DE NETTOYAGE:", "TEMPO PULIZIA:", "TIEMPO DE LIMPIEZA:", "ВРЕМЯ ОЧИСТКИ:", "ЧАС ОЧИЩЕННЯ:", "CZAS CZYSZCZENIA:", "ČAS ČIŠTĚNÍ:", "ČAS ČISTENIA:" },
    /* TXT_FIRMWARE_UPDATE */           { "AŽURIRANJE...", "UPDATING...", "AKTUALISIERUNG...", "MISE À JOUR...", "AGGIORNAMENTO...", "ACTUALIZANDO...", "ОБНОВЛЕНИЕ...", "ОНОВЛЕННЯ...", "AKTUALIZACJA...", "AKTUALIZACE...", "AKTUALIZÁCIA..." },
    /* TXT_UPDATE_IN_PROGRESS */        { "Ažuriranje u toku, molimo sacekajte...", "Update in progress, please wait...", "Aktualisierung läuft, bitte warten...", "Mise à jour en cours, veuillez patienter...", "Aggiornamento in corso, attendere prego...", "Actualización en curso, por favor espere...", "Идет обновление, подождите...", "Триває оновлення, зачекайте...", "Aktualizacja w toku, proszę czekać...", "Probíhá aktualizace, čekejte prosím...", "Prebieha aktualizácia, čakajte prosím..." },
    /* TXT_MONDAY */                    { "Ponedjeljak", "Monday", "Montag", "Lundi", "Lunedì", "Lunes", "Понедельник", "Понеділок", "Poniedziałek", "Pondělí", "Pondelok" },
    /* TXT_TUESDAY */                   { "Utorak", "Tuesday", "Dienstag", "Mardi", "Martedì", "Martes", "Вторник", "Вівторок", "Wtorek", "Úterý", "Utorok" },
    /* TXT_WEDNESDAY */                 { "Srijeda", "Wednesday", "Mittwoch", "Mercredi", "Mercoledì", "Miércoles", "Среда", "Середа", "Środa", "Středa", "Streda" },
    /* TXT_THURSDAY */                  { "Četvrtak", "Thursday", "Donnerstag", "Jeudi", "Giovedì", "Jueves", "Четверг", "Четвер", "Czwartek", "Čtvrtek", "Štvrtok" },
    /* TXT_FRIDAY */                    { "Petak", "Friday", "Freitag", "Vendredi", "Venerdì", "Viernes", "Пятница", "П'ятниця", "Piątek", "Pátek", "Piatok" },
    /* TXT_SATURDAY */                  { "Subota", "Saturday", "Samstag", "Samedi", "Sabato", "Sábado", "Суббота", "Субота", "Sobota", "Sobota", "Sobota" },
    /* TXT_SUNDAY */                    { "Nedjelja", "Sunday", "Sonntag", "Dimanche", "Domenica", "Domingo", "Воскресенье", "Неділя", "Niedziela", "Neděle", "Nedeľa" },
    /* TXT_MONTH_JAN */                 { "Januar", "January", "Januar", "Janvier", "Gennaio", "Enero", "Январь", "Січень", "Styczeń", "Leden", "Január" },
    /* TXT_MONTH_FEB */                 { "Februar", "February", "Februar", "Février", "Febbraio", "Febrero", "Февраль", "Лютий", "Luty", "Únor", "Február" },
    /* TXT_MONTH_MAR */                 { "Mart", "March", "März", "Mars", "Marzo", "Marzo", "Март", "Березень", "Marzec", "Březen", "Marec" },
    /* TXT_MONTH_APR */                 { "April", "April", "April", "Avril", "Aprile", "Abril", "Апрель", "Квітень", "Kwiecień", "Duben", "Apríl" },
    /* TXT_MONTH_MAY */                 { "Maj", "May", "Mai", "Mai", "Maggio", "Mayo", "Май", "Травень", "Maj", "Květen", "Máj" },
    /* TXT_MONTH_JUN */                 { "Juni", "June", "Juni", "Juin", "Giugno", "Junio", "Июнь", "Червень", "Czerwiec", "Červen", "Jún" },
    /* TXT_MONTH_JUL */                 { "Juli", "July", "Juli", "Juillet", "Luglio", "Julio", "Июль", "Липень", "Lipiec", "Červenec", "Júl" },
    /* TXT_MONTH_AUG */                 { "August", "August", "August", "Août", "Agosto", "Agosto", "Август", "Серпень", "Sierpień", "Srpen", "August" },
    /* TXT_MONTH_SEP */                 { "Septembar", "September", "September", "Septembre", "Settembre", "Septiembre", "Сентябрь", "Вересень", "Wrzesień", "Září", "September" },
    /* TXT_MONTH_OCT */                 { "Oktobar", "October", "Oktober", "Octobre", "Ottobre", "Octubre", "Октябрь", "Жовтень", "Październik", "Říjen", "Október" },
    /* TXT_MONTH_NOV */                 { "Novembar", "November", "November", "Novembre", "Novembre", "Noviembre", "Ноябрь", "Листопад", "Listopad", "Listopad", "November" },
    /* TXT_MONTH_DEC */                 { "Decembar", "December", "Dezember", "Décembre", "Dicembre", "Diciembre", "Декабрь", "Грудень", "Grudzień", "Prosinec", "December" },
    /* TXT_LANGUAGE_NAME */             { "Bos/Cro/Srb/Mon", "English", "Deutsch", "Français", "Italiano", "Español", "Русский", "Українська", "Polski", "Čeština", "Slovenčina" },
    /* TXT_DATETIME_SETUP_TITLE */      { "Podesite Datum i Vrijeme", "Set Date and Time", "Datum und Uhrzeit einstellen", "Régler la date et l'heure", "Imposta data e ora", "Configurar fecha y hora", "Установить дату и время", "Встановити дату та час", "Ustaw datę i godzinę", "Nastavte datum a čas", "Nastavte dátum a čas" },
    /* TXT_TIMER_SETTINGS_TITLE */      { "Podešavanje tajmera", "Timer Settings", "Timer-Einstellungen", "Réglages de la minuterie", "Impostazioni timer", "Configuración del temporizador", "Настройки таймера", "Налаштування таймера", "Ustawienia timera", "Nastavení časovače", "Nastavenia časovača" },
    /* TXT_DAY */                       { "Dan", "Day", "Tag", "Jour", "Giorno", "Día", "День", "День", "Dzień", "Den", "Deň" },
    /* TXT_MONTH */                     { "Mjesec", "Month", "Monat", "Mois", "Mese", "Mes", "Месяц", "Місяць", "Miesiąc", "Měsíc", "Mesiac" },
    /* TXT_YEAR */                      { "Godina", "Year", "Jahr", "Année", "Anno", "Año", "Год", "Рік", "Rok", "Rok", "Rok" },
    /* TXT_HOUR */                      { "Sat", "Hour", "Stunde", "Heure", "Ora", "Hora", "Час", "Година", "Godzina", "Hodina", "Hodina" },
    /* TXT_MINUTE */                    { "Minuta", "Minute", "Minute", "Minuto", "Minuto", "Минута", "Хвилина", "Minuta", "Minuta", "Minúta" },
    /* TXT_LUSTER */                    { "LUSTER", "CHANDELIER", "KRONLEUCHTER", "LUSTRE", "LAMPADARIO", "ARAÑA", "ЛЮСТРА", "ЛЮСТРА", "ŻYRANDOL", "LUSTR", "LUSTER" },
    /* TXT_SPOT */                      { "SPOT", "SPOT", "STRAHLER", "SPOT", "FARETTO", "FOCO", "ТОЧЕЧНЫЙ", "ТОЧКОВИЙ", "PUNKTOWE", "BODOVÉ", "BODOVÉ" },
    /* TXT_VISILICA */                  { "VISILICA", "PENDANT", "HÄNGELEUCHTE", "SUSPENSION", "SOSPENSIONE", "COLGANTE", "ПОДВЕС", "ПІДВІС", "WISZĄCA", "ZÁVĚSNÉ", "ZÁVESNÉ" },
    /* TXT_PLAFONJERA */                { "PLAFONJERA", "CEILING", "DECKENLEUCHTE", "PLAFONNIER", "PLAFONIERA", "PLAFÓN", "ПОТОЛОЧНЫЙ", "СТЕЛЬОВИЙ", "PLAFON", "STROPNÍ", "STROPNÉ" },
    /* TXT_ZIDNA */                     { "ZIDNA", "WALL", "WANDLEUCHTE", "APPLIQUE", "DA PARETE", "DE PARED", "НАСТЕННЫЙ", "НАСТІННИЙ", "KINKIET", "NÁSTĚNNÉ", "NÁSTENNÉ" },
    /* TXT_SLIKA */                     { "SLIKA", "PICTURE", "BILDERLEUCHTE", "TABLEAU", "QUADRO", "CUADRO", "КАРТИНА", "КАРТИНА", "OBRAZ", "OBRAZ", "OBRAZ" },
    /* TXT_PODNA */                     { "PODNA", "FLOOR", "STEHLEUCHTE", "LAMPADAIRE", "DA TERRA", "DE PIE", "НАПОЛЬНЫЙ", "ПІДЛОГОВИЙ", "PODŁOGOWA", "STOJACÍ", "STOJACA" },
    /* TXT_STOLNA */                    { "STOLNA", "TABLE", "TISCHLEUCHTE", "DE TABLE", "DA TAVOLO", "DE MESA", "НАСТОЛЬНЫЙ", "НАСТІЛЬНИЙ", "STOŁOWA", "STOLNÍ", "STOLNÁ" },
    /* TXT_LED_TRAKA */                 { "LED TRAKA", "LED STRIP", "LED-STREIFEN", "BANDE LED", "STRISCIA LED", "TIRA LED", "ЛЕД-ЛЕНТА", "ЛЕД-СТРІЧКА", "TAŚMA LED", "LED PÁSEK", "LED PÁSIK" },
    /* TXT_VENTILATOR_IKONA */          { "VENTILATOR", "FAN", "VENTILATOR", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_FASADA */                    { "FASADA", "FACADE", "FASSADE", "FAÇADE", "FACCIATA", "FACHADA", "ФАСАД", "ФАСАД", "FASADA", "FASÁDA", "FASÁDA" },
    /* TXT_STAZA */                     { "STAZA", "PATH", "WEG", "CHEMIN", "SENTIERO", "CAMINO", "ДОРОЖКА", "ДОРІЖКА", "ŚCIEŻKA", "CESTA", "CESTA" },
    /* TXT_REFLEKTOR */                 { "REFLEKTOR", "FLOODLIGHT", "SCHEINWERFER", "PROJECTEUR", "PROIETTORE", "REFLECTOR", "ПРОЖЕКТОР", "ПРОЖЕКТОР", "REFLEKTOR", "REFLEKTOR", "REFLEKTOR" },
    /* TXT_SCENE_WIZZARD */             { "Dodaj Scenu", "Add Scene", "Szene hinzufügen", "Ajouter une scène", "Aggiungi scena", "Añadir escena", "Добавить сцену", "Додати сцену", "Dodaj scenę", "Přidat scénu", "Pridať scénu" },
    /* TXT_SCENE_MORNING */             { "Jutro", "Morning", "Morgen", "Matin", "Mattina", "Mañana", "Утро", "Ранок", "Poranek", "Ráno", "Ráno" },
    /* TXT_SCENE_SLEEP */               { "Spavanje", "Sleep", "Schlafen", "Dormir", "Sonno", "Dormir", "Сон", "Сон", "Sen", "Spánek", "Spánok" },
    /* TXT_SCENE_LEAVING */             { "Odlazak", "Leaving", "Verlassen", "Départ", "Uscita", "Salida", "Уход", "Вихід", "Wyjście", "Odchod", "Odchod" },
    /* TXT_SCENE_HOMECOMING */          { "Povratak", "Homecoming", "Heimkehr", "Retour", "Ritorno a casa", "Regreso a casa", "Возвращение", "Повернення", "Powrót do domu", "Návrat domů", "Návrat domov" },
    /* TXT_SCENE_MOVIE */               { "Film", "Movie", "Film", "Film", "Film", "Película", "Фильм", "Фільм", "Film", "Film", "Film" },
    /* TXT_SCENE_DINNER */              { "Večera", "Dinner", "Abendessen", "Dîner", "Cena", "Cena", "Ужин", "Вечеря", "Kolacja", "Večeře", "Večera" },
    /* TXT_SCENE_READING */             { "Čitanje", "Reading", "Lesen", "Lecture", "Lettura", "Lectura", "Чтение", "Читання", "Czytanie", "Čtení", "Čítanie" },
    /* TXT_SCENE_RELAXING */            { "Opuštanje", "Relaxing", "Entspannen", "Détente", "Rilassante", "Relajación", "Расслабление", "Розслаблення", "Relaks", "Odpočinek", "Oddych" },
    /* TXT_SCENE_GATHERING */           { "Druženje", "Gathering", "Treffen", "Rassemblement", "Incontro", "Reunión", "Сбор", "Збори", "Spotkanie", "Setkání", "Stretnutie" },
    /* TXT_GATE_SWING */                { "Krilna Kapija", "Swing Gate", "Flügeltor", "Portail Battant", "Cancello a Battente", "Puerta Batiente", "Распашные Ворота", "Розпашні Ворота", "Brama Skrzydłowa", "Křídlová Brána", "Krídlová Brána" },
    /* TXT_GATE_SLIDING */              { "Klizna Kapija", "Sliding Gate", "Schiebetor", "Portail Coulissant", "Cancello Scorrevole", "Puerta Corredera", "Откатные Ворота", "Відкатні Ворота", "Brama Przesuwna", "Posuvná Brána", "Posuvná Brána" },
    /* TXT_GATE_GARAGE */               { "Garaža", "Garage", "Garage", "Garage", "Garage", "Garaje", "Гараж", "Гараж", "Garaż", "Garáž", "Garáž" },
    /* TXT_GATE_RAMP */                 { "Rampa", "Barrier", "Schranke", "Barrière", "Barriera", "Barrera", "Шлагбаум", "Шлагбаум", "Szlaban", "Závora", "Rampa" },
    /* TXT_GATE_PEDESTRIAN_LOCK */      { "Brava", "Lock", "Schloss", "Serrure", "Serratura", "Cerradura", "Замок", "Замок", "Zamek", "Zámek", "Zámok" },
    /* TXT_GATE_SECURITY_DOOR */        { "Sigurnosna Vrata", "Security Door", "Sicherheitstür", "Porte de Sécurité", "Porta Blindata", "Puerta de Seguridad", "Бронированная Дверь", "Броньовані Двері", "Drzwi Bezpieczeństwa", "Bezpečnostní Dveře", "Bezpečnostné Dvere" },
    /* TXT_GATE_UNDERGROUND_RAMP */     { "Podzemna Rampa", "Underground Ramp", "Tiefgarage Rampe", "Rampe Souterraine", "Rampa Sotterranea", "Rampa Subterránea", "Подземный Пандус", "Підземний Пандус", "Rampa Podziemna", "Podzemní Rampa", "Podzemná Rampa" },
    /* TXT_GATE_CONTROL_PROFILE */      { "Profil Kontrole", "Control Profile", "Steuerungsprofil", "Profil de Contrôle", "Profilo di Controllo", "Perfil de Control", "Профиль Управления", "Профіль Керування", "Profil Sterowania", "Profil Ovládání", "Profil Ovládania" },
    /* TXT_GATE_APPEARANCE */           { "Izgled", "Appearance", "Erscheinungsbild", "Apparence", "Aspetto", "Apariencia", "Внешний Вид", "Зовнішній Вигляд", "Wygląd", "Vzhled", "Vzhľad" },
    /* TXT_GATE_CMD_OPEN */             { "OTVORI", "OPEN", "ÖFFNEN", "OUVRIR", "APRI", "ABRIR", "ОТКРЫТЬ", "ВІДКРИТИ", "OTWÓRZ", "OTEVŘÍT", "OTVORIŤ" },
    /* TXT_GATE_CMD_CLOSE */            { "ZATVORI", "CLOSE", "SCHLIESSEN", "FERMER", "CHIUDI", "CERRAR", "ЗАКРЫТЬ", "ЗАКРИТИ", "ZAMKNIJ", "ZAVŘÍT", "ZAVRIEŤ" },
    /* TXT_GATE_CMD_STOP */             { "STOP", "STOP", "STOP", "STOP", "STOP", "PARAR", "СТОП", "СТОП", "STOP", "STOP", "STOP" },
    /* TXT_GATE_CMD_PEDESTRIAN */       { "PJEŠAK", "PEDESTRIAN", "FUSSGÄNGER", "PIÉTON", "PEDONALE", "PEATÓN", "ПЕШЕХОД", "ПІШОХІД", "PIESZY", "CHODEC", "CHODEC" },
    /* TXT_GATE_CMD_UNLOCK */           { "OTKLJUČAJ", "UNLOCK", "ENTSPERREN", "DÉVERROUILLER", "SBLOCCA", "DESBLOQUEAR", "ОТКРЫТЬ", "ВІДІМКНУТИ", "ODBLOKUJ", "ODEMKNOUT", "ODOMKNÚŤ" },
    /* TXT_GATE_MAIN_SECONDARY */       { "GLAVNA", "MAIN", "HAUPT", "PRINCIPALE", "PRINCIPALE", "PRINCIPAL", "ГЛАВНАЯ", "ГОЛОВНА", "GŁÓWNA", "HLAVNÍ", "HLAVNÁ" },
    /* TXT_GATE_YARD_SECONDARY */       { "DVORIŠTE", "YARD", "HOF", "COUR", "CORTILE", "PATIO", "ДВОР", "ДВІР", "PODWÓRKO", "DVŮR", "DVOR" },
    /* TXT_GATE_ENTRANCE_SECONDARY */   { "ULAZ", "ENTRANCE", "EINGANG", "ENTRÉE", "INGRESSO", "ENTRADA", "ВХОД", "ВХІД", "WEJŚCIE", "VCHOD", "VCHOD" },
    /* TXT_GATE_STATUS_CLOSED */        { "ZATVORENO", "CLOSED", "GESCHLOSSEN", "FERMÉ", "CHIUSO", "CERRADO", "ЗАКРЫТО", "ЗАЧИНЕНО", "ZAMKNIĘTE", "ZAVŘENO", "ZATVORENÉ" },
    /* TXT_GATE_STATUS_OPENING */       { "OTVARANJE...", "OPENING...", "ÖFFNEN...", "OUVERTURE...", "APERTURA...", "ABRIENDO...", "ОТКРЫТИЕ...", "ВІДКРИТТЯ...", "OTWIERANIE...", "OTEVÍRÁNÍ...", "OTVÁRANIE..." },
    /* TXT_GATE_STATUS_OPEN */          { "OTVORENO", "OPEN", "OFFEN", "OUVERT", "APERTO", "ABIERTO", "ОТКРЫТО", "ВІДКРИТО", "OTWARTE", "OTEVŘENO", "OTVORENÉ" },
    /* TXT_GATE_STATUS_CLOSING */       { "ZATVARANJE...", "CLOSING...", "SCHLIESSEN...", "FERMETURE...", "CHIUSURA...", "CERRANDO...", "ЗАКРЫТИЕ...", "ЗАЧИНЕННЯ...", "ZAMYKANIE...", "ZAVÍRÁNÍ...", "ZATVÁRANIE..." },
    /* TXT_GATE_STATUS_PARTIAL */       { "DJELIMIČNO", "PARTIAL", "TEILWEISE", "PARTIEL", "PARZIALE", "PARCIAL", "ЧАСТИЧНО", "ЧАСТКОВО", "CZĘŚCIOWO", "ČÁSTEČNĚ", "ČIASTOČNE" },
    /* TXT_GATE_STATUS_FAULT */         { "GREŠKA!", "FAULT!", "FEHLER!", "ERREUR!", "GUASTO!", "¡FALLO!", "ОШИБКА!", "ПОМИЛКА!", "BŁĄD!", "CHYBA!", "CHYBA!" },
    /* TXT_GATE_STATUS_UNDEFINED */     { "NEPOZNATO", "UNKNOWN", "UNBEKANNT", "INCONNU", "SCONOSCIUTO", "DESCONOCIDO", "НЕИЗВЕСТНО", "НЕВІДОМО", "NIEZNANY", "NEZNÁMÝ", "NEZNÁMY" },
    /* TXT_GATE_SECONDARY_1 */          { "1", "1", "1", "1", "1", "1", "1", "1", "1", "1", "1" },
    /* TXT_GATE_SECONDARY_2 */          { "2", "2", "2", "2", "2", "2", "2", "2", "2", "2", "2" },
    /* TXT_GATE_SECONDARY_3 */          { "3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3" },
    /* TXT_GATE_SECONDARY_4 */          { "4", "4", "4", "4", "4", "4", "4", "4", "4", "4", "4" },
    /* TXT_GATE_SECONDARY_5 */          { "5", "5", "5", "5", "5", "5", "5", "5", "5", "5", "5" },
    /* TXT_GATE_SECONDARY_6 */          { "6", "6", "6", "6", "6", "6", "6", "6", "6", "6", "6" },
    /* TXT_GATE_SECONDARY_7 */          { "7", "7", "7", "7", "7", "7", "7", "7", "7", "7", "7" },
    /* TXT_GATE_SECONDARY_8 */          { "8", "8", "8", "8", "8", "8", "8", "8", "8", "8", "8" },
    /* TXT_GATE_SECONDARY_DONJA */      { "DONJA", "LOWER", "UNTERE", "INFÉRIEURE", "INFERIORE", "INFERIOR", "НИЖНЯЯ", "НИЖНЯ", "DOLNA", "SPODNÍ", "SPODNÁ" },
    /* TXT_GATE_SECONDARY_SREDNJA */    { "SREDNJA", "MIDDLE", "MITTLERE", "MOYENNE", "CENTRALE", "CENTRAL", "СРЕДНЯЯ", "СЕРЕДНЯ", "ŚRODKOWA", "STŘEDNÍ", "STREDNÁ" },
    /* TXT_GATE_SECONDARY_GORNJA */     { "GORNJA", "UPPER", "OBERE", "SUPÉRIEURE", "SUPERIORE", "SUPERIOR", "ВЕРХНЯЯ", "ВЕРХНЯ", "GÓRNA", "HORNÍ", "HORNÁ" },
    /* TXT_GATE_SECONDARY_LIJEVA */     { "LIJEVA", "LEFT", "LINKE", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВАЯ", "ЛІВА", "LEWA", "LEVÁ", "ĽAVÁ" },
    /* TXT_GATE_SECONDARY_DESNA */      { "DESNA", "RIGHT", "RECHTE", "DROITE", "DESTRA", "DERECHA", "ПРАВАЯ", "ПРАВА", "PRAWA", "PRAVÁ", "PRAVÁ" },
    /* TXT_GATE_SECONDARY_PREDNJA */    { "PREDNJA", "FRONT", "VORDERE", "AVANT", "ANTERIORE", "DELANTERA", "ПЕРЕДНЯЯ", "ПЕРЕДНЯ", "PRZEDNIA", "PŘEDNÍ", "PREDNÁ" },
    /* TXT_GATE_SECONDARY_ZADNJA */     { "ZADNJA", "REAR", "HINTERE", "ARRIÈRE", "POSTERIORE", "TRASERA", "ЗАДНЯЯ", "ЗАДНЯ", "TYLNA", "ZADNÍ", "ZADNÁ" },
    /* TXT_GATE_SECONDARY_ISTOK */      { "ISTOK", "EAST", "OST", "EST", "EST", "ESTE", "ВОСТОК", "СХІД", "WSCHÓD", "VÝCHOD", "VÝCHOD" },
    /* TXT_GATE_SECONDARY_ZAPAD */      { "ZAPAD", "WEST", "WEST", "OUEST", "OVEST", "OESTE", "ЗАПАД", "ЗАХІД", "ZACHÓD", "ZÁPAD", "ZÁPAD" },
    /* TXT_GATE_SECONDARY_SJEVER */     { "SJEVER", "NORTH", "NORD", "NORD", "NORD", "NORTE", "СЕВЕР", "ПІВНІЧ", "PÓŁNOC", "SEVER", "SEVER" },
    /* TXT_GATE_SECONDARY_JUG */        { "JUG", "SOUTH", "SÜD", "SUD", "SUD", "SUR", "ЮГ", "ПІВДЕНЬ", "POŁUDNIE", "JIH", "JUH" },
    /* TXT_GATE_SECONDARY_ULAZ */       { "ULAZ", "ENTRANCE", "EINGANG", "ENTRÉE", "INGRESSO", "ENTRADA", "ВХОД", "ВХІД", "WEJŚCIE", "VCHOD", "VCHOD" },
    /* TXT_GATE_SECONDARY_IZLAZ */      { "IZLAZ", "EXIT", "AUSGANG", "SORTIE", "USCITA", "SALIDA", "ВЫХОД", "ВИХІД", "WYJŚCIE", "VÝCHOD", "VÝCHOD" },
    /* TXT_GATE_SECONDARY_PROLAZ */     { "PROLAZ", "PASSAGE", "DURCHGANG", "PASSAGE", "PASSAGGIO", "PASAJE", "ПРОХОД", "ПРОХІД", "PRZEJŚCIE", "PRŮCHOD", "PRIECHOD" },
    /* TXT_GATE_SECONDARY_GLAVNI */     { "GLAVNI", "MAIN", "HAUPT", "PRINCIPAL", "PRINCIPALE", "PRINCIPAL", "ГЛАВНЫЙ", "ГОЛОВНИЙ", "GŁÓWNY", "HLAVNÍ", "HLAVNÝ" },
    /* TXT_GATE_SECONDARY_SPOREDNI */   { "SPOREDNI", "SIDE", "NEBEN", "SECONDAIRE", "SECONDARIO", "SECUNDARIO", "БОКОВОЙ", "БІЧНИЙ", "BOCZNY", "VEDLEJŠÍ", "VEDĽAJŠÍ" },
    /* TXT_GATE_SECONDARY_SERVISNI */   { "SERVISNI", "SERVICE", "SERVICE", "SERVICE", "SERVIZIO", "SERVICIO", "СЛУЖЕБНЫЙ", "СЛУЖБОВИЙ", "SERWISOWY", "SERVISNÍ", "SERVISNÝ" },
    /* TXT_GATE_SECONDARY_PRIVATNI */   { "PRIVATNI", "PRIVATE", "PRIVAT", "PRIVÉ", "PRIVATO", "PRIVADO", "ЧАСТНЫЙ", "ПРИВАТНИЙ", "PRYWATNY", "SOUKROMÝ", "SÚKROMNÝ" },
    /* TXT_GATE_SECONDARY_DOSTAVA */    { "DOSTAVA", "DELIVERY", "LIEFERUNG", "LIVRAISON", "CONSEGNA", "ENTREGA", "ДОСТАВКА", "ДОСТАВКА", "DOSTAWA", "DORUČENÍ", "DORUČENIE" },
    /* TXT_GATE_SECONDARY_KUCA_ZA_GOSTE */ { "KUĆA ZA GOSTE", "GUEST HOUSE", "GÄSTEHAUS", "MAISON D'AMIS", "CASA DEGLI OSPITI", "CASA DE HUÉSPEDES", "ГОСТЕВОЙ ДОМ", "ГОСТЬОВИЙ ДІМ", "DOM GOŚCINNY", "DŮM PRO HOSTY", "DOM PRE HOSTÍ" },
    /* TXT_GATE_SECONDARY_BAZEN */      { "BAZEN", "POOL", "POOL", "PISCINE", "PISCINA", "PISCINA", "БАССЕЙН", "БАСЕЙН", "BASEN", "BAZÉN", "BAZÉN" },
    /* TXT_GATE_SECONDARY_TENISKI_TEREN */{ "TENIS", "TENNIS", "TENNIS", "TENNIS", "TENNIS", "TENIS", "ТЕННИС", "ТЕНІС", "TENIS", "TENIS", "TENIS" },
    /* TXT_GATE_SECONDARY_VINARIJA */   { "VINARIJA", "WINERY", "WEINKELLER", "CAVE À VIN", "CANTINA", "BODEGA", "ВИНОДЕЛЬНЯ", "ВИНОРОБНЯ", "WINIARNIA", "VINAŘSTVÍ", "VINÁRSTVO" },
    /* TXT_GATE_SECONDARY_KONJUSNICA */ { "KONJUŠNICA", "STABLES", "STALLUNGEN", "ÉCURIES", "SCUDERIE", "ESTABLOS", "КОНЮШНЯ", "СТАЙНЯ", "STAJNIE", "STÁJE", "STAJNE" },
    /* TXT_GATE_SECONDARY_VRT */        { "VRT", "GARDEN", "GARTEN", "JARDIN", "GIARDINO", "JARDÍN", "САД", "САД", "OGRÓD", "ZAHRADA", "ZÁHRADA" },
    /* TXT_GATE_SECONDARY_PARK */       { "PARK", "PARK", "PARK", "PARC", "PARCO", "PARQUE", "ПАРК", "ПАРК", "PARK", "PARK", "PARK" },
    /* TXT_GATE_SECONDARY_JEZERO */     { "JEZERO", "LAKE", "SEE", "LAC", "LAGO", "LAGO", "ОЗЕРО", "ОЗЕРО", "JEZIORO", "JEZERO", "JAZERO" },
    /* TXT_GATE_SECONDARY_KOVANA */     { "KOVANA", "WROUGHT", "SCHMIEDEEIS.", "FER FORGÉ", "IN FERRO", "FORJADO", "КОВАНАЯ", "КОВАНА", "KUTA", "KOVANÁ", "KOVANÁ" },
    /* TXT_GATE_SECONDARY_DRVENA */     { "DRVENA", "WOODEN", "HOLZ", "EN BOIS", "IN LEGNO", "DE MADERA", "ДЕРЕВЯННАЯ", "ДЕРЕВ'ЯНА", "DREWNIANA", "DŘEVĚNÁ", "DREVENÁ" },
    /* TXT_GATE_SECONDARY_MODERNA */    { "MODERNA", "MODERN", "MODERN", "MODERNE", "MODERNO", "MODERNA", "СОВРЕМЕННАЯ", "СУЧАСНА", "NOWOCZESNA", "MODERNÍ", "MODERNÁ" },
    /* TXT_GATE_SECONDARY_KAMENA */     { "KAMENA", "STONE", "STEIN", "EN PIERRE", "IN PIETRA", "DE PIEDRA", "КАМЕННАЯ", "КАМ'ЯНА", "KAMIENNA", "KAMENNÁ", "KAMENNÁ" },
    /* TXT_GLAVNI_SECONDARY */          { "GLAVNI", "MAIN", "HAUPT", "PRINCIPAL", "PRINCIPALE", "PRINCIPAL", "ГЛАВНЫЙ", "ГОЛОВНИЙ", "GŁÓWNY", "HLAVNÍ", "HLAVNÝ" },
    /* TXT_AMBIJENT_SECONDARY */        { "AMBIJENT", "AMBIENT", "AMBIENTE", "AMBIANCE", "AMBIENTE", "AMBIENTE", "АТМОСФЕРА", "АТМОСФЕРА", "NASTRÓJ", "PROSTŘEDÍ", "PROSTREDIE" },
    /* TXT_TRPEZARIJA_SECONDARY */      { "TRPEZARIJA", "DINING", "ESSZIMMER", "SALLE À MANGER", "SALA DA PRANZO", "COMEDOR", "СТОЛОВАЯ", "ЇДАЛЬНЯ", "JADALNIA", "JÍDELNA", "JEDÁLEŇ" },
    /* TXT_DNEVNA_SOBA_SECONDARY */     { "DNEVNA SOBA", "LIVING RM", "WOHNZIMMER", "SALON", "SOGGIORNO", "SALA DE ESTAR", "ГОСТИНАЯ", "ВІТАЛЬНЯ", "SALON", "OBÝVACÍ POKOJ", "OBÝVAČKA" },
    /* TXT_LIJEVI_SECONDARY */          { "LIJEVI", "LEFT", "LINKS", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВЫЙ", "ЛІВИЙ", "LEWY", "LEVÝ", "ĽAVÝ" },
    /* TXT_DESNI_SECONDARY */           { "DESNI", "RIGHT", "RECHTS", "DROITE", "DESTRA", "DERECHA", "ПРАВЫЙ", "ПРАВИЙ", "PRAWY", "PRAVÝ", "PRAVÝ" },
    /* TXT_CENTRALNI_SECONDARY */       { "CENTRALNI", "CENTRAL", "ZENTRAL", "CENTRAL", "CENTRALE", "CENTRAL", "ЦЕНТРАЛЬНЫЙ", "ЦЕНТРАЛЬНИЙ", "CENTRALNY", "STŘEDNÍ", "STREDNÝ" },
    /* TXT_PREDNJI_SECONDARY */         { "PREDNJI", "FRONT", "VORNE", "AVANT", "ANTERIORE", "FRONTAL", "ПЕРЕДНИЙ", "ПЕРЕДНІЙ", "PRZEDNI", "PŘEDNÍ", "PREDNÝ" },
    /* TXT_ZADNJI_SECONDARY */          { "ZADNJI", "REAR", "HINTEN", "ARRIÈRE", "POSTERIORE", "TRASERA", "ЗАДНИЙ", "ЗАДНІЙ", "TYLNY", "ZADNÍ", "ZADNÝ" },
    /* TXT_HODNIK_SECONDARY */          { "HODNIK", "HALLWAY", "FLUR", "COULOIR", "CORRIDOIO", "PASILLO", "КОРИДОР", "КОРИДОР", "KORYTARZ", "CHODBA", "CHODBA" },
    /* TXT_KUHINJA_SECONDARY */         { "KUHINJA", "KITCHEN", "KÜCHE", "CUISINE", "CUCINA", "COCINA", "КУХНЯ", "КУХНЯ", "KUCHNIA", "KUCHYNĚ", "KUCHYŇA" },
    /* TXT_IZNAD_SANKA_SECONDARY */     { "IZNAD ŠANKA", "ABOVE BAR", "ÜBER THEKE", "AU-DESSUS BAR", "SOPRA BANCONE", "SOBRE BARRA", "НАД БАРОМ", "НАД БАРОМ", "NAD BAREM", "NAD BAREM", "NAD BAROM" },
    /* TXT_IZNAD_STola_SECONDARY */     { "IZNAD STOLA", "ABOVE TABLE", "ÜBER TISCH", "AU-DESSUS TABLE", "SOPRA TAVOLO", "SOBRE MESA", "НАД СТОЛОМ", "НАД СТОЛОМ", "NAD STOŁEM", "NAD STOLEM", "NAD STOLOM" },
    /* TXT_PORED_KREVETA_1_SECONDARY */ { "PORED KREVETA 1", "BESIDE BED 1", "NEBEN BETT 1", "À CÔTÉ DU LIT 1", "ACCANTO AL LETTO 1", "JUNTO A CAMA 1", "У КРОВАТИ 1", "БІЛЯ ЛІЖКА 1", "OBOK ŁÓŻKA 1", "VEDLE POSTELE 1", "VEDĽA POSTELE 1" },
    /* TXT_PORED_KREVETA_2_SECONDARY */ { "PORED KREVETA 2", "BESIDE BED 2", "NEBEN BETT 2", "À CÔTÉ DU LIT 2", "ACCANTO AL LETTO 2", "JUNTO A CAMA 2", "У КРОВАТИ 2", "БІЛЯ ЛІЖКА 2", "OBOK ŁÓŻKA 2", "VEDLE POSTELE 2", "VEDĽA POSTELE 2" },
    /* TXT_GLAVNA_SECONDARY */          { "GLAVNA", "MAIN", "HAUPT", "PRINCIPALE", "PRINCIPALE", "PRINCIPAL", "ГЛАВНАЯ", "ГОЛОВНА", "GŁÓWNA", "HLAVNÍ", "HLAVNÁ" },
    /* TXT_SOBA_1_SECONDARY */          { "SOBA 1", "ROOM 1", "ZIMMER 1", "CHAMBRE 1", "CAMERA 1", "HABITACIÓN 1", "КОМНАТА 1", "КІМНАТА 1", "POKÓJ 1", "POKOJ 1", "IZBA 1" },
    /* TXT_SOBA_2_SECONDARY */          { "SOBA 2", "ROOM 2", "ZIMMER 2", "CHAMBRE 2", "CAMERA 2", "HABITACIÓN 2", "КОМНАТА 2", "КІМНАТА 2", "POKÓJ 2", "POKOJ 2", "IZBA 2" },
    /* TXT_KUPATILO_SECONDARY */        { "KUPATILO", "BATHROOM", "BADEZIMMER", "SALLE DE BAIN", "BAGNO", "BAÑO", "ВАННАЯ", "ВАННА", "ŁAZIENKA", "KOUPELNA", "KÚPEĽŇA" },
    /* TXT_LIJEVA_SECONDARY */          { "LIJEVA", "LEFT", "LINKE", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВАЯ", "ЛІВА", "LEWA", "LEVÁ", "ĽAVÁ" },
    /* TXT_DESNA_SECONDARY */           { "DESNA", "RIGHT", "RECHTE", "DROITE", "DESTRA", "DERECHA", "ПРАВАЯ", "ПРАВА", "PRAWA", "PRAVÁ", "PRAVÁ" },
    /* TXT_GORE_SECONDARY */            { "GORE", "UP", "OBEN", "HAUT", "SU", "ARRIBA", "ВВЕРХ", "ВГОРУ", "GÓRA", "NAHORU", "HORE" },
    /* TXT_DOLE_SECONDARY */            { "DOLE", "DOWN", "UNTEN", "BAS", "GIÙ", "ABAJO", "ВНИЗ", "ВНИЗ", "DÓŁ", "DOLŮ", "DOLE" },
    /* TXT_CITANJE_SECONDARY */         { "ČITANJE", "READING", "LESEN", "LECTURE", "LETTURA", "LECTURA", "ЧТЕНИЕ", "ЧИТАННЯ", "CZYTANIE", "ČTENÍ", "ČÍTANIE" },
    /* TXT_OGLEDALO_SECONDARY */        { "OGLEDALO", "MIRROR", "SPIEGEL", "MIROIR", "SPECCHIO", "ESPEJO", "ЗЕРКАЛО", "ДЗЕРКАЛО", "LUSTRO", "ZRCADLO", "ZRKADLO" },
    /* TXT_UGAO_SECONDARY */            { "UGAO", "CORNER", "ECKE", "COIN", "ANGOLO", "ESQUINA", "УГОЛ", "КУТ", "NAROŻNIK", "ROH", "ROH" },
    /* TXT_PORED_FOTELJE_SECONDARY */   { "PORED FOTELJE", "BY ARMCHAIR", "AM SESSEL", "PRÈS DU FAUTEUIL", "VICINO POLTRONA", "JUNTO AL SILLÓN", "У КРЕСЛА", "БІЛЯ КРІСЛА", "PRZY FOTELU", "U KŘESLA", "PRI KRESLE" },
    /* TXT_RADNI_STO_SECONDARY */       { "RADNI STO", "DESK", "SCHREIBTISCH", "BUREAU", "SCRIVANIA", "ESCRITORIO", "РАБОЧИЙ СТОЛ", "РОБОЧИЙ СТІЛ", "BIURKO", "PRACOVNÍ STŮL", "PRACOVNÝ STÔL" },
    /* TXT_NOCNA_1_SECONDARY */         { "NOĆNA 1", "NIGHT 1", "NACHT 1", "NUIT 1", "NOTTE 1", "NOCHE 1", "НОЧНАЯ 1", "НІЧНА 1", "NOCNA 1", "NOČNÍ 1", "NOČNÁ 1" },
    /* TXT_NOCNA_2_SECONDARY */         { "NOĆNA 2", "NIGHT 2", "NACHT 2", "NUIT 2", "NOTTE 2", "NOCHE 2", "НОЧНАЯ 2", "НІЧНА 2", "NOCNA 2", "NOČNÍ 2", "NOČNÁ 2" },
    /* TXT_ISPOD_ELEMENTA_SECONDARY */  { "ISPOD ELEMENTA", "UNDER-CABINET", "UNTERSCHRANK", "SOUS-MEUBLE", "SOTTOMOBILE", "BAJO GABINETE", "ПОД ШКАФОМ", "ПІД ШАФОЮ", "PODSZAFKOWA", "POD SKŘÍŇKOU", "POD SKRINKOU" },
    /* TXT_IZNAD_ELEMENTA_SECONDARY */  { "IZNAD ELEMENTA", "OVER-CABINET", "OBERSCHRANK", "SUR-MEUBLE", "SOPRAMOBILE", "SOBRE GABINETE", "НАД ШКАФОМ", "НАД ШАФОЮ", "NADSZAFKOWA", "NAD SKŘÍŇKOU", "NAD SKRINKOU" },
    /* TXT_ORMAR_SECONDARY */           { "ORMAR", "WARDROBE", "KLEIDERSCHRANK", "ARMOIRE", "GUARDAROBA", "ARMARIO", "ШКАФ", "ШАФА", "SZAFKA", "ŠATNÍ SKŘÍŇ", "ŠATNÍK" },
    /* TXT_STEPENICE_SECONDARY */       { "STEPENICE", "STAIRS", "TREPPE", "ESCALIERS", "SCALE", "ESCALERAS", "ЛЕСТНИЦА", "СХОДИ", "SCHODY", "SCHODY", "SCHODY" },
    /* TXT_TV_SECONDARY */              { "TV", "TV", "FERNSEHER", "TÉLÉ", "TV", "TELE", "ТВ", "ТБ", "TELEWIZOR", "TELEVIZE", "TELEVÍZOR" },
    /* TXT_ULAZ_SECONDARY */            { "ULAZ", "ENTRANCE", "EINGANG", "ENTRÉE", "INGRESSO", "ENTRADA", "ВХОД", "ВХІД", "WEJŚCIE", "VCHOD", "VCHOD" },
    /* TXT_TERASA_SECONDARY */          { "TERASA", "TERRACE", "TERRASSE", "TERRASSE", "TERRAZZA", "TERRAZA", "ТЕРРАСА", "ТЕРАСА", "TARAS", "TERASA", "TERASA" },
    /* TXT_BALKON_SECONDARY */          { "BALKON", "BALCONY", "BALKON", "BALCON", "BALCONE", "BALCÓN", "БАЛКОН", "БАЛКОН", "BALKON", "BALKON", "BALKÓN" },
    /* TXT_ZADNJA_SECONDARY */          { "ZADNJA", "REAR", "HINTERE", "ARRIÈRE", "POSTERIORE", "TRASERA", "ЗАДНЯЯ", "ЗАДНЯ", "TYLNA", "ZADNÍ", "ZADNÁ" },
    /* TXT_PRILAZ_SECONDARY */          { "PRILAZ", "DRIVEWAY", "AUFFAHRT", "ALLÉE", "VIALE", "ENTRADA", "ПОДЪЕЗД", "ПІД'ЇЗД", "PODJAZD", "PŘÍJEZDOVÁ CESTA", "PRÍJAZDOVÁ CESTA" },
    /* TXT_DVORISTE_SECONDARY */        { "DVORIŠTE", "YARD", "HOF", "COUR", "CORTILE", "PATIO", "ДВОР", "ДВІР", "PODWÓRKO", "DVŮR", "DVOR" },
    /* TXT_DRVO_SECONDARY */            { "DRVO", "TREE", "BAUM", "ARBRE", "ALBERO", "ÁRBOL", "ДЕРЕВО", "ДЕРЕВО", "DRZEWO", "STROM", "STROM" },
    /* TXT_ALARM_SETTINGS_TITLE */      { "Podešavanje Alarma", "Alarm Settings", "Alarmeinstellungen", "Réglages de l'alarme", "Impostazioni Allarme", "Ajustes de Alarma", "Настройки Сигнализации", "Налаштування Сигналізації", "Ustawienia Alarmu", "Nastavení Alarmu", "Nastavenia Alarmu" },
    /* TXT_ALARM_SYSTEM_ARM_DISARM */   { "Sistem (Sve Particije)", "System (All Partitions)", "System (Alle Bereiche)", "Système (Toutes partitions)", "Sistema (Tutte le partizioni)", "Sistema (Todas las particiones)", "Система (Все разделы)", "Система (Всі розділи)", "Strefa 1", "Sekce 1", "Sekcia 1" },
    /* TXT_ALARM_PARTITION_1 */         { "Particija 1", "Partition 1", "Bereich 1", "Partition 1", "Partizione 1", "Partición 1", "Раздел 1", "Розділ 1", "Strefa 1", "Sekce 1", "Sekcia 1" },
    /* TXT_ALARM_PARTITION_2 */         { "Particija 2", "Partition 2", "Bereich 2", "Partition 2", "Partizione 2", "Partición 2", "Раздел 2", "Розділ 2", "Strefa 2", "Sekce 2", "Sekcia 2" },
    /* TXT_ALARM_PARTITION_3 */         { "Particija 3", "Partition 3", "Bereich 3", "Partition 3", "Partizione 3", "Partición 3", "Раздел 3", "Розділ 3", "Strefa 3", "Sekce 3", "Sekcia 3" },
    /* TXT_ALARM_RELAY_ADDRESS */       { "Adresa Releja", "Relay Address", "Relaisadresse", "Adresse du relais", "Indirizzo Relè", "Dirección del Relé", "Адрес реле", "Адреса реле", "Adres przekaźnika", "Adresa Relé", "Adresa Relé" },
    /* TXT_ALARM_FEEDBACK_ADDRESS */    { "Adresa Feedback-a", "Feedback Address", "Rückmeldeadresse", "Adresse de retour", "Indirizzo Feedback", "Dirección de Feedback", "Адрес обр. связи", "Адреса зворотного зв'язку", "Adres zwrotny", "Adresa Zpětné Vazby", "Adresa Spätnej Väzby" },
    /* TXT_ALARM_SYSTEM_STATUS_FB */    { "Feedback Statusa Alarma", "Alarm Status Feedback", "Alarmstatus Rückmeldung", "Retour d'état d'alarme", "Feedback Stato Allarme", "Feedback de Estado de Alarma", "Обр. связь статуса", "Зворотний зв'язок статусу", "Sygnał zwrotny stanu", "Zpětná vazba stavu", "Spätná väzba stavu" },
    /* TXT_ALARM_PULSE_LENGTH */        { "Dužina Pulsa (ms)", "Pulse Length (ms)", "Impulsdauer (ms)", "Durée de l'impulsion (ms)", "Durata Impulso (ms)", "Duración del Pulso (ms)", "Длит. импульса (мс)", "Тривалість імпульсу (ms)", "Długość impulsu (ms)", "Délka pulzu (ms)", "Dĺžka pulzu (ms)" },
    /* TXT_ALARM_SILENT_ALARM */        { "Tihi Alarm (SOS)", "Silent Alarm (SOS)", "Stiller Alarm (SOS)", "Alarme Silencieuse (SOS)", "Allarme Silenzioso (SOS)", "Alarma Silenciosa (SOS)", "Тихая тревога (SOS)", "Тиха тривога (SOS)", "Cichy Alarm (SOS)", "Tichý Poplach (SOS)", "Tichý Poplach (SOS)" },
    /* TXT_ALARM_STATE_ARMED */         { "NAORUŽANO", "ARMED", "SCHARF", "ARMÉ", "INSERITO", "ARMADO", "ПОД ОХРАНОЙ", "ПІД ОХОРОНОЮ", "UZBROJONY", "ZAPNUTO", "ZAPNUTÉ" },
    /* TXT_ALARM_STATE_DISARMED */      { "RAZORUŽANO", "DISARMED", "UNSCHARF", "DÉSARMÉ", "DISINSERITO", "DESARMADO", "СНЯТО С ОХРАНЫ", "ЗНЯТО З ОХОРОНИ", "ROZBROJONY", "VYPNUTO", "VYPNUTÉ" },
    /* TXT_ALARM_STATE_ARMING */        { "Naoružavanje...", "Arming...", "Schärfung...", "Armement...", "Inserimento...", "Armando...", "Вооружение...", "Озброєння...", "Uzbrajanie...", "Aktivace...", "Vklapljanje..." },
    /* TXT_ALARM_STATE_DISARMING */     { "Razoružavanje...", "Disarming...", "Entschärfung...", "Désarmement...", "Disinserimento...", "Desarmando...", "Снятие с охраны...", "Зняття з охорони...", "Rozbrajanie...", "Deaktivace...", "Izklapljanje..." },
    /* TXT_ALARM_SYSTEM */              { "SISTEM", "SYSTEM", "SYSTEM", "SYSTÈME", "SISTEMA", "SISTEMA", "СИСТЕМА", "СИСТЕМА", "SYSTEM", "SYSTÉM", "SISTEM" },
    /* TXT_ALARM_PARTITION */           { "PARTICIJA", "PARTITION", "BEREICH", "PARTITION", "PARTIZIONE", "PARTICIÓN", "РАЗДЕЛ", "РОЗДІЛ", "PARTYCJA", "SEKCE", "PARTICIJA" },
    /* TXT_ALARM_CMD_ARM */             { "NAORUŽAJ", "ARM", "SCHARF", "ARMER", "INSERIRE", "ARMAR", "ПОСТАВИТЬ", "ПОСТАВИТИ", "UZBRÓJ", "ZAPNOUT", "ZAPNÚŤ" },
    /* TXT_ALARM_CMD_DISARM */          { "RAZORUŽAJ", "DISARM", "UNSCHARF", "DÉSARMER", "DISINSERIRE", "DESARMAR", "СНЯТЬ", "ЗНЯТИ", "ROZBRÓJ", "VYPNOUT", "VYPNÚŤ" },
    /* TXT_ALARM_ENTER_PIN */           { "UNESITE PIN", "ENTER PIN", "PIN EINGEBEN", "ENTREZ PIN", "INSERIRE PIN", "INTRODUZCA PIN", "ВВЕДИТЕ PIN", "ВВЕДІТЬ PIN", "WPROWADŹ PIN", "ZADEJTE PIN", "ZADAJTE PIN" },
    /* TXT_PIN_ENTER_CURRENT */         { "TRENUTNI PIN", "CURRENT PIN", "AKTUELLE PIN", "PIN ACTUEL", "PIN ATTUALE", "PIN ACTUAL", "ТЕКУЩИЙ PIN", "ПОТОЧНИЙ PIN", "OBECNY PIN", "AKTUÁLNÍ PIN", "AKTUÁLNY PIN" },
    /* TXT_PIN_ENTER_NEW */             { "UNESITE NOVI PIN", "ENTER NEW PIN", "NEUE PIN EINGEBEN", "ENTREZ NOUVEAU PIN", "INSERIRE NUOVO PIN", "INTRODUZCA NUEVO PIN", "ВВЕДИТЕ НОВЫЙ PIN", "ВВЕДІТЬ НОВИЙ PIN", "WPROWADŹ NOWY PIN", "ZADEJTE NOVÝ PIN", "ZADAJTE NOVÝ PIN" },
    /* TXT_PIN_CONFIRM_NEW */           { "POTVRDITE NOVI PIN", "CONFIRM NEW PIN", "NEUE PIN BESTÄTIGEN", "CONFIRMEZ NOUVEAU PIN", "CONFERMA NUOVO PIN", "CONFIRME NUEVO PIN", "ПОДТВЕРДИТЕ НОВЫЙ PIN", "ПІДТВЕРДІТЬ НОВИЙ PIN", "POTWIERDŹ NOWY PIN", "POTVRĎTE NOVÝ PIN", "POTVRĎTE NOVÝ PIN" },
    /* TXT_PIN_WRONG */                 { "POGREŠAN PIN", "WRONG PIN", "FALSCHE PIN", "MAUVAIS PIN", "PIN ERRATO", "PIN INCORRECTO", "НЕВЕРНЫЙ PIN", "НЕПРАВИЛЬНИЙ PIN", "BŁĘDNY PIN", "ŠPATNÝ PIN", "NESPRÁVNY PIN" },
    /* TXT_PINS_DONT_MATCH */           { "PIN-ovi se ne podudaraju", "PINs do not match", "PINS stimmen nicht überein", "Les PINs ne correspondent pas", "I PIN non corrispondono", "Los PIN no coinciden", "PIN-коды не совпадают", "PIN-коди не збігаються", "PINy nie pasują", "PINy se neshodují", "PINy sa nezhodujú" },
    /* TXT_PIN_CHANGE_SUCCESS */        { "PIN uspješno promijenjen", "PIN changed successfully", "PIN erfolgreich geändert", "PIN changé avec succès", "PIN modificato con successo", "PIN cambiado con éxito", "PIN успешно изменен", "PIN успішно змінено", "PIN zmieniony pomyślnie", "PIN úspěšně změněn", "PIN úspešne zmenený" },
    /* TXT_ALARM_CHANGE_PIN */          { "Promijeni Glavni PIN", "Change Main PIN", "Haupt-PIN ändern", "Changer le PIN principal", "Cambia PIN principale", "Cambiar PIN principal", "Изменить главный PIN", "Змінити головний PIN", "Zmień główny PIN", "Změnit hlavní PIN", "Zmeniť hlavný PIN" },
    /* TXT_ALARM_SYSTEM_NAME */         { "Naziv Sistema", "System Name", "Systemname", "Nom du système", "Nome del sistema", "Nombre del sistema", "Имя системы", "Назва системи", "Nazwa systemu", "Název systému", "Názov systému" },
    /* TXT_ALARM_PARTITION_NAME */      { "Naziv Particije", "Partition Name", "Partitionsname", "Nom de la partition", "Nome partizione", "Nombre de partición", "Имя раздела", "Назва розділу", "Nazwa partycji", "Název sekce", "Názov sekcie" },
    /* TXT_ALARM_NOT_CONFIGURED */      { "Alarmni sistem nije konfigurisan.", "Alarm system not configured.", "Alarmsystem nicht konfiguriert.", "Système d'alarme non configuré.", "Sistema di allarme non configurato.", "Sistema de alarma no configurado.", "Система сигнализации не настроена.", "Система сигналізації не налаштована.", "System alarmowy nie jest skonfigurowany.", "Alarmový systém není nakonfigurován.", "Alarmový systém nie je nakonfigurovaný." },
    /* TXT_OK */                        { "OK", "OK", "OK", "OK", "OK", "OK", "ОК", "ОК", "OK", "OK", "OK" },
    /* TXT_DEL */                       { "DEL", "DEL", "LÖSCH", "SUP", "CANC", "DEL", "УДАЛ", "ВИД", "DEL", "DEL", "DEL" },
    /* TXT_OFF_SHORT */                 { "ISKLJ.", "OFF", "AUS", "ARRÊT", "OFF", "OFF", "ВЫКЛ", "ВИМК", "WYŁ", "VYP", "VYP" },
    /* TXT_ERROR */                     { "GREŠKA", "ERROR", "FEHLER", "ERREUR", "ERRORE", "ERROR", "ОШИБКА", "ПОМИЛКА", "BŁĄD", "CHYBA", "CHYBA" },
};

static const char* _acContent[LANGUAGE_COUNT][7] = {
    { "PON", "UTO", "SRI", "ČET", "PET", "SUB", "NED" },
    { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" },
    { "MO", "DI", "MI", "DO", "FR", "SA", "SO" },
    { "LUN", "MAR", "MER", "JEU", "VEN", "SAM", "DIM" },
    { "LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM" },
    { "LUN", "MAR", "MIÉ", "JUE", "VIE", "SÁB", "DOM" },
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС" },
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "НД" },
    { "PON", "WT", "ŚR", "CZW", "PT", "SOB", "ND" },
    { "PO", "ÚT", "ST", "ČT", "PÁ", "SO", "NE" },
    { "PO", "UT", "ST", "ŠT", "PI", "SO", "NE" }
};

#endif // __TRANSLATIONS_H__
