/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


#if (__RS485_H__ != FW_BUILD)
#error "rs485 header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h" // Uvijek prvi
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "curtain.h"
#include "lights.h"
#include "display.h"
#include "stm32746g_eeprom.h" // Mapa ide nakon svih definicija
#include "firmware_update_agent.h"
#include "rs485.h"

/* Imported Types  -----------------------------------------------------------*/
/* Imported Variables --------------------------------------------------------*/
/* Imported Functions    -----------------------------------------------------*/
/* Private Typedef -----------------------------------------------------------*/
static TinyFrame tfapp;
/* Private Define  -----------------------------------------------------------*/
#define BIN_ACK_POZICIJA		3   // gdje ocekujem ACK bajt u baferu odgovora binarnog upita
#define DIM_ACK_POZICIJA		3   // pozicija ACK bajta u odgovoru na komande dimeru
#define JAL_ACK_POZICIJA		3   // pozicija ACK bajta u odgovoru na komande žaluzinama
#define THE_ACK_POZICIJA		18  // pozicija ACK bajta u odgovoru na komande termostatu
#define RGB_ACK_POZICIJA		5   // pozicija ACK bajta u odgovoru na komande rgbw
#define MAX_RETRIES 3           // Maksimalan broj pokušaja
#define TIMEOUT_MS 10           // Timeout u ms za cekanje odgovora
#define TH_INFO_DELAY 100       // Kašnjenje termostat info poruke maste->slave nakon što master dobije set paket
#define RESPONSE_TIME   200  // ms
#define MAX_GET_RETRY   3
/* Private Variables  --------------------------------------------------------*/
TF_Msg sendData;
bool init_tf = false;               // true = tf inicijalizovan, sprjecava blokadu kada sys timer krene a tf još nije inicijalizovan
volatile bool fw_flag = false;      // true = u toku je transfer firmwera, false = nije
volatile bool ack_flag = true;     // true = ACK ogovora primljen, false = nije,  Signalizira da je odgovor stigao
volatile bool isSending = false;    // Flag koji oznacava da li je trenutno aktivno slanje
volatile bool th_save = false;      // treba spasiti postavke termostata
volatile uint8_t qr_save;           // new qr code ready for eeprom
volatile uint32_t th_info_delay;    // delay za slanje termostat info ako je master dobio set paket
uint32_t rstmr = 0;
uint32_t wradd = 0;
uint32_t bcnt = 0;;
uint8_t rec, tfifa;
uint8_t eebuf[64]; // bufer za upis u eeprom
static uint8_t ack_pozicija;

// Globalne promenljive
CommandQueue binaryQueue = {0};
CommandQueue dimmerQueue = {0};
CommandQueue rgbwQueue = {0};
CommandQueue curtainQueue = {0};
CommandQueue thermoQueue = {0};
static GetResponseBuffer getResponseBuffer;
/* Private macros   ----------------------------------------------------------*/
/* Private Function Prototypes -----------------------------------------------*/
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief  staticka inline funkcija pauze 1~2ms za kašnjenje odgovora za stabilne repeatere
  * @param
  * @retval
  */
static inline void delay_us(uint32_t us) {
    volatile uint32_t cycles = (HAL_RCC_GetHCLKFreq() / 20000000) * us;  // Smanjen faktor
    while (cycles--) {
        __asm volatile("NOP");  // Izbegava optimizaciju
    }
}
/**
 * @brief  Slušaoci (listener) za dolazne BINARY_SET komande sa RS485 bus-a.
 * @note   Kada primi poruku o promjeni stanja binarnog svjetla, ova funkcija
 * izvlaci adresu i novo stanje, te poziva javnu API funkciju
 * `LIGHTS_UpdateExternalState` da obavijesti `lights` modul.
 */
TF_Result BINARY_SET_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Provjera da li je odgovor validan (sadrži ACK)
    if (msg->data[BIN_ACK_POZICIJA] == ACK)
    {
        // =======================================================================
        // === POCETAK REFAKTORISANJA ===
        
        // KORAK 1: Izvlacimo adresu i novo stanje iz primljene poruke.
        uint16_t adr = (uint16_t)(msg->data[0] << 8) | msg->data[1];
        uint8_t state = (msg->data[2] == BINARY_ON) ? 1 : 0;

        // KORAK 2: Brišemo kompletnu `for` petlju.
        // Umjesto da `rs485` modul pretražuje svjetla, on samo poziva
        // jednu javnu funkciju i prosljeduje joj adresu i stanje.
        LIGHTS_UpdateExternalState(adr, state);
        
        // === KRAJ REFAKTORISANJA ===
        // =======================================================================
    }
    return TF_STAY; // Ne odgovaramo na ovu poruku, samo je slušamo.
}
/**
 * @brief  Slušaoci (listener) za dolazne DIMMER_SET komande sa RS485 bus-a.
 * @note   Kada primi poruku o promjeni svjetline dimera, ova funkcija
 * izvlaci adresu i novu vrijednost, te poziva javnu API funkciju
 * `LIGHTS_UpdateExternalBrightness`.
 */
TF_Result DIMMER_SET_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Provjera da li je odgovor validan (sadrži ACK)
    if (msg->data[DIM_ACK_POZICIJA] == ACK)
    {
        // =======================================================================
        // === POCETAK REFAKTORISANJA ===
        
        // KORAK 1: Izvlacimo adresu i novu vrijednost svjetline iz poruke.
        uint16_t adr = (uint16_t)(msg->data[0] << 8) | msg->data[1];
        uint8_t brightness = msg->data[2];

        // KORAK 2: Validacija vrijednosti (0-100)
        if (brightness <= 100)
        {
            // KORAK 3: Brišemo `for` petlju i pozivamo jednu javnu funkciju.
            // `rs485` modul više ne brine o tome koje je svjetlo na kojoj adresi.
            LIGHTS_UpdateExternalBrightness(adr, brightness);
        }
        
        // === KRAJ REFAKTORISANJA ===
        // =======================================================================
    }
    return TF_STAY; // Ne odgovaramo na ovu poruku.
}
/**
* @brief :  Promjene i update žaluzina je malo vjerovatno da ce donijet neku bitnu korist
            softveru ali je jako korisno kada se stoka igra sa displejima tako da svaka
            naredna komnda sa bilo kojeg displeja bude u skladu sa stvarnim stanjem
* @param :
* @retval:  TF_STAY / ne odgovaraj ne ovu poruku, ovo je iskljucivo za aktuatore
*/
TF_Result JALOUSIE_SET_Listener(TinyFrame *tf, TF_Msg *msg)
{
    uint16_t adr = (uint16_t)(msg->data[0]<<8) | msg->data[1];  // sastavi adresu iz upita
    uint8_t dir = msg->data[2]; // smejr šaluzine
    // Pozivamo novu Curtain_Update_External funkciju koja ce sama pronaci
    // odgovarajucu roletnu i ažurirati njeno stanje.
    // Više ne moramo da prolazimo kroz petlju ovdje.
    Curtain_Update_External(adr, dir);
    
    return TF_STAY;
}
/**
* @brief :  Promjene i update rgb vrijednosti sa uredaja unutar rs485 busa
* @param :
* @retval:  TF_STAY / ne odgovaraj ne ovu poruku, ovo je iskljucivo za aktuatore
*/
TF_Result RGB_SET_Listener(TinyFrame *tf, TF_Msg *msg)
{

    return TF_STAY;
}
/**
* @brief :  Promjene i update rgb vrijednosti sa wifi/json interfejsa
*           docemo i do toga ako budemo koristili
* @param :
* @retval:  TF_STAY
*/
TF_Result RGB_INFO_Listener(TinyFrame *tf, TF_Msg *msg)
{

    return TF_STAY;
}
/**
* @brief :  Ovo je tip samo za explicitan zahtjev sa http-a inace svi uredaji
*           u busu imaju sve zadnje promjene stanja preko info poruka
*           ako ovaj kontroler ima registrovan master termostat
*           reagovace na ovaj zahtjev prema svom id termostata
*           tako što ce poslati cijelu strukturu termostata,
*           struktura treba biti kreirana pazeci na padding
*           ili koristeci __attribute__((packed)) ili jedan po jedan
*           element kopirana u bafer, najbolje koristiti padding
*           tako da se zna tacan rapored elemenata u strukturi
*           koristi read->modify->write na klijentu, nakon read fail
*           odma vrati termostat nedostupan za http zahtjev
*
* @param :
* @retval:  TF_STAY / vrati cijelu strukturu termostata koja je u cijelom projektu ista
*/
TF_Result THERMOSTAT_GET_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Dobijamo handle za termostat.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    uint8_t resp[15] = {0};

    // Koristimo API funkcije za provjeru uslova.
    if(Thermostat_IsMaster(pThst) && Thermostat_GetGroup(pThst) == msg->data[0])
    {
        // === KOMPLETNO POPUNJAVANJE ODGOVORA KORIŠTENJEM GETTERA ===
        // Svaki direktan pristup je zamijenjen pozivom odgovarajuce API funkcije.
        resp[0] = Thermostat_GetGroup(pThst);
        resp[1] = Thermostat_IsMaster(pThst);
        resp[2] = Thermostat_GetControlMode(pThst);
        resp[3] = Thermostat_GetState(pThst);
        resp[4] = (Thermostat_GetMeasuredTemp(pThst) >> 8) & 0xFF;
        resp[5] = Thermostat_GetMeasuredTemp(pThst) & 0xFF;
        resp[6] = Thermostat_GetSetpoint(pThst);
        resp[7] = Thermostat_Get_SP_Min(pThst);
        resp[8] = Thermostat_Get_SP_Max(pThst);
        resp[9] = Thermostat_GetSetpointDifference(pThst);
        resp[10] = Thermostat_GetFanSpeed(pThst);
        resp[11] = Thermostat_GetFanLowBand(pThst);
        resp[12] = Thermostat_GetFanHighBand(pThst);
        resp[13] = Thermostat_GetFanDifference(pThst);
        resp[14] = Thermostat_GetFanControlMode(pThst);
        
        // Ostatak logike ostaje isti.
        msg->len = 15;
        msg->data = resp;
        TF_Respond(tf, msg);
    }
    return TF_STAY;
}
/**
* @brief :  Ako ovaj kontroler ima registrovan master termostat sa istim
*           id kao iz poruke promjenice svoju vrijednost, master termostat
*           ce odgovoriti na ovaj paket i još dodatno poslati info paket
*           nakon backoff vremena, a takoder i promjenu aktuatora prema
*           procesiranom novom stanju
* @param :
* @retval:  TF_STAY
*/
TF_Result THERMOSTAT_SET_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Dobijamo handle za termostat
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    uint8_t resp[3] = {0};

    // Koristimo API funkcije umjesto direktnog pristupa
    if(Thermostat_IsMaster(pThst) && (Thermostat_GetGroup(pThst) != 0) && (Thermostat_GetGroup(pThst) == msg->data[0]))
    {
        Thermostat_SP_Temp_Set(pThst, msg->data[1]);
        
        resp[0] = msg->data[0];
        resp[1] = msg->data[1];
        resp[2] = ACK;
        msg->data = resp;
        msg->len = 3;
        TF_Respond(tf, msg);

        th_info_delay = HAL_GetTick() + TH_INFO_DELAY;
    }
    return TF_STAY;
}
/**
* @brief :  Ako ovaj kontroler ima termostat slušace poruke ovog tipa
*           svi termostati sa istim id iz poruke se sinhronizuju prema
*           ovim paketima, mater termostat dodatno još odgovara na slave
*           zahtjeve, ako je primljen info sa praznom strukturom to je
*           zahtjev masteru da sinhronizuje uredaj koji se tek ukljucio
* @param :
* @retval:  TF_STAY / samo master odgovara na poruku
*/
TF_Result THERMOSTAT_INFO_Listener(TinyFrame *tf, TF_Msg *msg)
{
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    bool all_zero = true;
    uint8_t resp[16] = {0};

    // Provjera da li je poruka zahtjev za sinhronizaciju (svi bajtovi nula osim prvog)
    for (int i = 1; i < msg->len; i++) {
        if (msg->data[i] != 0) {
            all_zero = false;
            break;
        }
    }

    if(Thermostat_GetGroup(pThst) == msg->data[0])
    {
        if(!all_zero) {
            Thermostat_SetControlMode(pThst, msg->data[2]);
            
            // Slave termostat prihvata izmjerenu temperaturu od mastera
            if(!Thermostat_IsMaster(pThst)) {
                int16_t new_mv_temp = (int16_t)(msg->data[4] << 8) | msg->data[5];
                Thermostat_SetMeasuredTemp(pThst, new_mv_temp);
            }
        }

        // Ako je ovaj uredaj master, on odgovara na info poruke
        if(Thermostat_IsMaster(pThst)) {
            // Provjera da li se drugi master pojavio na mreži
            if(msg->data[1]) { 
                Thermostat_SetMaster(pThst, false);
                th_save = true; // Flag za snimanje promjene uloge
            }

            // --- KOMPLETNO POPUNJAVANJE ODGOVORA KORIŠTENJEM GETTERA ---
            resp[0] = Thermostat_GetGroup(pThst);
            resp[1] = Thermostat_IsMaster(pThst);
            resp[2] = Thermostat_GetControlMode(pThst);
            resp[3] = Thermostat_GetState(pThst); // Koristi novi getter za stanje
            resp[4] = (Thermostat_GetMeasuredTemp(pThst) >> 8) & 0xFF;
            resp[5] = Thermostat_GetMeasuredTemp(pThst) & 0xFF;
            resp[6] = Thermostat_GetSetpoint(pThst);
            resp[7] = Thermostat_Get_SP_Min(pThst);
            resp[8] = Thermostat_Get_SP_Max(pThst);
            resp[9] = Thermostat_GetSetpointDifference(pThst);
            resp[10] = Thermostat_GetFanSpeed(pThst);
            resp[11] = Thermostat_GetFanLowBand(pThst);
            resp[12] = Thermostat_GetFanHighBand(pThst);
            resp[13] = Thermostat_GetFanDifference(pThst);
            resp[14] = Thermostat_GetFanControlMode(pThst);
            resp[15] = ACK;
            
            msg->data = resp;
            msg->len = 16;
            TF_Respond(tf, msg);
        }
        if(screen == SCREEN_THERMOSTAT) DISP_SetThermostatMenuState(0);
    }
    return TF_STAY;
}
/**
* @brief :  Ako ovaj kontroler ima registrovan master termostat sa istim
*           id kao iz poruke promjenice svoju strukturu, master termostat
*           ce odgovoriti na ovaj paket i još dodatno poslati info paket
*           nakon backoff vremena, a takoder i promjenu aktuatora prema
*           procesiranom novom stanju, nakon promjene reinicijalizovati
*           termostat
* @param :
* @retval:  TF_STAY
*/
TF_Result THERMOSTAT_SETUP_Listener(TinyFrame *tf, TF_Msg *msg)
{
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    uint8_t resp[2] = {0, NAK};

    if(Thermostat_IsMaster(pThst) && Thermostat_GetGroup(pThst) == msg->data[0])
    {
        // --- KOMPLETNO POSTAVLJANJE VRIJEDNOSTI KORIŠTENJEM SETTERA ---
        Thermostat_SetGroup(pThst, msg->data[0]);
        Thermostat_SetMaster(pThst, msg->data[1]);
        Thermostat_SetControlMode(pThst, msg->data[2]);
        
        // Objašnjenje za komentare:
        // th_state (stanje releja) i mv_temp (izmjerena temp.) se ne postavljaju preko ove komande.
        // - th_state je REZULTAT rada termostata, a ne postavka.
        // - mv_temp dolazi sa fizickog senzora preko ADC-a.
        // Zato preskacemo msg->data[3] i msg->data[4].
        
        Thermostat_SP_Temp_Set(pThst, msg->data[5]);
        Thermostat_Set_SP_Min(pThst, msg->data[6]);
        Thermostat_Set_SP_Max(pThst, msg->data[7]);
        Thermostat_SetSetpointDifference(pThst, msg->data[8]);
        
        // fan_speed je takoder rezultat rada, ne postavka, preskacemo msg->data[9].

        Thermostat_SetFanLowBand(pThst, msg->data[10]);
        Thermostat_SetFanHighBand(pThst, msg->data[11]);
        Thermostat_SetFanDifference(pThst, msg->data[12]);
        Thermostat_SetFanControlMode(pThst, msg->data[13]);
        
        THSTAT_Save(pThst); // Sacuvaj sve nove promjene odjednom.
        
        resp[0] = msg->data[0];
        resp[1] = ACK;
        msg->data = resp;
        msg->len = 2;
        TF_Respond(tf, msg);

        th_info_delay = HAL_GetTick() + TH_INFO_DELAY;
    }
    return TF_STAY;
}
/**
 * @brief Listener posvecen iskljucivo porukama za ažuriranje firmvera.
 *
 * @note  Ova funkcija služi kao "dispecer". Ona ne sadrži nikakvu logiku, vec
 * samo prosljeduje primljenu poruku modulu "Firmware Update Agent" koji
 * sadrži kompletnu logiku mašine stanja. Na ovaj nacin, `rs485.c` ostaje
 * cist i zadužen samo za transport, dok je sva kompleksnost ažuriranja
 * enkapsulirana u svom modulu.
 * NOVO: Ova funkcija sada ažurira i globalni tajmer `g_last_fw_packet_timestamp`
 * pri svakom primljenom paketu ovog tipa. Ovo omogucava svim uredajima
 * na busu da budu svjesni aktivnosti ažuriranja i da se privremeno blokiraju.
 *
 * @param tf    Pokazivac na TinyFrame instancu.
 * @param msg   Pokazivac na primljenu TF_Msg poruku.
 * @retval      TF_Result Uvijek vraca TF_STAY da listener ostane aktivan.
 */
TF_Result FIRMWARE_UPDATE_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Ažuriraj globalni tajmer da signalizira aktivnost na busu.
    // Ovo se dešava za SVAKI primljeni paket tipa FIRMWARE_UPDATE.
    g_last_fw_packet_timestamp = HAL_GetTick();

    // Proslijedi poruku Agentu na dalju obradu.
    FwUpdateAgent_ProcessMessage(tf, msg);
    
    return TF_STAY;
}
/**
* @brief :  Promjena QR koda za wifi i app, kako smo u interruptu usarta
*           ovdje se ne može koristit hal delay koji je sastavni dio hal
*           drivera pa cemo koristit flag za upis primljenog qr koda u eeprom
*           display funkcija ce qr kod ucitavati iz eeproma u initu displeja
*           svaki je qr kod dobrodošao a samo na explicitno adresiran inetrfejs
*           adresu odgovaramo potvrdno
*
* @param :
* @retval:  TF_STAY
*/
TF_Result QR_REQUEST_Listener(TinyFrame *tf, TF_Msg *msg)
{
#ifndef QR_CODE_COUNT
#define QR_CODE_COUNT 2 // ovdje nije dostupno dok se ne premjesti u header zato i postoji common.h
#endif

    uint8_t resp[2] = {QR_CODE_SET, QR_CODE_QUERY_RESPONSE_DATA_TOO_LONG}; // iako je život sam pozitivan, odgovor je defaultno negativan
    bool len_valid = QR_Code_isDataLengthShortEnough(msg->len - 3); // provjeri limit velicine
    bool data_valid = ((msg->data[2] > 0) && (msg->data[2] <= QR_CODE_COUNT)); // provjeri validan index

    if(len_valid && data_valid)
    {
        QR_Code_Set(msg->data[2], msg->data + 3);
        if(screen == SCREEN_QR_CODE) shouldDrawScreen = true; // obavijesti screen
        eebuf[0] = msg->len - 3; // spasi dužinu qr koda u prvom bajtu
        memcpy(&eebuf[1], msg->data + 3, msg->len - 3); // kopiraj qr kod u temp buffer
        qr_save = msg->data[2]; // ovdje nije moguc siguran upis zato setuj flag/id za upis u eeprom
    }
    // ako je direktno adresirano na tinyframe interface address odgovori
    if(msg->data[1] == tfifa)
    {
        if (len_valid && data_valid) resp[1] = ACK; // vrati i potvrdan odgovor
        msg->data = resp; // prikaci paket
        msg->len = 2; // vrati dva bajta
        TF_Respond(tf, msg);
    }
    return TF_STAY;
}
/**
* @brief :  Koliko je sati ?
* @param :
* @retval:  TF_STAY
*/
TF_Result TIME_INFO_Listener(TinyFrame *tf, TF_Msg *msg)
{
    rtcdt.WeekDay = msg->data[0];
    rtcdt.Date    = msg->data[1];
    rtcdt.Month   = msg->data[2];
    rtcdt.Year    = msg->data[3];
    rtctm.Hours   = msg->data[4];
    rtctm.Minutes = msg->data[5];
    rtctm.Seconds = msg->data[6];
    HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
    RtcTimeValidSet();
    return TF_STAY;
}
/**
* @brief :  ovo je ID listener registrovan za sve SET upite, takav nacin mogucava više
*           razlicitih simultanih upita sa po jedan FIFO bufer komandi sa push / pop
*           mehanizmom, idealno treba dograditi provjeru poruke iz upita sa odgovorom
*           tako da je sigurno odgovor na taj upit
* @param :
* @retval:  samouništenje bez odgovora za svaki pojedinacni tip komande za koji je registrovan u pozivima
*/
TF_Result SET_RESPONSE_Listener(TinyFrame *tf, TF_Msg *msg)
{
    // Provjera da li je ACK bajt na pravoj poziciji, najbrži metod
    if (msg->len > ack_pozicija && msg->data[ack_pozicija] == ACK)
    {
        ack_flag = true; // Samo postavljamo flag, ne diramo queue
    }
    return TF_CLOSE;
}
/**
* @brief :  ovo je ID listener registrovan za sve GET upite
* @param :
* @retval:  samouništenje bez odgovora za svaki pojedinacni tip komande za koji je registrovan u pozivima
*/
TF_Result GET_RESPONSE_Listener(TinyFrame *tf, TF_Msg *msg) {
    if (msg->len > sizeof(getResponseBuffer.data)) return TF_CLOSE; // Osigurajmo da ne prepunimo bafer

    getResponseBuffer.commandType = msg->type;
    getResponseBuffer.length = msg->len;
    memcpy(getResponseBuffer.data, msg->data, msg->len);
    getResponseBuffer.ready = true;
    return TF_CLOSE;
}
/**
* @brief :  Ubaci sljedecu komandu u red komandi na cekanju
* @param :
* @retval:  ne vraca ništa samo izade ako je dug red treba proširit memoriju
*/
bool AddCommand(CommandQueue *queue, uint8_t commandType, uint8_t *data, uint8_t length)
{
    if (queue->count >= COMMAND_QUEUE_SIZE) {
        // Ako je queue pun, odluciti da:
        // 1. Prebaciš u "overflow buffer" i kasnije obradiš
        // 2. Prepišeš najstariju komandu (ring buffer stil)
        // 3. Samo odbaci novu komandu
        return false; // vrati pozivaocu status
    }

    // Upisujemo novu komandu u red
    queue->commands[queue->tail].commandType = commandType;
    memcpy(queue->commands[queue->tail].data, data, length);
    queue->commands[queue->tail].length = length;
    // Kružno pomjeranje repa
    queue->tail = (queue->tail + 1) % COMMAND_QUEUE_SIZE;
    queue->count++;
    return true; // komanda uspješno dodana u red komandi
}
/**
* @brief :  šalji slijedecu komandu iz reda na cekanju sa resend mehanizmom i cekanjem odgovora
* @param :
* @retval:  ne vraca ništa samo izadje ako je prazan red
*/
void SendCommand(CommandQueue *queue)
{
    if (queue->count == 0) return; // Ako nema komandi, izlazimo

    Command *cmd = &queue->commands[queue->head];
    // gdje ocekujem ACK bajt u baferu odgovora
    if      (cmd->commandType == BINARY_SET)        ack_pozicija = BIN_ACK_POZICIJA;
    else if (cmd->commandType == DIMMER_SET)        ack_pozicija = DIM_ACK_POZICIJA;
    else if (cmd->commandType == JALOUSIE_SET)      ack_pozicija = JAL_ACK_POZICIJA;
    else if (cmd->commandType == THERMOSTAT_INFO)   ack_pozicija = THE_ACK_POZICIJA;
    else if (cmd->commandType == RGB_SET)           ack_pozicija = RGB_ACK_POZICIJA;

    // Pokušaj ponovnog slanja komande
    for (int pokusaj = 0; pokusaj < MAX_RETRIES; pokusaj++) {

        ack_flag = false; // Resetujemo flag prije slanja komande

        // Šaljemo komandu
        TF_QuerySimple(&tfapp, cmd->commandType, cmd->data, cmd->length, SET_RESPONSE_Listener, TIMEOUT_MS);

        // Cekamo odgovor sa timeoutom
        int timeout = TIMEOUT_MS;
        while (timeout--) {
            if (ack_flag) break;  // Ako ACK stigne, prekini cekanje
            HAL_Delay(1); // ovo je nužno zlo
        }

        if (ack_flag) break; // Ako smo dobili odgovor, izlazimo iz petlje
    }

    // Ako je komanda završena (bilo zbog ACK-a ili timeouta), ukloni je iz reda
    queue->head = (queue->head + 1) % COMMAND_QUEUE_SIZE;
    queue->count--;

    // Resetujemo flag da ne utice na sledece komande
    ack_flag = false;
}
/**
* @brief :  traži stanje bilo cega na busu
* @param :
* @retval:  true = odgovor u baferu / false = odgovora nije bilo
*/
bool GetState(uint8_t commandType, uint16_t address, uint8_t *response)
{
    uint8_t buf[2];
    buf[0] = (address >> 8) & 0xFF;
    buf[1] = address & 0xFF;

    for (int attempt = 0; attempt < MAX_GET_RETRY; attempt++) {
        getResponseBuffer.ready = false;  // Resetujemo status odgovora
        getResponseBuffer.commandType = 0;

        TF_QuerySimple(&tfapp, commandType, buf, sizeof(buf), GET_RESPONSE_Listener, RESPONSE_TIME);

        int timeout = RESPONSE_TIME;
        while (timeout--) {
            if (getResponseBuffer.ready && getResponseBuffer.commandType == commandType) {
                memcpy(response, getResponseBuffer.data, getResponseBuffer.length);
                return true;  // Uspješno primljen odgovor
            }
            HAL_Delay(1);  // Odbrojavamo timeout
        }
    }

    return false;  // Ako nakon svih pokušaja nema odgovora, vracamo false
}
/**
* @brief :  init usart interface to rs485 9 bit receiving
* @param :  and init state to receive packet control block
* @retval:  wait to receive:
*           packet start address marker SOH or STX  2 byte  (1 x 9 bit)
*           packet receiver address 4 bytes msb + lsb       (2 x 9 bit)
*           packet sender address msb + lsb 4 bytes         (2 x 9 bit)
*           packet lenght msb + lsb 4 bytes                 (2 x 9 bit)
*/
void RS485_Init(void)
{
    sendData.data = NULL;
    sendData.len = 0;

    if(!init_tf) {
        init_tf = TF_InitStatic(&tfapp, TF_MASTER);

        TF_AddTypeListener(&tfapp, RGB_SET, RGB_SET_Listener);
        TF_AddTypeListener(&tfapp, RGB_INFO, RGB_INFO_Listener);
        TF_AddTypeListener(&tfapp, BINARY_SET, BINARY_SET_Listener);
        TF_AddTypeListener(&tfapp, DIMMER_SET, DIMMER_SET_Listener);
        TF_AddTypeListener(&tfapp, JALOUSIE_SET, JALOUSIE_SET_Listener);
        TF_AddTypeListener(&tfapp, QR_REQUEST, QR_REQUEST_Listener);
        TF_AddTypeListener(&tfapp, TIME_INFO, TIME_INFO_Listener);
        TF_AddTypeListener(&tfapp, THERMOSTAT_GET, THERMOSTAT_GET_Listener);
        TF_AddTypeListener(&tfapp, THERMOSTAT_SET, THERMOSTAT_SET_Listener);
        TF_AddTypeListener(&tfapp, THERMOSTAT_INFO, THERMOSTAT_INFO_Listener);
        TF_AddTypeListener(&tfapp, THERMOSTAT_SETUP, THERMOSTAT_SETUP_Listener);
        TF_AddTypeListener(&tfapp, FIRMWARE_UPDATE, FIRMWARE_UPDATE_Listener);
    }
    HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
* @brief  : Servisiramo bufere za slanje i flagove na cekanju
*           zasad je ili<->ili  fimware update <-> normalan rad
*           dok se ne razvije precizan timing vremenskih slotova
*           u toku transfera novog firmware-a
* @param  :
* @retval : nema
*/
void RS485_Service(void)
{
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    
    uint32_t now = HAL_GetTick();

    if (IsFwUpdateActiv())
    {
        if(now >= rstmr + 5000)
        {
            StopFwUpdate();
            wradd = 0;
            bcnt = 0;
        }
        return;
    }
    // šalji komande na redu
    SendCommand(&binaryQueue);
    SendCommand(&dimmerQueue);
    SendCommand(&rgbwQueue);
    SendCommand(&curtainQueue);
    SendCommand(&thermoQueue);
    // spasinovi qr kod ako je na cekanju
    if(qr_save)
    {
        EE_WriteBuffer(eebuf, (qr_save == 1 ? EE_QR_CODE1 : EE_QR_CODE2), eebuf[0]+1); // racunaj i prvi bajt
        qr_save = 0;
    }
    // spasi postavke termostata ako su na cekanju
    if(th_save)
    {

    }
    // pošalji termostatima info paket ako je na cekanju
    if(th_info_delay && (now - th_info_delay) >=0 )
    {
        th_info_delay = 0;
        Thermostat_SetInfoChanged(pThst, true);
    }

}
/**
  * @brief
  * @param
  * @retval
  */
void RS485_Tick(void)
{
    if (init_tf == true) {
        TF_Tick(&tfapp);
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
    delay_us(4000);
    HAL_UART_Transmit(&huart1,(uint8_t*)buff, len, RESP_TOUT);
    HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
  * @brief
  * @param
  * @retval
  */
void RS485_RxCpltCallback(void)
{
    TF_AcceptChar(&tfapp, rec);
    HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
* @brief : all data send from buffer ?
* @param : what  should one to say   ? well done,
* @retval: well done, and there will be more..
*/
void RS485_TxCpltCallback(void)
{
}
/**
* @brief : usart error occured during transfer
* @param : clear error flags and reinit usaart
* @retval: and wait for address mark from master
*/
void RS485_ErrorCallback(void)
{
    __HAL_UART_CLEAR_PEFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_FLUSH_DRREGISTER(&huart1);
    huart1.ErrorCode = HAL_UART_ERROR_NONE;
    HAL_UART_Receive_IT(&huart1, &rec, 1);
}

/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
