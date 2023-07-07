#ifndef LoggerBaseExtH_h
#define LoggerBaseExtH_h

// ===================================================================== //
// Public functions extensions for basic logging paramters
// ===================================================================== //

// void setLoggingInterval_atl485(uint16_t loggingIntervalMinutes);

void setLoggerId(const char* newLoggerId, bool copyId = false,
                 uint8_t LoggerIdSize = NEW_LOGGERID_MAX_SIZE);

/* Provides a method of getting battery voltage
 to determine if enough power to continue
*/
bat_handler_atl _bat_handler_atl = NULL;

void setBatHandler(bool (*bat_handler_atl)(lb_pwr_req_t reqBatState));

#if !defined SERIALIZE_POST_MAX_READINGS
#define SERIALIZE_POST_MAX_READINGS 20
#endif  // SERIALIZE_POST_MAX_READINGS
uint16_t _sendAtOneTimeMaxX_num = SERIALIZE_POST_MAX_READINGS;
void     setSendEveryX(
        uint8_t  sendEveryX_num,
        uint16_t sendAtOneTimeMaxX_num = SERIALIZE_POST_MAX_READINGS) {
    _sendEveryX_num = sendEveryX_num;
    // Check range, if too small set to max
    if (sendAtOneTimeMaxX_num > 3) {
        _sendAtOneTimeMaxX_num = sendAtOneTimeMaxX_num;
    } else {
        _sendAtOneTimeMaxX_num = -1;  // Max
    }
}
uint8_t getSendEveryX(void) {
    return _sendEveryX_num;
}
#if !defined SERIALIZE_sendEveryX_NUM
#define SERIALIZE_sendEveryX_NUM 2
#endif  // SERIALIZE_sendEveryX_NUM
// Range typically 0-20
uint8_t _sendEveryX_num = SERIALIZE_sendEveryX_NUM;
int8_t  _sendEveryX_cnt = 0;  // Counter to track status

void setSendOffset(uint8_t param1) {
    // Might also have check for SampleTime * _sendEveryX_num < param1
    if (_sendOffset_MAX >= param1) {
        _sendOffset_min = param1;
    } else {
        _sendOffset_min = _sendOffset_MAX;
    }
}
uint8_t getSendOffset(void) {
    return _sendOffset_min;
}
#if !defined SERIALIZE_sendOffset_min
#define SERIALIZE_sendOffset_min 0
#endif  // SERIALIZE_sendOffset_min
uint8_t       _sendOffset_min = SERIALIZE_sendOffset_min;
const uint8_t _sendOffset_MAX = 14;  // Max allowed
bool          _sendOffset_act = false;
uint8_t       _sendOffset_cnt = 0;

void setSendPacingDelay(uint8_t param) {
    _sendPacingDelay_mSec = param;
}
uint8_t getSendPacingDelay(void) {
    return _sendPacingDelay_mSec;
}
#define SERIALIZE_sendPacingDelay_mSec 2
uint16_t _sendPacingDelay_mSec = SERIALIZE_sendPacingDelay_mSec;

    uint16_t setSendQueSz_num(uint16_t sqz_num) {
        MS_DBG(F("setSendQueSz_num"), sqz_num);
        return _sendQueSz_num = sqz_num;
    }

    uint16_t getSendQueSz_num() {
        MS_DBG(F("getSendQueSz_num"), _sendQueSz_num);
        return _sendQueSz_num;
    }   
#if !defined LB_SENDQUESZ_NUM_DEF 
#define LB_SENDQUESZ_NUM_DEF  2800L //MMWGI_SEND_QUE_SZ_NUM_DEF
#endif  // LB_SENDQUESZ_NUM_DEF 
    uint16_t _sendQueSz_num = LB_SENDQUESZ_NUM_DEF ; //See MMMWGI_SEND_QUE_SZ_NUM_DEF 

    uint16_t setPostMax_num(uint16_t mp_num) {
        MS_DBG(F("setMaxPost_num"), mp_num);
        return _postMax_num = mp_num;
    }

    uint16_t getPostMax_num() {
        MS_DBG(F("getPostMax_num"), _postMax_num);
        return _postMax_num;
    }   
#if !defined LB_POSTMAX_NUM_DEF 
#define LB_POSTMAX_NUM_DEF 96L //MMWGI_POST_MAX_RECS_MUM_DEF
#endif  // LB_POSTMAX_NUM_DEF
    uint16_t _postMax_num = LB_POSTMAX_NUM_DEF; //See MMWGI_POST_MAX_RECS_MUM_DEF   

// Time woken up
uint32_t wakeUpTime_secs;

public:
void startFixedWatchdog() {
    //Fixed time for initialize.
    watchDogTimer.setupWatchDog(5*60);
    watchDogTimer.enableWatchDog();
};
void resetWatchdog() {watchDogTimer.resetWatchDog();}

// ===================================================================== //
// Public functions for a "reading .ini" file
// ===================================================================== //

public:
bool parseIniSd(const char* ini_filename, ini_handler_atl485 handler_fn);
bool parseIniFile(const char* ini_filename, ini_handler_atl485 unhandledFnReq);
bool parseAndRename(const char ini_ext, const char* ini_filename, ini_handler_atl485 unhandledFnReq);

#ifdef USE_MS_SD_INI
void setPs_cache(persistent_store_t* ps_ram);
void printFileHeaderExtra(Stream* stream);
#endif  // USE_MS_SD_INI
// virtual bool parseIniSd(const char *ini_fn,ini_handler handler_fn);
void        SDusbPoll(uint8_t sdActions);
#if defined USE_RTCLIB
#include "RTClib.h"
USE_RTCLIB* rtcExtPhyObj();
#endif  // USE_RTCLIB
static bool usbDriveActive(void);



private:
static int32_t sd1_card_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
static int32_t sd1_card_write_cb(uint32_t lba, uint8_t* buffer,
                                 uint32_t bufsize);
static void    sd1_card_flush_cb(void);
static int32_t sdq_flashspi_read_cb(uint32_t lba, void* buffer,
                                    uint32_t bufsize);
static int32_t sdq_flashspi_write_cb(uint32_t lba, uint8_t* buffer,
                                     uint32_t bufsize);
static void    sdq_flashspi_flush_cb(void);
static bool    sdq_ready(void);  // Called when USB present and checking of
                                 // readiness

persistent_store_t* ps_cache = NULL;

// void print_rootdir(File* rdir);
bool SDextendedInit(bool sdSuccess);
// int inihandlerFn( const char* section, const char* name, const char* value);
// Parse the supplied File for ModularSensors [Section] name=value pairs
// return !0 for error with line number
virtual int8_t inihParseFile(ini_handler_atl485 handler_fn);
// Tokens and UUID's for EnviroDIY
const char* _registrationToken;
const char* _samplingFeature;
const char* _LoggerId_buf = NULL;

public:
void forceSysReset(uint8_t source, uint16_t simpleCheck);
/**
 * @brief Process queued readings to send to remote if internet available.
 *
 * If previously registered, it will determine if battery power is available
 * It uses an algorithim to reliably deliver the readings.
 *
 */
#define LOGGER_RELIABLE_POST (Logger::CIA_NEW_READING|Logger::CIA_POST_READINGS)
#define LOGGER_NEW_READING (Logger::CIA_NEW_READING)
void logDataAndPubReliably(uint8_t cia_val_override =0);

/**
 * @brief Process queued readings to send to remote if internet available.
 *
 * @param internetPresent  true if an internet connection is present.
 *   For false store the readings for later transmission
 *   This reads from stored files
 *   1) RDELAY.txt
 *   2) QUE0.txt
 *   For data in RDELAY.txt, an attempt is made to transmit each line,
 *   if not sucessful then it is stored at the end of QUE0.txt
 *   Once RDELAY.txt is complete, 
 *   any readings in QUE0.txt are attempted,
 * 
 *   The format of he readings in the file are dependent on sensor configuration, 
 *   so they maynot be compatible if the builds sensor configuration changes.
 * 
 *   The forwarding of RDELAY.txt to QUE0.txt is only up to specific thresholds
 *   POST_MAX_NUM
 *   RDELAY_FAILED_POSTS_THRESHOLD 
 */
#define RDELAY_FAILED_POSTS_THRESHOLD 7
void publishDataQuedToRemotes(bool internetPresent);

/**
 * @brief Set up for sleep.
 *
 * For external clock chip need to ensure the data is set correctly
 *
 */
void setExtRtcSleep(); 

private:
/**
 * @brief Check HTTP status response
 *
 * @param rspCode  true if server is deemed to have received message
 * or nothing further can be done with message. 
 */
bool publishRspCodeAccepted(int16_t  rspCode);

// ===================================================================== //
/* Serializing/Deserialing
  A common set of functions that operate on files
  serzRdelFn_str
  serzQuedFn
*/
// ===================================================================== //
public:
bool     deszqNextCh(void);
long     deszq_epochTime = 0;  // Marked Epoch Time
char*    deszq_nextChar;
uint16_t deszq_nextCharSz;

// Calculated length of timeVariant data fields as ASCII+ delimiter, except
// for last data field
uint16_t deszq_timeVariant_sz;

private:
#define sd1_Err(s) sd1_card_fatfs.errorPrint(F(s))
uint16_t deszq_status    = 0;  // Bit wise status of reading
uint16_t deszLinesRead   = 0;
uint16_t deszLinesUnsent = 0;
#define QUEFILE_MAX_LINE 100
char     deszq_line[QUEFILE_MAX_LINE] = "";
uint16_t desz_pending_records         = 0;

// Qu SdFat/sd1_card_fatfs connects to Physical pins or File/logFile or
// keep to LFN - capitals  https://en.wikipedia.org/wiki/8.3_filename

#if defined MS_LOGGERBASE_POSTS
File        postsLogHndl;            // Record all POSTS when enabled
const char* postsLogFn_str = "DBG";  // Not more than 8.3 total

#endif  // MS_LOGGERBASE_POSTS

// que Readings DELAYed (RDEL) ~ serialize/deserialize
File        serzRdelFile;
const char* serzRdelFn_str = "RDELAY.TXT";

// QUEueD for reliable delivery
// first POST didn't suceed to serialize/deserialize
// Potentially multiple versions of files based on dataPublisher[]
File serzQuedFile;
#define FN_BUFFER_SZ 13
char        serzQuedFn[FN_BUFFER_SZ] = "";
const char* serzQuedFn_str           = "QUE";  // begin of name, keep 8.3


// perform a serialize to RdelFile
bool serzRdel_Line(void);
// Uses serzRdelFn_str, File serzRdelFile
bool  deszRdelStart();
char* deszFind(const char* in_line, char caller_id);
#define deszRdelLine() deszLine(&serzRdelFile)
bool deszRdelClose(bool deleteFile = false);

// Uses serzQuedFn_str, File  serzQuedFile
bool serzQuedStart(char uniqueId);  // Use 1st, & sets filename
bool deszQuedStart(void);
#define deszQuedLine() deszLine(&serzQuedFile)
uint16_t serzQuedFlushFile();
bool serzQuedCloseFile(bool action);
/*
bool deszQuedCleanup(bool debug = false);
*/
// This does the work
bool deszLine(File* filep);

// Utility resources
//void setFileTimeStampMet(File fileToStamp, uint8_t stampFlag);
bool deszDbg(void);
bool postLogOpen();
bool postLogOpen(const char* postsLogNm_str);
void postLogLine(uint32_t tmr_ms, int16_t rspParam);
void postLogLine(const char *logMsg,bool addCR=true);
// Macro to print to TTY and log on uSD 
#define PRINT_LOGLINE_P(msg_parm) \
    char tttbuf[sizeof(msg_parm)+1]; \
    strcpy_P(tttbuf,msg_parm);\
    PRINTOUT(tttbuf);\
    postLogLine(tttbuf);
void postLogClose();
bool listFile(File* filep, char* fn_str, char* uid);

public:
// A simple data-time formatting, compatible with reading into excel spreadsheet - see also formatDateTime_ISO8601()
String formatDateTime_str(DateTime& dt);
String formatDateTime_str(uint32_t epochTime);
bool serzBegin(void);
// The SD card and file
#if 0  // defined BOARD_SDQ_QSPI_FLASH
       // This can be on the Adafruit Express options
       //#include "Adafruit_SPIFlash.h"
#if defined(__SAMD51__) || defined(NRF52840_XXAA)
        static Adafruit_FlashTransport_QSPI sdq_flashspi_transport_QSPI_phy; //Uses default pin assignments
#else
#if (SPI_INTERFACES_COUNT == 1)
                Adafruit_FlashTransport_SPI sdq_flashspi_transport_QSPI_phy(SS, &SPI);
#else
                Adafruit_FlashTransport_SPI sdq_flashspi_transport_QSPI_phy(SS1, &SPI1);
#endif
#endif  //__SAMD51__ ..
        // Adafruit_SPIFlash sdq_flashspi_phy(&sdq_flashspi_transport_QSPI_phy);
        // The following needs to be extended to allow
        // sdq_flashspi_transport_QSPI to be added at runtime This is in
        // Adafruit_SPIFlashBase(Adafruit_SPIFlashBase)
        static Adafruit_SPIFlash sdq_flashspi_phy; ** needs to init sdq_flashspi_transport_QSPI_phy
        static FatFileSystem sdq_card_phy; //// sdq_flashspi_fatfs;  File system object on external flash from SdFat
#endif  // SDQ_QSPI_FLASH
#endif  // LoggerBaseExt_h
