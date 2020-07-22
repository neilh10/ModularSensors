#ifndef LoggerBaseExtH_h
#define LoggerBaseExtH_h

// ===================================================================== //
// Public functions extensions for basic logging paramters
// ===================================================================== //

// void setLoggingInterval_atl485(uint16_t loggingIntervalMinutes);

void setLoggerId(const char* newLoggerId, bool copyId = false,
                 uint8_t LoggerIdSize = NEW_LOGGERID_MAX_SIZE);


void setSendEveryX(uint8_t param) {
    _sendEveryX_num = param;
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

// ===================================================================== //
// Public functions for a "reading .ini" file
// ===================================================================== //

public:
bool parseIniSd(const char* ini_fn, ini_handler_atl485 handler_fn);
#ifdef USE_MS_SD_INI
void setPs_cache(persistent_store_t* ps_ram);
void printFileHeaderExtra(Stream* stream);
#endif  // USE_MS_SD_INI
// virtual bool parseIniSd(const char *ini_fn,ini_handler handler_fn);
void        SDusbPoll(uint8_t sdActions);
#if defined USE_RTCLIB
USE_RTCLIB* rtcExtPhyObj();
#endif  // USE_RTCLIB
static bool usbDriveActive(void);
// Time woken up
uint32_t wakeUpTime_secs;


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

// ===================================================================== //
// Serializing/Deserialing through READINGS.TXT
// ===================================================================== //
public:
bool     deszReadNext(void);
long     queFile_epochTime = 0;  // Marked Epoch Time
char*    queFile_nextChar;
uint16_t nextStr_sz;

private:
long queFile_status = 0;  // Bit wise status of reading


// keep to LFN - capitals  https://en.wikipedia.org/wiki/8.3_filename
File        deszReadFile;
const char* readingsFn_str = "READINGS.TXT";

File        postsLogHndl;                    // Record all POSTS when enabled
const char* postsLogFn_str = "POSTLOG.TXT";  // Not more than 8.3

File        quedFileHndl;            // Save for que - keep to 8.3
const char* quedFileFn_str = "QUE";  // begin of name
#define FN_BUFFER_SZ 13
char quedFn_str[FN_BUFFER_SZ] = "";

uint16_t deszLinesRead   = 0;
uint16_t deszLinesUnsent = 0;
#define QUEFILE_MAX_LINE 100
char deslzFile_line[QUEFILE_MAX_LINE] = "";

void publishDataQuedToRemotes(void);
// bool serzRead(void);
bool serzReadLine(void);
// void serzReadFn(void);
// bool serzQueFn(char* instance);
void serzQueCloseFile(bool action);

// Uses READINGS.TXT
bool deszReadStart();
#define deszReadLine() deszLine(&deszReadFile)
bool deszReadClose(bool deleteFile = false);


/* 
bool deszQuedCleanup(bool debug = false);
*/
// does the work
bool deszLine(File* filep);


bool deszDbg(void);
void setFileAccessTime(File* fileToStamp);


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
