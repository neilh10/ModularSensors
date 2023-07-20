/**
 * @file LoggerBase.cpp
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the Logger class.
 */

#include <ModularSensors.h>   // Needed for the version number
#include "dataPublisherBase.h"

/**
 * @brief To prevent compiler/linker crashes with enable interrupt library, we
 * must define LIBCALL_ENABLEINTERRUPT before importing EnableInterrupt within a
 * library.
 */
#define LIBCALL_ENABLEINTERRUPT
// To handle external and pin change interrupts
#include <EnableInterrupt.h>
// For all i2c communication, including with the real time clock
#include <Wire.h>


#if defined BOARD_SDQ_QSPI_FLASH
// This works as a static instance and allows initializer for
Adafruit_FlashTransport_QSPI
                  sdq_flashspi_transport_QSPI_phy;  // Uses default pin for SQSP
Adafruit_SPIFlash sdq_flashspi_phy(&sdq_flashspi_transport_QSPI_phy);

// File system object on external flash from SdFat
FatFileSystem sd0_card_fatfs;

// Set to true when PC write to flash
bool sd1_card_changed = false;
bool sd0_card_changed = false;
bool usbDriveStatus   = false;

#endif  // BOARD_SDQ_QSPI_FLASH
#if defined USE_TINYUSB
// USB Mass Storage object - indepedent of other objects
Adafruit_USBD_MSC usb_msc;
#endif  // USE_TINYUSB

// Time Zone support in hours from UTC/GMT −10 to +14
// https://en.wikipedia.org/wiki/Coordinated_Universal_Time
// Initialize the static timezone
int8_t Logger::_loggerTimeZone = 0;
// Initialize the static time adjustment
int8_t Logger::_loggerRTCOffset = 0;
// Initialize the static timestamps
uint32_t Logger::markedLocalEpochTime = 0;
uint32_t Logger::markedUTCEpochTime   = 0;
// Initialize the testing/logging flags
volatile bool Logger::isLoggingNow = false;
volatile bool Logger::isTestingNow = false;
volatile bool Logger::startTesting = false;

// For SAMD or processors with internal RTC, there may also be an external RTC
// that keeps time through power off. 
// The internal RTC is zero_sleep_rtc  name traceable to SAMD21/Arduino ZERO
// Any external RTC is named as rtcExtPhy. The Mayfly only has the external RTC
// Initialize the RTC for the SAMD boards
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_SAMD_ZERO)
// RTCZero internal registers based on year 2000/20yk
// "Epoch19yk" seconds from 1900, using  "struct tm", mktime, gmtime
RTC_INT_CLASS Logger::zero_sleep_rtc;
//RTC_INT_CLASS zero_sleep_rtc;
//RTCZero Logger::zero_sleep_rtc;
#define zr zero_sleep_rtc
//For time being assume ony internal RTC - zero_sleep_rtc  name to be changed later
#define rtcExtPhy zero_sleep_rtc
//SAMD51 has two Alarms and requires an ID, SAMD21 had one Alarm
#define RTC_ALM_ID 0
#endif

#if defined USE_RTCLIB
// or change to 
// USE_RTC_EXTPHY rtcExtPhy;
USE_RTCLIB  rtcExtPhy;
// For RTClib.h:DateTime(uint32_t) use secs since 1970
#define DateTimeClass(varNam, epochTime) DateTime varNam(epochTime);
#else
// For Sodaq_DS3231.h:DateTime(long) uses secs since 2000
#define DateTimeClass(varNam, epochTime) \
    DateTime varNam((long)((uint64_t)(epochTime - EPOCH_TIME_DTCLASS)));
#endif  //  USE_RTCLIB

// Constructors
Logger::Logger(const char* loggerID, uint16_t loggingIntervalMinutes,
               int8_t SDCardSSPin, int8_t mcuWakePin, VariableArray* inputArray)
    : _SDCardSSPin(SDCardSSPin),
      _mcuWakePin(mcuWakePin) {
    // Set parameters from constructor
    setLoggerID(loggerID);
    setLoggingInterval(loggingIntervalMinutes);
    setVariableArray(inputArray);

    // Set the testing/logging flags to false
    isLoggingNow = false;
    isTestingNow = false;
    startTesting = false;

    // Set the initial pin values
    // NOTE: Only setting values here, not the pin mode.
    // The pin mode can only be set at run time, not here at compile time.

    // Clear arrays
    for (uint8_t i = 0; i < MAX_NUMBER_SENDERS; i++) {
        dataPublishers[i] = nullptr;
    }
}
Logger::Logger(const char* loggerID, uint16_t loggingIntervalMinutes,
               VariableArray* inputArray) {
    // Set parameters from constructor
    setLoggerID(loggerID);
    setLoggingInterval(loggingIntervalMinutes);
    setVariableArray(inputArray);

    // Set the testing/logging flags to false
    isLoggingNow = false;
    isTestingNow = false;
    startTesting = false;

    // Clear arrays
    for (uint8_t i = 0; i < MAX_NUMBER_SENDERS; i++) {
        dataPublishers[i] = nullptr;
    }
}
Logger::Logger() {
    // Set the testing/logging flags to false
    isLoggingNow = false;
    isTestingNow = false;
    startTesting = false;

    // Clear arrays
    for (uint8_t i = 0; i < MAX_NUMBER_SENDERS; i++) {
        dataPublishers[i] = nullptr;
    }
}
// Destructor
Logger::~Logger() {}


// ===================================================================== //
// Public functions to get and set basic logging paramters
// ===================================================================== //

// Sets the logger ID
void Logger::setLoggerID(const char* loggerID) {
    _loggerID = loggerID;
}

// Sets/Gets the logging interval
void Logger::setLoggingInterval(uint16_t loggingIntervalMinutes) {
    _loggingIntervalMinutes = loggingIntervalMinutes;
}


// Adds the sampling feature UUID
void Logger::setSamplingFeatureUUID(const char* samplingFeatureUUID) {
    _samplingFeatureUUID = samplingFeatureUUID;
}

// Sets up a pin controlling the power to the SD card
void Logger::setSDCardPwr(int8_t SDCardPowerPin) {
    _SDCardPowerPin = SDCardPowerPin;
    if (_SDCardPowerPin >= 0) {
        pinMode(_SDCardPowerPin, OUTPUT);
        digitalWrite(_SDCardPowerPin, LOW);
        MS_DBG(F("Pin"), _SDCardPowerPin, F("set as SD Card Power Pin"));
    }
}
// NOTE:  Structure of power switching on SD card taken from:
// https://thecavepearlproject.org/2017/05/21/switching-off-sd-cards-for-low-power-data-logging/
void Logger::turnOnSDcard(bool waitToSettle) {
    if (_SDCardPowerPin >= 0) {
        digitalWrite(_SDCardPowerPin, HIGH);
        // TODO(SRGDamia1):  figure out how long to wait
        if (waitToSettle) { delay(6); }
    }
}
void Logger::turnOffSDcard(bool waitForHousekeeping) {
    if (_SDCardPowerPin >= 0) {
        // TODO(SRGDamia1): set All SPI pins to INPUT?
        // TODO(SRGDamia1): set ALL SPI pins HIGH (~30k pull-up)
        pinMode(_SDCardPowerPin, OUTPUT);
        digitalWrite(_SDCardPowerPin, LOW);
        // TODO(SRGDamia1):  wait in lower power mode
        if (waitForHousekeeping) {
            // Specs say up to 1s for internal housekeeping after each write
            delay(1000);
        }
    }
}


// Sets up a pin for the slave select (chip select) of the SD card
void Logger::setSDCardSS(int8_t SDCardSSPin) {
    _SDCardSSPin = SDCardSSPin;
    if (_SDCardSSPin >= 0) {
        pinMode(_SDCardSSPin, OUTPUT);
        MS_DBG(F("Pin"), _SDCardSSPin, F("set as SD Card Slave/Chip Select"));
    }
}


// Sets both pins related to the SD card
void Logger::setSDCardPins(int8_t SDCardSSPin, int8_t SDCardPowerPin) {
    setSDCardPwr(SDCardPowerPin);
    setSDCardSS(SDCardSSPin);
}


// Sets up the wake up pin for an RTC interrupt
// NOTE:  This sets the pin mode but does NOT enable the interrupt!
void Logger::setRTCWakePin(int8_t mcuWakePin, uint8_t wakePinMode) {
    _mcuWakePin = mcuWakePin;
    if (_mcuWakePin >= 0) {
        pinMode(_mcuWakePin, wakePinMode);
        MS_DBG(F("Pin"), _mcuWakePin, F("set as RTC wake up pin"));
    } else {
        MS_DBG(F("Logger mcu will not sleep between readings!"));
    }
}


// Sets up a pin for an LED or other way of alerting that data is being logged
void Logger::setAlertPin(int8_t ledPin) {
    _ledPin = ledPin;
    if (_ledPin >= 0) {
        pinMode(_ledPin, OUTPUT);
        MS_DBG(F("Pin"), _ledPin, F("set as LED alert pin"));
    }
}
void Logger::alertOn() {
    if (_ledPin >= 0) { digitalWrite(_ledPin, HIGH); }
}
void Logger::alertOff() {
    if (_ledPin >= 0) { digitalWrite(_ledPin, LOW); }
}


// Sets up a pin for an interrupt to enter testing mode
void Logger::setTestingModePin(int8_t buttonPin, uint8_t buttonPinMode) {
    _buttonPin = buttonPin;

    // Set up the interrupt to be able to enter sensor testing mode
    // NOTE:  Entering testing mode before the sensors have been set-up may
    // give unexpected results.
    if (_buttonPin >= 0) {
        pinMode(_buttonPin, buttonPinMode);
        enableInterrupt(_buttonPin, Logger::testingISR, CHANGE);
        MS_DBG(F("Button on pin"), _buttonPin,
               F("can be used to enter sensor testing mode."));
    }
}


// Sets up the five pins of interest for the logger
void Logger::setLoggerPins(int8_t mcuWakePin, int8_t SDCardSSPin,
                           int8_t SDCardPowerPin, int8_t buttonPin,
                           int8_t ledPin, uint8_t wakePinMode,
                           uint8_t buttonPinMode) {
    setRTCWakePin(mcuWakePin, wakePinMode);
    setSDCardSS(SDCardSSPin);
    setSDCardPwr(SDCardPowerPin);
    setTestingModePin(buttonPin, buttonPinMode);
    setAlertPin(ledPin);
}


// ===================================================================== //
// Public functions to get information about the attached variable array
// ===================================================================== //

// Assigns the variable array object
void Logger::setVariableArray(VariableArray* inputArray) {
    _internalArray = inputArray;
}


// Returns the number of variables in the internal array
uint8_t Logger::getArrayVarCount() {
    return _internalArray->getVariableCount();
}


// This gets the name of the parent sensor, if applicable
String Logger::getParentSensorNameAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getParentSensorName();
}
// This gets the details of the parent sensor, if applicable
String Logger::getParentSensorDetails(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getParentSensorDetails();
}
// This gets the name and location of the parent sensor, if applicable
String Logger::getParentSensorNameAndLocationAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]
        ->getParentSensorNameAndLocation();
}
// This gets the variable's name using http://vocabulary.odm2.org/variablename/
String Logger::getVarNameAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getVarName();
}
// This gets the variable's unit using http://vocabulary.odm2.org/units/
String Logger::getVarUnitAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getVarUnit();
}
// This returns a customized code for the variable, if one is given, and a
// default if not
String Logger::getVarCodeAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getVarCode();
}
// This returns the variable UUID, if one has been assigned
String Logger::getVarUUIDAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getVarUUID();
}
// This returns the current value of the variable as a string with the
// correct number of significant figures
String Logger::getValueStringAtI(uint8_t position_i) {
    return _internalArray->arrayOfVars[position_i]->getValueString();
}


// ===================================================================== //
// Public functions for internet and dataPublishers
// ===================================================================== //

// Set up communications
// Adds a loggerModem objct to the logger
// loggerModem = TinyGSM modem + TinyGSM client + Modem On Off
void Logger::attachModem(loggerModem& modem) {
    _logModem = &modem;
}
void Logger::attachModem(loggerModem* modem) {
    _logModem = modem;
}


// Takes advantage of the modem to synchronize the clock
bool Logger::syncRTC() {
    bool success = false;
    if (_logModem != nullptr) {
        // Synchronize the RTC with NIST
        PRINTOUT(F("Attempting to connect to the internet and synchronize RTC "
                   "with NIST"));
        PRINTOUT(F("This may take up to two minutes!"));
        if (_logModem->modemWake()) {
            watchDogTimer.resetWatchDog();
            if (_logModem->connectInternet(120000L)) {
                const static char CONNECT_INTERNET_pm[] EDIY_PROGMEM = 
                "Connected to internet for RTC sync with NIST"; 
                PRINT_LOGLINE_P(CONNECT_INTERNET_pm);
                watchDogTimer.resetWatchDog();
                success = setRTClock(_logModem->getNISTTime());
                // success = true;
                watchDogTimer.resetWatchDog();
                _logModem->updateModemMetadata();
            } else {
                const static char COULD_NOT_CONNECT_INTERNET_pm[] EDIY_PROGMEM   = 
                "Could not connect to internet for RTC sync.";
                PRINT_LOGLINE_P(COULD_NOT_CONNECT_INTERNET_pm);
            }
        } else {
            const static char COULD_NOT_WAKE_pm[] EDIY_PROGMEM = "Could not wake modem for RTC sync.";
            PRINT_LOGLINE_P(COULD_NOT_WAKE_pm);
        }
        watchDogTimer.resetWatchDog();
        // Power down the modem - but only if there will be more than 15 seconds
        // before the NEXT logging interval - it can take the modem that long to
        // shut down

        uint32_t setupFinishTime = getNowLocalEpoch();
        if (setupFinishTime % (_loggingIntervalMinutes * 60) > 15) {
            MS_DBG(F("At"), formatDateTime_ISO8601(setupFinishTime), F("with"),
                   setupFinishTime % (_loggingIntervalMinutes * 60),
                   F("seconds until next logging interval, putting modem to "
                     "sleep"));
            _logModem->disconnectInternet();
            _logModem->modemSleepPowerDown();
        } else {
            MS_DBG(F("At"), formatDateTime_ISO8601(setupFinishTime),
                   F("there are only"),
                   setupFinishTime % (_loggingIntervalMinutes * 60),
                   F("seconds until next logging interval; leaving modem on "
                     "and connected to the internet."));
        }
    }
    watchDogTimer.resetWatchDog();
    return success;
}


void Logger::registerDataPublisher(dataPublisher* publisher) {
    // find the next empty spot in the publisher array
    uint8_t i = 0;
    for (; i < MAX_NUMBER_SENDERS; i++) {
        if (dataPublishers[i] == publisher) {
            MS_DBG(F("dataPublisher already registered."));
            return;
        }
        if (dataPublishers[i] == nullptr) break;
    }

    // register the publisher there
    dataPublishers[i] = publisher;
}


void Logger::publishDataToRemotes(void) {
    // Assumes that there is an internet connection
    MS_DBG(F("Sending out remote data."));

    for (uint8_t i = 0; i < MAX_NUMBER_SENDERS; i++) {
        if (dataPublishers[i] != nullptr) {
            _dataPubInstance = i;
            PRINTOUT(F("\nSending data to ["), i, F("]"),
                     dataPublishers[i]->getEndpoint());
            dataPublishers[i]->publishData();
            watchDogTimer.resetWatchDog();
        }
    }
}
void Logger::sendDataToRemotes(void) {
    publishDataToRemotes();
}


// ===================================================================== //
// Public functions to access the clock in proper format and time zone
// ===================================================================== //

// Sets the static timezone that the data will be logged in - this must be set
void Logger::setLoggerTimeZone(int8_t timeZone) {
    _loggerTimeZone = timeZone;
// Some helpful prints for debugging
#ifdef STANDARD_SERIAL_OUTPUT
    const char* prtout1 = "Logger timezone is set to UTC";
    if (_loggerTimeZone == 0) {
        PRINTOUT(prtout1);
    } else if (_loggerTimeZone > 0) {
        PRINTOUT(prtout1, '+', _loggerTimeZone);
    } else {
        PRINTOUT(prtout1, _loggerTimeZone);
    }
#endif
}
int8_t Logger::getLoggerTimeZone(void) {
    return Logger::_loggerTimeZone;
}
// Duplicates for backwards compatibility
void Logger::setTimeZone(int8_t timeZone) {
    setLoggerTimeZone(timeZone);
}
int8_t Logger::getTimeZone(void) {
    return getLoggerTimeZone();
}

// Sets the static timezone that the RTC is programmed in
// I VERY VERY STRONGLY RECOMMEND SETTING THE RTC IN UTC
// You can either set the RTC offset directly or set the offset between the
// RTC and the logger
void Logger::setRTCTimeZone(int8_t timeZone) {
    _loggerRTCOffset = _loggerTimeZone - timeZone;
// Some helpful prints for debugging
#ifdef STANDARD_SERIAL_OUTPUT
    const char* prtout1 = "RTC timezone is set to UTC";
    if ((_loggerTimeZone - _loggerRTCOffset) == 0) {
        PRINTOUT(prtout1);
    } else if ((_loggerTimeZone - _loggerRTCOffset) > 0) {
        PRINTOUT(prtout1, '+', (_loggerTimeZone - _loggerRTCOffset));
    } else {
        PRINTOUT(prtout1, (_loggerTimeZone - _loggerRTCOffset));
    }
#endif
}
int8_t Logger::getRTCTimeZone(void) {
    return Logger::_loggerTimeZone - Logger::_loggerRTCOffset;
}


// This set the offset between the built-in clock and the time zone where
// the data is being recorded.  If your RTC is set in UTC and your logging
// timezone is EST, this should be -5.  If your RTC is set in EST and your
// timezone is EST this does not need to be called.
// You can either set the RTC offset directly or set the offset between the
// RTC and the logger
void Logger::setTZOffset(int8_t offset) {
    _loggerRTCOffset = offset;
    // Some helpful prints for debugging
    if (_loggerRTCOffset == 0) {
        PRINTOUT(F("RTC and Logger are set in the same timezone."));
    } else if (_loggerRTCOffset < 0) {
        PRINTOUT(F("RTC is set"), -1 * _loggerRTCOffset,
                 F("hours ahead of logging timezone"));
    } else {
        PRINTOUT(F("RTC is set"), _loggerRTCOffset,
                 F("hours behind the logging timezone"));
    }
}
int8_t Logger::getTZOffset(void) {
    return Logger::_loggerRTCOffset;
}

// This gets the current epoch time (unix time, ie, the number of seconds
// from January 1, 1970 00:00:00 UTC) and corrects it to the specified time zone
#if defined MS_SAMD_DS3231 || not defined ARDUINO_ARCH_SAMD

uint32_t Logger::getNowEpoch(void) {
    // Depreciated in 0.33.0, left in for compatiblity
    return getNowLocalEpoch();
}
uint32_t Logger::getNowLocalEpoch(void) {
    uint32_t currentEpochTime = getNowUTCEpoch();
    // Do NOT apply an offset if the timestamp is obviously bad
    if (isRTCSane(currentEpochTime))
        currentEpochTime += ((uint32_t)_loggerRTCOffset) * 3600;
    return currentEpochTime;
}


uint32_t Logger::getNowUTCEpoch(void) {
    uint32_t currentEpochTime = rtcExtPhy.now().getEpoch();
    if (!isRTCSane(currentEpochTime)) {
        PRINTOUT(F("Bad time "), currentEpochTime, " ",
                 formatDateTime_ISO8601(currentEpochTime).substring(0, 10),
                 " Setting to ",
                 formatDateTime_ISO8601(EPOCH_TIME_LOWER_SANITY_SECS));
        currentEpochTime = EPOCH_TIME_LOWER_SANITY_SECS;
        setNowUTCEpoch(currentEpochTime);
    }

    return currentEpochTime;
}
void Logger::setNowUTCEpoch(uint32_t ts) {
    rtcExtPhy.setEpoch(ts);
}

#elif defined ARDUINO_ARCH_SAMD

uint32_t Logger::getNowUTCEpoch(void) {
    uint32_t currentEpochTime = zero_sleep_rtc.now().unixtime();
    if (!isRTCSane(currentEpochTime)) {
        PRINTOUT(F("Bad time, resetting clock."), currentEpochTime, " ",
                 formatDateTime_ISO8601(currentEpochTime), " Setting to ",
                 formatDateTime_ISO8601(EPOCH_TIME_LOWER_SANITY_SECS));
        currentEpochTime = EPOCH_TIME_LOWER_SANITY_SECS;
        setNowUTCEpoch(currentEpochTime);
    }
    return currentEpochTime;
}
void Logger::setNowUTCEpoch(uint32_t ts) {
    zero_sleep_rtc.adjust(DateTime(ts));
}
uint32_t Logger::getNowLocalEpoch(void) {
    return (uint32_t)(getNowUTCEpoch() + (_loggerRTCOffset * HOURS_TO_SECS));
}

#endif

// This converts the current UNIX timestamp (ie, the number of seconds
// from January 1, 1970 00:00:00 UTC) into a DateTime object
// The DateTime object constructor requires the number of seconds from
// January 1, 2000 (NOT 1970) as input, so we need to subtract.
DateTime Logger::dtFromEpoch(uint32_t epochTime) {
    #if defined __AVR__
    DateTime dt(epochTime - EPOCH_TIME_OFF);
    #else
    //SeeedArduinoRTC lib assumes Epoch
    DateTime dt(epochTime);
    #endif 
    return dt;
}

DateTime Logger::dtFromEpochUTC(uint32_t epochTimeUTC) {
    // DateTime dt(epochTimeUTC-EPOCH_TIME_OFF);
    DateTimeClass(dt, epochTimeUTC) return dt;
}

DateTime Logger::dtFromEpochTz(uint32_t epochTimeTz) {
    // The DateTime object constructor requires the number of seconds from
    // January 1, 2000 (NOT 1970) as input, so we need to subtract.
    // DateTime dtTz(epochTimeTz - EPOCH_TIME_OFF);
    DateTimeClass(dtTz, epochTimeTz) return dtTz;
}

// This converts a date-time object into a ISO8601 formatted string
// It assumes the supplied date/time is in the LOGGER's timezone and adds
// the LOGGER's offset as the time zone offset in the string.
String Logger::formatDateTime_ISO8601(DateTime& dt) {
    // Set up an inital string
    String dateTimeStr;
    // Convert the DateTime object to a String
    dt.addToString(dateTimeStr);
    dateTimeStr.replace(" ", "T");
    auto tzString = String(_loggerTimeZone);
    if (-24 <= _loggerTimeZone && _loggerTimeZone <= -10) {
        tzString += F(":00");
    } else if (-10 < _loggerTimeZone && _loggerTimeZone < 0) {
        tzString = tzString.substring(0, 1) + '0' + tzString.substring(1, 2) +
            F(":00");
    } else if (_loggerTimeZone == 0) {
        tzString = 'Z';
    } else if (0 < _loggerTimeZone && _loggerTimeZone < 10) {
        tzString = "+0" + tzString + F(":00");
    } else if (10 <= _loggerTimeZone && _loggerTimeZone <= 24) {
        tzString = "+" + tzString + F(":00");
    }
    dateTimeStr += tzString;
    return dateTimeStr;
}

// This converts an epoch time (unix time) into a ISO8601 formatted string.
// It assumes the supplied date/time is in the LOGGER's timezone
String Logger::formatDateTime_ISO8601(uint32_t epochTimeTz) {
    // Create a DateTime object from the epochTime
    DateTimeClass(dtTz, epochTimeTz);
    return formatDateTime_ISO8601(dtTz);
}


// This sets the real time clock to the given time
bool Logger::setRTClock(uint32_t UTCEpochSeconds) {
    bool retVal = false;

    // If the timestamp is zero, just exit
    if (UTCEpochSeconds == 0) {
        PRINTOUT(F("Bad timestamp, not setting clock."));
        return false;
    }

    // The "setTime" is the number of seconds since Jan 1, 1970 in UTC
    // We're interested in the setTime in the logger's and RTC's timezone
    // The RTC's timezone is equal to the logger's timezone minus the offset
    // between the logger and the RTC.
    // Only works for ARM CC if long, AVR was uint32_t
    long set_rtcTZ = UTCEpochSeconds;
    // NOTE:  We're only looking at local time here in order to print it out for
    // the user
    uint32_t set_logTZ = UTCEpochSeconds +
        ((uint32_t)getLoggerTimeZone()) * 3600;
    MS_DBG(F("    Time for Logger supplied by NIST:"), set_logTZ, F("->"),
           formatDateTime_ISO8601(set_logTZ));

    // Check the current RTC time
    uint32_t cur_logTZ = getNowLocalEpoch();
    MS_DBG(F("    Current Time on RTC:"), cur_logTZ, F("->"),
           formatDateTime_ISO8601(cur_logTZ));
    MS_DBG(F("    Offset between NIST and RTC:"), abs(set_logTZ - cur_logTZ));

    // NOTE:  Because we take the time to do some UTC/Local conversions and
    // print stuff out, the clock might end up being set up to a few
    // milliseconds behind the input time.  Given the clock is only accurate to
    // seconds (not milliseconds or less), I don't think this is a problem.

    // If the RTC and NIST disagree by more than 5 seconds, set the clock
    #define NIST_TIME_DIFF_SEC 5
    if (abs(set_logTZ - cur_logTZ) > 5) {
        setNowUTCEpoch(set_rtcTZ);
        PRINTOUT(F("Internal Clock set "), formatDateTime_ISO8601(set_rtcTZ));
        retVal = true;
    } else {
        PRINTOUT(F("Internal Clock within "), NIST_TIME_DIFF_SEC,
                 F("seconds of NIST."));
        // return false;
        retVal = true;  // RTC valid
    }
#if defined ADAFRUIT_FEATHERWING_RTC_SD || defined USE_RTCLIB
    // Check the current ExtRtc time -
    DateTime nowExtUTC          = rtcExtPhy.now();  // UTC
    uint32_t nowExtUTCEpoch_sec = nowExtUTC.unixtime();
    MS_DBG("         Time Returned by rtcExt:", nowExtUTCEpoch_sec,
           "->(T=", getTimeZone(), ")",
           formatDateTime_ISO8601(nowExtUTCEpoch_sec));
    uint32_t time_diff_sec = abs((long)((uint64_t)nowExtUTCEpoch_sec) -
                        (long)((uint64_t)UTCEpochSeconds));
    if (time_diff_sec > NIST_TIME_DIFF_SEC) {
        rtcExtPhy.adjust(UTCEpochSeconds);  // const DateTime& dt);
        nowExtUTC = rtcExtPhy.now();
        MS_DBG("         rtcExt diff", time_diff_sec, " updated to UTS ",
               UTCEpochSeconds, "->", formatDateTime_ISO8601(UTCEpochSeconds));
        retVal = true;
    }

#endif  // ADAFRUIT_FEATHERWING_RTC_SD
    return retVal;
}

// This checks that the logger time is within a "sane" range
bool Logger::isRTCSane(void) {
    uint32_t curRTC = getNowLocalEpoch();
    return isRTCSane(curRTC);
}
bool Logger::isRTCSane(uint32_t epochTime) {
    if (epochTime < EPOCH_TIME_LOWER_SANITY_SECS || /*bad before <date>*/
        epochTime > EPOCH_TIME_UPPER_SANITY_SECS)   /*bad after <date>*/
    {
        return false;
    } else {
        return true;
    }
}


// This sets static variables for the date/time - this is needed so that all
// data outputs (SD, EnviroDIY, serial printing, etc) print the same time
// for updating the sensors - even though the routines to update the sensors
// and to output the data may take several seconds.
// It is not currently possible to output the instantaneous time an individual
// sensor was updated, just a single marked time.  By custom, this should be
// called before updating the sensors, not after.
void Logger::markTime(void) {
    Logger::markedUTCEpochTime   = getNowUTCEpoch();
    Logger::markedLocalEpochTime = markedUTCEpochTime +
        ((uint32_t)_loggerRTCOffset) * 3600;
    
    MS_DEEP_DBG(F("markTime UTC"), markedUTCEpochTime,F("local"),formatDateTime_ISO8601(markedLocalEpochTime),markedLocalEpochTime  );
}


// This checks to see if the CURRENT time is an even interval of the logging
// rate
uint8_t Logger::checkInterval(void) {
    uint8_t retval = CIA_NOACTION;
    uint32_t checkTime = getNowLocalEpoch();
    int modulus_time_sec =checkTime % (_loggingIntervalMinutes * 60);
    MS_DBG(F("Current Epoch local Timestamp:"), checkTime, F("->"),
           formatDateTime_ISO8601(checkTime));
    MS_DBG(F("Logging interval in seconds:"), (_loggingIntervalMinutes * 60));
    MS_DBG(F("Mod of Logging Interval:"),
           modulus_time_sec);

    if (_sendOffset_act) {
        // A Timer is counting down to perform delayed Post Readings
        if (0 >= --_sendOffset_cnt) {
            // Timer has expired
            _sendOffset_act = false;
            retval |= CIA_POST_READINGS;
            MS_DBG(F("sendOffset Post Readings"));
        } else {
            MS_DBG(F("sendOffset Timer "), _sendOffset_cnt);
        }
    }

    if (modulus_time_sec < 59 ) {
        // Update the time variables with the current time
        markTime();
        MS_DBG(F("Take Sensor readings. Epoch:"), Logger::markedLocalEpochTime);

        // Check what actions for this time period
        retval |= CIA_NEW_READING;
        if (1 < _sendEveryX_num) {
            _sendEveryX_cnt++;
            if (_sendEveryX_cnt >= _sendEveryX_num) {
                _sendEveryX_cnt = 0;
                // Check if delay ~ offset to Send Readings
                if (0 == _sendOffset_min) {
                    // No dealy ~ send readings now
                    retval |= CIA_POST_READINGS;
                    MS_DBG(F("sendEveryX Post Readings"));
                } else {
                    // delayed retval |= CIA_POST_READINGS;
                    _sendOffset_act = true;
                    _sendOffset_cnt = _sendOffset_min;
                    MS_DBG(F("sendEveryX Timer sendOffset started "),
                           _sendOffset_min);
                }
            } else {
                MS_DBG(F("sendEveryX "), _sendEveryX_cnt, F("counting to "),
                       _sendEveryX_num);
            }
        } else {
            retval |= CIA_POST_READINGS;
            MS_DBG(F("Post readings."));
        }
    } else {
        MS_DBG(F("Not time yet."));
    }
    if (!isRTCSane(checkTime)) {
        PRINTOUT(F("----- WARNING ----- !!!!!!!!!!!!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!! ----- WARNING ----- !!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!!!!!!!!!!!! ----- WARNING ----- "));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(' ');
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("The current clock timestamp is not valid!"),
                 formatDateTime_ISO8601(getNowUTCEpoch()).substring(0, 10));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(' ');
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("----- WARNING ----- !!!!!!!!!!!!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!! ----- WARNING ----- !!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!!!!!!!!!!!! ----- WARNING ----- "));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
    }
    return retval;
}


// This checks to see if the MARKED time is an even interval of the logging rate
bool Logger::checkMarkedInterval(void) {
    bool retval;
    MS_DBG(F("Marked Time:"), Logger::markedLocalEpochTime,
           F("Logging interval in seconds:"), (_loggingIntervalMinutes * 60),
           F("Mod of Logging Interval:"),
           Logger::markedLocalEpochTime % (_loggingIntervalMinutes * 60));

    if (Logger::markedLocalEpochTime != 0 &&
        (Logger::markedLocalEpochTime % (_loggingIntervalMinutes * 60) == 0)) {
        MS_DBG(F("Time to log!"));
        retval = true;
    } else {
        MS_DBG(F("Not time yet."));
        retval = false;
    }
    return retval;
}


// ============================================================================
//  Public Functions for sleeping the logger
// ============================================================================

// Set up the Interrupt Service Request for waking
// In this case, we're doing nothing, we just want the processor to wake
// This must be a static function (which means it can only call other static
// funcions.)
void Logger::wakeISR(void) {
    MS_DEEP_DBG(F("\nClock interrupt!"));
}

#if defined(__AVR__)
#define freeRamMcr() \
    (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval)
int16_t freeRamLb() {
    extern int16_t __heap_start, *__brkval;
    int16_t   v;
    return  freeRamMcr();
}
/* SP initialized to top of RAM, and grows downwards
Static is allocated at beginning __bss, then heap*/

inline uint16_t freeRamCnt() {
    extern int16_t __heap_start, *__brkval;
    uint16_t  cnt = 0;
    uint8_t * p;
#define START_FREE_RAM ((uint8_t*)(__brkval == 0 ? (int)&__heap_start : (int)__brkval) )
#define END_FREE_RAM   (uint16_t)&p

    for (p = START_FREE_RAM; (uint16_t)p < END_FREE_RAM; p++) {
        if(0 == *p) cnt++;
    }
    return cnt;
}
#if !defined FREE_RAM_SEED 
#define FREE_RAM_SEED 0
#endif
#define DF_RAM_LINE 16

#if defined MS_DUMP_FREE_RAM
inline uint16_t dumpFreeRam(uint16_t maxCount)
{
    extern int16_t __heap_start, *__brkval;
    const uint8_t *rmp = (uint8_t *)(__brkval == 0 ? (int)&__heap_start : (int)__brkval);//_end top heap ;
    uint8_t  rmp_ch;
    char txt_buf[DF_RAM_LINE+2];
    uint8_t txt_idx;
    bool smartList,smartList1st;
    uint8_t lp, sl_cnt;
    uint16_t nu_cnt=0;
    uint16_t fr_cnt=0; //Top of stack, last parmater

    Serial.print(F("\ndumpFreeRam from heap(low)=0x"));
    Serial.print((uint16_t)__brkval,HEX);
    Serial.print(F("to stack(high)=0x"));
    Serial.print((uint16_t)&fr_cnt ,HEX);
    Serial.print(F(")  size(dec):"));
    Serial.print( (uint16_t)&fr_cnt - (uint16_t)rmp );
    rmp =(uint8_t *) ((uint16_t)rmp & 0xFFF0); //Start beginning of 16byte section
    smartList1st=true;
    for (;(uint16_t)rmp< (uint16_t)&fr_cnt; ) 
    {
        //* smart list ~ if all FREE_RAM_SEED don't list */
        smartList = true;
        for (lp=0;lp <DF_RAM_LINE ;lp++) 
        {
            if (FREE_RAM_SEED  != *(rmp+lp)){ 
                smartList=false;
                break;
            }
        } 

        if (!smartList) {
            fr_cnt +=lp;
            //List line compactly in hex
            Serial.print(F("\n0x"));
            Serial.print((uint16_t)rmp,HEX);
            txt_idx=0;
            for (lp=0;lp<DF_RAM_LINE ;lp++) {
                if (0==(lp&0x3)) {Serial.print(F(" "));} //Some readability
                rmp_ch = *rmp;
                if ( 0 == (rmp_ch & 0xF0) ) {Serial.print(F("0"));} //readability
                Serial.print(rmp_ch,HEX);
                if (isprint(rmp_ch)) {
                    txt_buf[txt_idx++]=rmp_ch;
                }else {
                    txt_buf[txt_idx++]='.';
                }
                rmp++;
            }
            delay(5); //10mS~100 lines/sec ~ For115K baud 11K chars/Sec, 183lines/sec
            txt_buf[DF_RAM_LINE]=0; //String terminatior
            Serial.print(F(" ;"));
            Serial.print(txt_buf);
            delay(5);
            smartList1st=true;
        } else {
            fr_cnt += DF_RAM_LINE;
            nu_cnt += DF_RAM_LINE;
            //List header
            if (smartList1st) {
                Serial.print(F("\nUnUsed 1k 0x"));
                smartList1st=false;
                Serial.print((uint16_t)rmp,HEX);
                sl_cnt=0;
            } else {
                Serial.print(F("."));
            }
            // 64*256byte sections
            if (63 < ++sl_cnt) {smartList1st=true;} //Start header agains 
            rmp+=DF_RAM_LINE;
        }
        if(fr_cnt>maxCount){PRINTOUT(F("  ..exceeded maxCount"),maxCount); break; }//Safety breakout
    }
    PRINTOUT(F("\nFree ram never allocated between (bytes dec)"),nu_cnt,F("and"),fr_cnt);
    return fr_cnt;
} //dumpFreeRam
#else 
inline uint16_t dumpFreeRam(uint16_t maxCount) {return 0;}
#endif //MS_DUMP_FREE_RAM
#elif defined(ARDUINO_ARCH_SAMD)
extern "C" char* sbrk(int i);

int16_t freeRamCalcLb() {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}
inline uint16_t freeRamCnt() {return 0;} 
inline uint16_t dumpFreeRam(uint16_t maxCount) {return 0;}
#endif // __AVR__

// Puts the system to sleep to conserve battery life.
// This DOES NOT sleep or wake the sensors!!
#if defined(__AVR__)
void Logger::systemSleep(uint8_t sleep_min) { //__AVR__
#if defined MS_SAMD_DS3231 || not defined ARDUINO_ARCH_SAMD
    // Don't go to sleep unless there's a wake pin!
    if (_mcuWakePin < 0) {
        PRINTOUT(F("MCU not Enabled,Use a non-negative wake pin to request sleep!"), _mcuWakePin);
        return;
    }


    // Unfortunately, because of the way the alarm on the DS3231 is set up, it
    // cannot interrupt on any frequencies other than every second, minute,
    // hour, day, or date.  We could set it to alarm hourly every 5 minutes past
    // the hour, but not every 5 minutes.  This is why we set the alarm for
    // every minute and use the checkInterval function.  This is a hardware
    // limitation of the DS3231; it is not due to the libraries or software.
    MS_DBG(F("Setting alarm on DS3231 RTC for every minute."));
    setExtRtcSleep();

    // Set up a pin to hear clock interrupt and attach the wake ISR to it
    noInterrupts(); // make a transaction, ensure no race condition.
    pinMode(_mcuWakePin, INPUT_PULLUP);
    enableInterrupt(_mcuWakePin, wakeISR, CHANGE);
    interrupts(); 

    // Clear the last interrupt flag in the RTC status register
    // It will float high if not already there, and then be pulled low
    // on next match
    rtcExtPhy.clearINTStatus();
    PRINTOUT(F("Going to sleep. Ram("),freeRamLb(),F("/"),freeRamCnt(),F(")  ZZzzz..."));
#endif

// Wait until the serial ports have finished transmitting
// This does not clear their buffers, it just waits until they are finished
// TODO(SRGDamia1):  Make sure can find all serial ports
#if defined(STANDARD_SERIAL_OUTPUT)
    STANDARD_SERIAL_OUTPUT.flush();  // for debugging
#endif
#if defined DEBUGGING_SERIAL_OUTPUT
    DEBUGGING_SERIAL_OUTPUT.flush();  // for debugging
#endif

    // Stop any I2C connections
    // This function actually disables the two-wire pin functionality and
    // turns off the internal pull-up resistors.
    Wire.end();
// Now force the I2C pins to LOW
// I2C devices have a nasty habit of stealing power from the SCL and SDA pins...
// This will only work for the "main" I2C/TWI interface
#ifdef SDA
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
#endif
#ifdef SCL
    pinMode(SCL, OUTPUT);
    digitalWrite(SCL, LOW);
#endif

#if defined ARDUINO_ARCH_SAMD

    // Disable the watch-dog timer
    watchDogTimer.disableWatchDog();

    // Sleep code from ArduinoLowPowerClass::sleep()
    bool restoreUSBDevice = false;
    // if (SERIAL_PORT_USBVIRTUAL)
    // {
    //     USBDevice.standby();
    // }
    // else
    // {
#ifndef USE_TINYUSB
    USBDevice.detach();
#endif
    restoreUSBDevice = true;
    // }
    // Disable systick interrupt:  See
    // https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    // Now go to sleep
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();

#elif defined ARDUINO_ARCH_AVR

    // Set the sleep mode
    // In the avr/sleep.h file, the call names of these 5 sleep modes are:
    // SLEEP_MODE_IDLE         -the least power savings
    // SLEEP_MODE_ADC
    // SLEEP_MODE_PWR_SAVE
    // SLEEP_MODE_STANDBY
    // SLEEP_MODE_PWR_DOWN     -the most power savings
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Dont disable watch-dog timer, let the extended watchdog handle real timeout.
    watchDogTimer.debugQuiet();  // not watchDogTimer.disableWatchDog();

    // Temporarily disables interrupts, so no mistakes are made when writing
    // to the processor registers
    noInterrupts();

    // Disable the processor ADC (must be disabled before it will power down)
    // ADCSRA = ADC Control and Status Register A
    // ADEN = ADC Enable
    ADCSRA &= ~_BV(ADEN);

// turn off the brown-out detector, if possible
// BODS = brown-out detector sleep
// BODSE = brown-out detector sleep enable
#if defined(BODS) && defined(BODSE)
    sleep_bod_disable();
#endif

    // disable all power-reduction modules (ie, the processor module clocks)
    // NOTE:  This only shuts down the various clocks on the processor via
    // the power reduction register!  It does NOT actually disable the
    // modules themselves or set the pins to any particular state!  This
    // means that the I2C/Serial/Timer/etc pins will still be active and
    // powered unless they are turned off prior to calling this function.
    power_all_disable();

    // Set the sleep enable bit.
    sleep_enable();

    int sleep_cnt=0;
#if defined ARDUINO_ARCH_AVR
    //Assuming an external RTC which activates processor aka Mayfly
    // There maybe intermediate interrupts eg Watchdog, that are ignored
    while (digitalRead(_mcuWakePin)) //when low normal processing.
#endif 
    { 
        // Re-enables interrupts so we can wake up again
        interrupts();

        // Actually put the processor into sleep mode.
        // This must happen after the SE bit is set.
        sleep_cpu();
        sleep_cnt++;
#endif
    // ---------------------------------------------------------------------


    // ---------------------------------------------------------------------
    // -- The portion below this happens on wake up, after any wake ISR's --

#if defined ARDUINO_ARCH_SAMD
    // Reattach the USB after waking
    // Enable systick interrupt
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    if (restoreUSBDevice) {
#ifndef USE_TINYUSB
        USBDevice.attach();
#endif
        uint32_t startTimer = millis();
        while (!SERIAL_PORT_USBVIRTUAL && ((millis() - startTimer) < 1000L)) {
            // wait
        }
    }
#endif

#if defined ARDUINO_ARCH_AVR

        // Temporarily disables interrupts, so no mistakes are made when writing
        // to the processor registers
        noInterrupts();
    } 

    // Re-enable all power modules (ie, the processor module clocks)
    // NOTE:  This only re-enables the various clocks on the processor!
    // The modules may need to be re-initialized after the clocks re-start.
    power_all_enable();

    // Clear the SE (sleep enable) bit.
    sleep_disable();

    // Re-enable the processor ADC
    ADCSRA |= _BV(ADEN);

    // Detach the from the pin - assumes Mayfly
    disableInterrupt(_mcuWakePin);

    // Re-enables interrupts
    interrupts();

#endif

    // Re-enable the watch-dog timer
    watchDogTimer.enableWatchDog();

// Re-start the I2C interface
#ifdef SDA
    pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
#endif
#ifdef SCL
    pinMode(SCL, INPUT_PULLUP);
#endif
    Wire.begin();
    // Eliminate any potential extra waits in the wire library
    // These waits would be caused by a readBytes or parseX being called
    // on wire after the Wire buffer has emptied.  The default stream
    // functions - used by wire - wait a timeout period after reading the
    // end of the buffer to see if an interrupt puts something into the
    // buffer.  In the case of the Wire library, that will never happen and
    // the timeout period is a useless delay.
    Wire.setTimeout(0);

#if defined(MS_SAMD_DS3231) || not defined(ARDUINO_ARCH_SAMD)
    // Stop the clock from sending out any interrupts while we're awake.
    // There's no reason to waste thought on the clock interrupt if it
    // happens while the processor is awake and doing other things.
    // nh: this re-initializes the RTC, maybe over driving the RTC - the disableInterrupt is a better way
    //rtc.disableInterrupts();  after Wire.begin()
    // Detach the from the pin
    //disableInterrupt(_mcuWakePin); moved up disable

#elif defined ARDUINO_ARCH_SAMD
    // not needed zero_sleep_rtc.disableAlarm(RTC_ALM_ID);
#endif

    // Wake-up message
    wakeUpTime_secs = getNowLocalEpoch();
    PRINTOUT(F("\n... zzzZZ Awake @"),sleep_cnt, formatDateTime_ISO8601(wakeUpTime_secs) );
    watchDogTimer.debugInfo();  // enable watchdog indications
    watchDogTimer.resetWatchDog();

    // The logger will now start the next function after the systemSleep
    // function in either the loop or setup
}


// end Logger::systemSleep AVR
#else // !__AVR__
#define serialBaudDebugDef 115200 
#define SerialStd STANDARD_SERIAL_OUTPUT
// https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
void lowpower_disable_ints(void) {
// Wio Terminal
// This is working through the specific schematic to turn off specific parts
// then turning them back on and verifying they are correctly initialized


// Wait until the serial ports have finished transmitting
// This does not clear their buffers, it just waits until they are finished
// TODO(SRGDamia1):  Make sure can find all serial ports
#if defined(STANDARD_SERIAL_OUTPUT)
    STANDARD_SERIAL_OUTPUT.flush();  // for debugging
    //STANDARD_SERIAL_OUTPUT.end(); doesn't work
#endif
#if defined DEBUGGING_SERIAL_OUTPUT
    DEBUGGING_SERIAL_OUTPUT.flush();  // for debugging
#endif
#if defined USB_SERIALSTD 
//tbd - need to detect if USB connected, to determine if needed to restor
#if defined(USE_TINYUSB)
//??
        //USBDevice.detach();
#elif defined(USBCON)
//??
        //USBDevice.detach();
#endif //USE_TINYUSB
#endif // USB_SERIALSTD 

    // Stop any I2C connections
    // This function actually disables the two-wire pin functionality and
    // turns off the internal pull-up resistors.
    Wire.end();

//Wio Terminal has two I2Cs
// I2C0  RPI/U3 ID_SD & ID_SC,  internal LIS3DHTR, ATECC608 
// I2C1  RPI/U3 GPIO2/SDA1 & GPIO3/SCL1
// I2C0_SCL has pullups 4.7K,R14 & R13. so no interal pullups 
//#define PIN_WIRE0_SLEEP  
// Now force the I2C pins to LOW
// I2C devices have a nasty habit of stealing power from the SCL and SDA pins...
// This will only work for the "main" I2C/TWI interface
#if defined PIN_WIRE0_SLEEP
#ifdef  PIN_WIRE_SDA
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
#endif
#ifdef PIN_WIRE_SCL
    pinMode(SCL, OUTPUT);
    digitalWrite(SCL, LOW);
#endif
#endif //PIN_WIRE1_SLEEP

#if defined WIO_TERMINAL
#pragma message ("Low Power for WIO_TERMINAL") 
  // in ordfder of variant.h 
  // LED
  // TX/RX - can be switched with ROLE
  // Digital and Analog Arduino pins
  // RPI BCM Connector - no action
  // FPC Connector - no action 
  // RPI Analog Iverlay - no action
  // USB - PIN_USB_HOST_ENABLE (for power)
  //Button_1 _2 _3 - cct pullup 4.7K  no action
  //SWITCH_X _Z _Y  _B _U pulled up 100K - no action

  // IRQ0  PC20 From RTL8720D - 
  pinMode(IRQ0,  INPUT_PULLUP); //?
  //Buzzer_CTR - Output control, Q4 driver, pulled down
  //pinMode(BUZZER_CTR,  INPUT_PULLDOWN);

  //MIC_INPUT - no action

  //GCLK - debug leave
  //Serial1 sercom2
  //Serial2 sercon1
  // I2C WIRE sercom3 - pulled up ext
  // I2C WIRE1 sercom4 -pulled up?
  //    GYROSCOPE - no external pull U5 LIS3DHTR PIN_WRE1_SCL _SDA 
  //  
  //pinMode(GYROSCOPE_INT1,  INPUT_PULLUP); // Push-Pull - no action 

  // micro SD socket 
  //SDCARD_SPI/SPI2  _SCK_PIN _SS_PIN _MOSI  _MISO _DET
  //  SDCARD_DET_PIN is pulled high with 100K
  pinMode(SDCARD_SS_PIN,  INPUT_PULLUP);
  pinMode(SDCARD_DET_PIN,  INPUT);

  //LCD
  //SPI LCD_SCK _CS  _MOSI_ MISOC  
  //    LCD_D/C 
  //    LCD_RESET  - Pulled hihg 4.7K
  //    LCD_BACKLIGHT=LOW  off

  pinMode(LCD_SS_PIN,   INPUT_PULLUP);
  pinMode(LCD_SCK_PIN,  INPUT_PULLDOWN);
  pinMode(LCD_MISO_PIN, INPUT_PULLDOWN);
  pinMode(LCD_MOSI_PIN, INPUT_PULLDOWN);
  pinMode(LCD_RESET,    INPUT);
  //Something causes to go from 6.0 to 6.5mA
  /*pinMode(LCD_DC,       INPUT_PULLDOWN); //??
  pinMode(LCD_XL,       INPUT_PULLDOWN);  
  pinMode(LCD_YU,       INPUT_PULLDOWN);  
  pinMode(LCD_XR,       INPUT_PULLDOWN);  
  pinMode(LCD_YD,       INPUT_PULLDOWN); */

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  //Turn off WiFi RTL8720D_CHIP_PU = LOW
 // pinMode(RTL8720D_CHIP_PU, OUTPUT);
 // digitalWrite(RTL8720D_CHIP_PU, LOW);
  // For power off, should other pins be low
  // RTL8720D_TXD _RXD
  // RTL8720D_SPI  _MISO_PIN _MOSI_PIN _SCK_PIN _SS_PIN  
  //RTL8720D_GPIO0    //low

  //QSPI  W25Q32JVZPIM
  // PIN_QSPI_CS _SCK _IO0  _IO1 _IO2 _IO3
  // PIN_QSPI_CS  should be high, 
  pinMode(PIN_QSPI_CS,  INPUT_PULLUP);
  // others are high impedance, so could pulled weakly low to hold them
  pinMode(PIN_QSPI_SCK, INPUT_PULLDOWN);
  pinMode(PIN_QSPI_IO0, INPUT_PULLDOWN);
  pinMode(PIN_QSPI_IO1, INPUT_PULLDOWN);
  pinMode(PIN_QSPI_IO2, INPUT_PULLDOWN);
  pinMode(PIN_QSPI_IO3, INPUT_PULLDOWN);

  //I2S sent to RPI - not used
  //Light sensor - input normally pulled low through 10K
  //ir sensors - output pulled low 

 //U11 ATECC608/DNP  I2C0_SCL _SDA 
 
//Power SW for RPI IO - outputs pulled low.
//OUTPUT_CTR_5V 
//OUTPUT_CTR_3V3

#endif //WIO_TERMINAL
    //Disable regular SysTick 
    SysTick->CTRL &= ~(SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk);

    //some possible further actions
    //tbd disable processor ADC
    //tbd turn off brown out detector
    //tbd 
    // wiring.c turn off un-needed peripherals
    // need int CLKS, CLK_APBAMASK_RTC 

    /* for time being leave on
    MCLK->APBAMASK.reg &= ~(MCLK_APBAMASK_SERCOM0 | MCLK_APBAMASK_SERCOM1 | MCLK_APBAMASK_TC0 | MCLK_APBAMASK_TC1);

    //Need 
    MCLK->APBBMASK.reg &= ~(MCLK_APBBMASK_SERCOM2 | MCLK_APBBMASK_SERCOM3 | MCLK_APBBMASK_TCC0 | MCLK_APBBMASK_TCC1 | MCLK_APBBMASK_TC3 | MCLK_APBBMASK_TC2);

                        // 0x2000 bit appearrs to be always on;
    MCLK->APBCMASK.reg &=  ~(MCLK_APBCMASK_TCC2 | MCLK_APBCMASK_TCC3 | MCLK_APBCMASK_TC4 | MCLK_APBCMASK_TC5 );

    MCLK->APBDMASK.reg &= ~(MCLK_APBDMASK_DAC | MCLK_APBDMASK_SERCOM4 | MCLK_APBDMASK_SERCOM5 | MCLK_APBDMASK_ADC0 | MCLK_APBDMASK_ADC1 | MCLK_APBDMASK_TCC4
            | MCLK_APBDMASK_TC6 | MCLK_APBDMASK_TC7 | MCLK_APBDMASK_SERCOM6 | MCLK_APBDMASK_SERCOM7);
    */
    // Turn off free running Cycle Count Register in DWT_CTRL
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;

} // lowpower_disable_ints

void lowpower_enable_ints(void) {
    //Re-enable clocks in the order that they are needed
    SysTick_Config( SystemCoreClock / 1000 );
    //Data Watchpoint and Trace Unit - 
    // Seperate core arm_cortexm4_processor_trm_100166_0001_00_en Technical Ref Manual.pdf
    // This is needed by protocols: OneWire
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    //tbd also need to look at enable what was disabled 
    // \framework-arduino-samd-seeed\cores\arduino\main.cpp init.cpp:init()
#if defined PIN_WIRE0_SLEEP
// Re-start the I2C interface
#ifdef PIN_WIRE_SDA
    pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
#endif
#ifdef PIN_WIRE_SCL
    pinMode(SCL, INPUT_PULLUP);
#endif
#endif // PIN_WIRE1_SLEEP
    Wire.begin();
    // Eliminate any potential extra waits in the wire library
    // These waits would be caused by a readBytes or parseX being called
    // on wire after the Wire buffer has emptied.  The default stream
    // functions - used by wire - wait a timeout period after reading the
    // end of the buffer to see if an interrupt puts something into the
    // buffer.  In the case of the Wire library, that will never happen and
    // the timeout period is a useless delay.
    Wire.setTimeout(0);    

#if defined USB_SERIALSTD 
    // Reattach the USB after waking - doest work
    //if (restoreUSBDevice) 
    {

#if defined(USE_TINYUSB)
        //??Adafruit_TinyUSB_Core_init();
        //??tinyusb_task();
        //USBDevice.attach();
#elif defined(USBCON)
        //USBDevice.init();
        //USBDevice.attach();
#endif //USE_TINYUSB
        #if 0
        uint32_t startTimer = millis();
        while (!SERIAL_PORT_USBVIRTUAL && ((millis() - startTimer) < 1000L)) {
            // wait
        }
        SerialStd.begin(serialBaudDebugDef);
        #endif //0
    }
#else  // USB_SERIALSTD 
    //This ensures that the ports are all setup as well
    STANDARD_SERIAL_OUTPUT.begin(serialBaudDebugDef);
#endif // USB_SERIALSTD 

}  // lowpower_enable_ints

#if 0
#define print_mc_status() print_act_status()

void print_rtc_time_field(uint32_t time_value) {
    //Serial.print("Time ");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.YEAR );
    SerialStd.print("/");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.MONTH );
    SerialStd.print("/");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.DAY );
    SerialStd.print(" ");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.HOUR );
    SerialStd.print(":");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.MINUTE );
    SerialStd.print(":");
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.bit.SECOND );        
 }
void print_act_status(void) {

    SerialStd.print("Alm ");
    //SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.reg,HEX);
    print_rtc_time_field(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.reg);
    SerialStd.print(" Match ");
    //SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].ALARM.reg,HEX);
    SerialStd.print(RTC->MODE2.Mode2Alarm[RTC_ALM_ID].MASK.bit.SEL ,HEX);

    SerialStd.print(" Ctl ");
    SerialStd.print(RTC->MODE2.CTRLA.reg ,HEX);
    SerialStd.print(" Mhz=");
    SerialStd.print(SystemCoreClock/1000000); 
    uint32_t nvicPriority= NVIC_GetPriorityGrouping();
    SerialStd.print(" NVIC ");
    SerialStd.println(nvicPriority);

#if 0 //defined USB_SERIALSTD
    SerialStd.print("Check actIRQ:");
    int intlp;
    for (intlp=0; intlp< PERIPH_COUNT_IRQn; intlp++)
    {
        if (NVIC_GetEnableIRQ((IRQn_Type)intlp)) {
            SerialStd.print(" ");
            SerialStdUSB.print(intlp);
        }
    }
    SerialStd.print(" TotChecked=");
    SerialStd.println(intlp);
    delay(100);
#endif //USB_SERIALSTD
 
}
#else 
#define print_mc_status() 
#endif 

// Use this to indicate a debug value
void flash_builtinLed(int count, int space_ms)
{
    for (int lpcnt = count; lpcnt > 0; lpcnt--)
    {
        digitalWrite(LED_BUILTIN, HIGH); // Show we're awake again
        delay(space_ms);
        digitalWrite(LED_BUILTIN, LOW);
        delay(space_ms);
    }
} // flash_redLed


void Logger::systemSleep(uint8_t sleep_min) { //SAMDx
    //For SAMDx using the built in USB  it simulates sleep. 
    //   So far haven't figure out how to restore USB after sleep
    // If it is built to use the Serial1 then it actually sleeps

    // Make sure interrupts are enabled for the clock
    NVIC_EnableIRQ(RTC_IRQn);       // enable RTC interrupt
    NVIC_SetPriority(RTC_IRQn, 0);  // highest priority

    // Alarms on the RTC built into the SAMD21/51 are set to every minute
    // The setting of the internal RTC alarm time that wakes up the processor
    // is set an intialization. 
    // An option is ARCH_SAMD_SET_RTC_EACH_ALARM but hasn't found to work
    uint32_t timeNow_secs = getNowUTCEpoch();
    uint32_t targetWakeup_secs;
    targetWakeup_secs = timeNow_secs + RTC_ALARM_SEC;
#if 0

    uint16_t local_secs;

    uint32_t adjust_secs;
    if (0 == sleep_min) {
        local_secs = (_loggingIntervalMinutes * 60);
    } else {
        local_secs = (sleep_min * 60);
    }

    targetWakeup_secs = timeNow_secs + local_secs;
    adjust_secs       = targetWakeup_secs % 60;
    targetWakeup_secs -= adjust_secs;
    MS_DBG("Setting alarm (", local_secs, "+", timeNow_secs, ") on RTC @",
           targetWakeup_secs, " ", formatDateTime_ISO8601(targetWakeup_secs),
           "\n  adj=", adjust_secs, " fm now=", timeNow_secs,
           " Awake=", timeNow_secs - wakeUpTime_secs);
#endif //0
    #if defined ARCH_SAMD_SET_RTC_EACH_ALARM
    DateTime timeNow = zero_sleep_rtc.now();   
    DateTime timeAlm = zero_sleep_rtc.alarm(RTC_ALM_ID);   
    PRINTOUT("  now   DDHHMMSS ",timeNow.day(),timeNow.hour(),timeNow.minute(),timeNow.second());
    PRINTOUT("  alarm DDHHMMSS ",timeAlm.day(),timeAlm.hour(),timeAlm.minute(),timeAlm.second());

    /*MS_DBG("Alm:", zr.getAlarmYear(), zr.getAlarmMonth(), zr.getAlarmDay(),
           "-", zr.getAlarmHours(), ":", zr.getAlarmMinutes(), ":",
           zr.getAlarmSeconds());*/
    // Assume max is an hour - need to revisit
    //zero_sleep_rtc.enableAlarm(RTC_ALM_ID,zero_sleep_rtc.MATCH_MMSS);
    #endif //ARCH_SAMD_SET_RTC_EACH_ALARM
    delay(100); //Debug output
    // Send one last message before shutting down serial ports
    PRINTOUT(F("Going to sleep. Ram("),freeRamCalcLb(),F("/"),freeRamCnt(),F(") SAM ZZzzz..."));
    print_mc_status();

    bool sleeping=true;
    // Disable the watch-dog timer
    watchDogTimer.disableWatchDog();

    // Sleep code from ArduinoLowPowerClass::sleep()

    //debug
    #if defined ARCH_SAMD_SET_RTC_EACH_ALARM
    timeNow_secs = zero_sleep_rtc.now().unixtime();
    PRINTOUT("Wake1 in ",targetWakeup_secs-timeNow_secs);
    delay(10);
    #endif // ARCH_SAMD_SET_RTC_EACH_ALARM

#if !defined USB_NOSLEEP 

    lowpower_disable_ints();
    // Now go to sleep
    //However in with USB attached may not truly sleep.

    //do {
    uint8_t rd_delay = 50;
#if 0// defined(MS_LOGGERBASE_DEBUG) || defined(MS_LOGGERBASE_SLEEP_DEBUG)
    // Maintens MS_DEBUGGING_STD output, but current is 13mA/SAMD51
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#else 
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
#endif

	#if defined(__SAMD51__) 
    #if defined WIO_TERMINAL
    // This works - with _STANDBY Serial1 doesn't recover
    #define MS_SLEEPCFG_MODE PM_SLEEPCFG_SLEEPMODE_IDLE2 
    //#define MS_SLEEPCFG_MODE PM_SLEEPCFG_SLEEPMODE_STANDBY
    #else
    // Adafruit Express M4
    #define MS_SLEEPCFG_MODE PM_SLEEPCFG_SLEEPMODE_STANDBY
    #endif //WIO_TERMINAL
    PM->SLEEPCFG.reg = (MS_SLEEPCFG_MODE & PM_SLEEPCFG_MASK );
    do {
        if (PM->SLEEPCFG.reg == (MS_SLEEPCFG_MODE & PM_SLEEPCFG_MASK )) break;
    } while (--rd_delay);  // Wait for it to take
	#else

    /* SAMD21 Slightly different 
    PM_SLEEPCFG_SLEEPMODE_STANDBY 3.3mA Can wake and run
    PM_SLEEPCFG_SLEEPMODE_HIBERNATE 3.1mA reset on int.watchdig
    PM_SLEEPCFG_SLEEPMODE_BACKUP 3.1mA requires recovery
    PM_SLEEPCFG_SLEEPMODE_OFF  2.9mA requires reset from ? watchdog*/
    #define MS_SLEEPCFG_MODE  PM_SLEEPCFG_SLEEPMODE_STANDBY
    // #define MS_SLEEPCFG_MODE PM_SLEEPCFG_SLEEPMODE_HIBERNATE
    // PM->STDBYCFG.FASTWKUP =0; default
    PM->SLEEPCFG.bit.SLEEPMODE = MS_SLEEPCFG_MODE;
    do {
        if (PM->SLEEPCFG.bit.SLEEPMODE != sleepMode_req) break;
    } while (--rd_delay);  // Wait for it to take
    #endif

    __DSB();
    __WFI();

    // ---------------------------------------------------------------------
    // -- The portion below this happens on wake up, after any wake ISR's --
    lowpower_enable_ints();

#else // USB_NOSLEEP
    //Simulate sleep until time expires
    sleeping=true;
    do {

        timeNow_secs = zero_sleep_rtc.now().unixtime();
        if (targetWakeup_secs <= timeNow_secs) 
        {
            sleeping =false;
        }  else {
            //SerialStd.print(" t="); //nh print here?
            //SerialStd.print((int32_t)targetWakeup_secs-(int32_t)timeNow_secs); //count 
            delay(200);
        }

    } while (sleeping);
#endif //USB_NOSLEEP

    // Re-enable the watch-dog timer
    watchDogTimer.enableWatchDog();

    // Wake-up message
    wakeUpTime_secs = getNowLocalEpoch();
    PRINTOUT(F("\n... zzzZZ SAM Awake @"), formatDateTime_ISO8601(wakeUpTime_secs)
#if defined ARDUINO_ARCH_SAMD  & defined ARCH_SAMD_SET_RTC_EACH_ALARM
    ,targetWakeup_secs, timeNow_secs
#endif //ARDUINO_ARCH_SAMD
    );
    flash_builtinLed(10,500); //Power measurement
    // The logger will now start the next function after the systemSleep
    // function in either the loop or setup
} //Logger::systemSleep SAMD
#endif //__AVR__
// ===================================================================== //
// Public functions for logging data to an SD card
// ===================================================================== //

// This sets a file name, if you want to decide on it in advance
void Logger::setFileName(String& fileName) {
    _fileName = fileName;
}
// Same as above, with a character array (overload function)
void Logger::setFileName(const char* fileName) {
    auto StrName = String(fileName);
    setFileName(StrName);
}


// This generates a file name from the logger id and the current date
// This will be used if the setFileName function is not called before
// the begin() function is called.
void Logger::generateAutoFileName(void) {
    // Generate the file name from logger ID and date
    auto fileName = String(_loggerID);
    fileName += "_";
    fileName += formatDateTime_ISO8601(getNowLocalEpoch()).substring(0, 10);
    fileName += ".csv";
    setFileName(fileName);
    _fileName = fileName;
}


/**
 * @brief This is a PRE-PROCESSOR MACRO to speed up generating header rows
 *
 * THIS IS NOT A FUNCTION, it is a pre-processor macro
 */
#define STREAM_CSV_ROW(firstCol, function)                       \
    stream->print("\"");                                         \
    stream->print(firstCol);                                     \
    stream->print("\",");                                        \
    for (uint8_t i = 0; i < getArrayVarCount(); i++) {           \
        stream->print("\"");                                     \
        stream->print(function);                                 \
        stream->print("\"");                                     \
        if (i + 1 != getArrayVarCount()) { stream->print(","); } \
    }                                                            \
    stream->println();

// This sends a file header out over an Arduino stream
void Logger::printFileHeader(Stream* stream) {
    // Very first line of the header is the logger ID
    stream->print(F("Data Logger: "));
    stream->println(_loggerID);

    // Next we're going to print the current file name
    stream->print(F("Data Logger File: "));
    stream->println(_fileName);

    printFileHeaderExtra(stream);

    // Adding the sampling feature UUID (only applies to EnviroDIY logger)
    if (strlen(_samplingFeatureUUID) > 1) {
        stream->print(F("Sampling Feature UUID: "));
        stream->print(_samplingFeatureUUID);
        stream->println(',');
    }

    // Next line will be the parent sensor names
    STREAM_CSV_ROW(F("Sensor Name:"), getParentSensorNameAtI(i))
    // Next comes the ODM2 variable name
    STREAM_CSV_ROW(F("Variable Name:"), getVarNameAtI(i))
    // Next comes the ODM2 unit name
    STREAM_CSV_ROW(F("Result Unit:"), getVarUnitAtI(i))
    // Next comes the variable UUIDs
    // We'll only add UUID's if we see a UUID for the first variable
    if (getVarUUIDAtI(0).length() > 1) {
        STREAM_CSV_ROW(F("Result UUID:"), getVarUUIDAtI(i))
    }

    // We'll finish up the the custom variable codes
    String dtRowHeader = F("Date and Time in UTC");
    if (_loggerTimeZone > 0) {
        dtRowHeader += '+' + _loggerTimeZone;
    } else if (_loggerTimeZone < 0) {
        dtRowHeader += _loggerTimeZone;
    }
    STREAM_CSV_ROW(dtRowHeader, getVarCodeAtI(i))
}


// This prints a comma separated list of volues of sensor data - including the
// time -  out over an Arduino stream
void Logger::printSensorDataCSV(Stream* stream) {
    String csvString = "";
    dtFromEpoch(Logger::markedLocalEpochTime).addToString(csvString);
    csvString += ',';
    stream->print(csvString);
    for (uint8_t i = 0; i < getArrayVarCount(); i++) {
        stream->print(getValueStringAtI(i));
        if (i + 1 != getArrayVarCount()) { stream->print(','); }
    }
    stream->println();
}

// Protected helper function - This checks if the SD card is available and ready
bool Logger::initializeSDCard(void) {
    bool retVal = true;
    // If we don't know the slave select of the sd card, we can't use it
    if (_SDCardSSPin < 0) {
        PRINTOUT(F("Slave/Chip select pin for SD card has not been set."));
        PRINTOUT(F("Data will not be saved!"));
        retVal = false;
    } else {
        // Initialise the SD card
        if (!sd1_card_fatfs.begin(_SDCardSSPin, SPI_FULL_SPEED)) {
            PRINTOUT(F("Error: SD card failed to initialize or is missing."));
            PRINTOUT(F("Data will not be saved!"));
            retVal = false;
        } else {
            // skip everything else if there's no SD card, otherwise it
            // might hang
            MS_DBG(F("Successfully connected to SD Card with card/slave "
                     "select "
                     "on pin"),
                   _SDCardSSPin);
        }
    }
    return SDextendedInit(retVal);
}


// Protected helper function - This sets a timestamp on a file
void Logger::setFileTimestamp(File fileToStamp, uint8_t stampFlag, bool localTime) {
    if (false == localTime) {
        fileToStamp.timestamp(stampFlag, dtFromEpoch(getNowLocalEpoch()).year(),
                            dtFromEpoch(getNowLocalEpoch()).month(),
                            dtFromEpoch(getNowLocalEpoch()).date(),
                            dtFromEpoch(getNowLocalEpoch()).hour(),
                            dtFromEpoch(getNowLocalEpoch()).minute(),
                            dtFromEpoch(getNowLocalEpoch()).second());
    }else {

        DateTime markedDtTz(getNowLocalEpoch()- EPOCH_TIME_DTCLASS );

        MS_DEEP_DBG(F("setFTTz"),markedDtTz.year(),markedDtTz.month(), markedDtTz.date(),
            markedDtTz.hour(), markedDtTz.minute(), markedDtTz.second());
        bool crStat = fileToStamp.timestamp(
            stampFlag, markedDtTz.year(), markedDtTz.month(), markedDtTz.date(),
            markedDtTz.hour(), markedDtTz.minute(), markedDtTz.second());
        if (!crStat) {
            PRINTOUT(F("setFTTz err for "), markedDtTz.year(), markedDtTz.month(),
                    markedDtTz.date(), markedDtTz.hour(), markedDtTz.minute(),
                    markedDtTz.second());
        }
    }
}


// Protected helper function - This opens or creates a file, converting a string
// file name to a character file name
bool Logger::openFile(String& filename, bool createFile,
                      bool writeDefaultHeader) {
    // Initialise the SD card
    // skip everything else if there's no SD card, otherwise it might hang
    if (!initializeSDCard()) return false;

    // Convert the string filename to a character file name for SdFat
    unsigned int fileNameLength = filename.length() + 1;
    char         charFileName[fileNameLength];
    filename.toCharArray(charFileName, fileNameLength);

    // First attempt to open an already existing file (in write mode), so we
    // don't try to re-create something that's already there.
    // This should also prevent the header from being written over and over
    // in the file.
    if (logFile.open(charFileName, O_WRITE | O_AT_END)) {
        MS_DBG(F("Opened existing file:"), filename);
        // Set access date time
        setFileTimestamp(logFile, T_ACCESS, true);
        return true;
    } else if (createFile) {
        // Create and then open the file in write mode
        if (logFile.open(charFileName, O_CREAT | O_WRITE | O_AT_END)) {
            MS_DBG(F("Created new file:"), filename);
            // Set creation date time
            setFileTimestamp(logFile, T_CREATE, true);
            // Write out a header, if requested
            if (writeDefaultHeader) {
                // Add header information
                printFileHeader(&logFile);
// Print out the header for debugging
#if defined(DEBUGGING_SERIAL_OUTPUT) && defined(MS_DEBUGGING_STD)
                MS_DBG(F("\n \\/---- File Header ----\\/"));
                printFileHeader(&DEBUGGING_SERIAL_OUTPUT);
                MS_DBG('\n');
#endif
                // Set write/modification date time
                setFileTimestamp(logFile, T_WRITE, true);
            }
            // Set access date time
            setFileTimestamp(logFile, T_ACCESS, true);
            return true;
        } else {
            // Return false if we couldn't create the file
            MS_DBG(F("Unable to create new file:"), filename);
            return false;
        }
    } else {
        // Return false if we couldn't access the file (and were not told to
        // create it)
        MS_DBG(F("Unable to to write to file:"), filename);
        return false;
    }
}


// These functions create a file on the SD card with the given filename and
// set the proper timestamps to the file.
// The filename may either be the one set by
// setFileName(String)/setFileName(void) or can be specified in the function. If
// specified, it will also write a header to the file based on the sensors in
// the group. This can be used to force a logger to create a file with a
// secondary file name.
bool Logger::createLogFile(String& filename, bool writeDefaultHeader) {
    // Attempt to create and open a file
    if (openFile(filename, true, writeDefaultHeader)) {
        // Close the file to save it (only do this if we'd opened it)
        logFile.close();
        PRINTOUT(F("Data will be saved as"), _fileName);
        return true;
    } else {
        PRINTOUT(F("Unable to create a file to save data to!"));
        return false;
    }
}
bool Logger::createLogFile(bool writeDefaultHeader) {
    if (_fileName == "") generateAutoFileName();
    return createLogFile(_fileName, writeDefaultHeader);
}


// These functions write a file on the SD card with the given filename and
// set the proper timestamps to the file.
// The filename may either be the one set by
// setFileName(String)/setFileName(void) or can be specified in the function. If
// the file does not already exist, the file will be created. This can be used
// to force a logger to write to a file with a secondary file name.
bool Logger::logToSD(String& filename, String& rec) {
    // First attempt to open the file without creating a new one
    if (!openFile(filename, false, false)) {
        PRINTOUT(F("Could not write to existing file on SD card, attempting to "
                   "create a file!"));
        // Next try to create the file, bail if we couldn't create it
        // This will not attempt to generate a new file name or add a header!
        if (!openFile(filename, true, false)) {
            PRINTOUT(F("Unable to write to SD card!"));
            return false;
        }
    }

    // If we could successfully open or create the file, write the data to it
    logFile.println(rec);
    // Echo the line to the serial port
    PRINTOUT(F("\n \\/---- Line Saved to"), filename, F("----\\/"));
    PRINTOUT(rec);

    // Set write/modification date time
    setFileTimestamp(logFile, T_WRITE, true);
    // Set access date time
    setFileTimestamp(logFile, T_ACCESS, true);
    // Close the file to save it
    logFile.close();
    return true;
}
bool Logger::logToSD(String& rec) {
    // Get a new file name if the name is blank
    if (_fileName == "") generateAutoFileName();
    return logToSD(_fileName, rec);
}
// NOTE:  This is structured differently than the version with a string input
// record.  This is to avoid the creation/passing of very long strings.
bool Logger::logToSD(void) {
    // Get a new file name if the name is blank
    if (_fileName == "") generateAutoFileName();

    // First attempt to open the file without creating a new one
    if (!openFile(_fileName, false, false)) {
        // Next try to create a new file, bail if we couldn't create it
        // Generate a filename with the current date, if the file name isn't set
        if (_fileName == "") generateAutoFileName();
        // Do add a default header to the new file!
        if (!openFile(_fileName, true, true)) {
            PRINTOUT(F("Unable to write to SD card!"));
            return false;
        }
    }

    // Write the data
    printSensorDataCSV(&logFile);
// Echo the line to the serial port
#if defined(STANDARD_SERIAL_OUTPUT)
    PRINTOUT(F("\n \\/---- Line Saved to"), _fileName, F("----\\/"));
    printSensorDataCSV(&STANDARD_SERIAL_OUTPUT);
    PRINTOUT('\n');
#endif

    // Set write/modification date time
    setFileTimestamp(logFile, T_WRITE, true);
    // Set access date time
    setFileTimestamp(logFile, T_ACCESS, true);
    // Close the file to save it
    logFile.close();
    return true;
}


// ===================================================================== //
// Public functions for a "sensor testing" mode
// ===================================================================== //
// A static function if you'd prefer to enter testing based on an interrupt
void Logger::testingISR() {
    MS_DEEP_DBG(F("Testing interrupt!"));
    if (!Logger::isTestingNow && !Logger::isLoggingNow) {
        Logger::startTesting = true;
        MS_DEEP_DBG(F("Testing flag has been set."));
    }
}


// This defines what to do in the testing mode
void Logger::testingMode() {
    // Flag to notify that we're in testing mode
    Logger::isTestingNow = true;
    // Unset the startTesting flag
    Logger::startTesting = false;

    PRINTOUT(F("------------------------------------------"));
    PRINTOUT(F("Entering sensor testing mode"));
    delay(100);  // This seems to prevent crashes, no clue why ....

    // Get the modem ready

    bool gotInternetConnection = false;
    if (_logModem != nullptr) {
        MS_DBG(F("Waking up"), _logModem->getModemName(), F("..."));
        if (_logModem->modemWake()) {
            // Connect to the network
            watchDogTimer.resetWatchDog();
            MS_DBG(F("Connecting to the Internet..."));
            if (_logModem->connectInternet()) {
                gotInternetConnection = true;
                // Publish data to remotes
                watchDogTimer.resetWatchDog();
            }
        }
    }

    // Power up all of the sensors
    _internalArray->sensorsPowerUp();

    // Wake up all of the sensors
    _internalArray->sensorsWake();

    // Update the sensors and print out data 25 times
    for (uint8_t i = 0; i < 25; i++) {
        PRINTOUT(F("------------------------------------------"));

        // Update the modem metadata
        // NOTE:  the extra get signal quality is an annoying redundancy
        // needed only for the wifi XBee.  Update metadata will also ask the
        // module for current signal quality using the underlying TinyGSM
        // getSignalQuality() function, but for the WiFi XBee it will not
        // actually measure anything except by explicitly making a connection,
        // which getModemSignalQuality() does.  For all of the other modules,
        // getModemSignalQuality() is just a straight pass-through to
        // getSignalQuality().
        if (gotInternetConnection) { _logModem->updateModemMetadata(); }

        watchDogTimer.resetWatchDog();
        // Update the values from all attached sensors
        // NOTE:  NOT using complete update because we want the sensors to be
        // left on between iterations in testing mode.
        _internalArray->updateAllSensors();
        // Print out the current logger time
        PRINTOUT(F("Current logger time is"),
                 formatDateTime_ISO8601(getNowLocalEpoch()));
        PRINTOUT(F("-----------------------"));
// Print out the sensor data
#if defined(STANDARD_SERIAL_OUTPUT)
        _internalArray->printSensorData(&STANDARD_SERIAL_OUTPUT);
#endif
        PRINTOUT(F("-----------------------"));
        watchDogTimer.resetWatchDog();

        delay(5000);
        watchDogTimer.resetWatchDog();
    }

    // Put sensors to sleep
    _internalArray->sensorsSleep();
    _internalArray->sensorsPowerDown();

    // Turn the modem off
    if (_logModem != nullptr) {
        if (gotInternetConnection) { _logModem->disconnectInternet(); }
        _logModem->modemSleepPowerDown();
    }

    PRINTOUT(F("Exiting testing mode"));
    PRINTOUT(F("------------------------------------------"));
    watchDogTimer.resetWatchDog();

    // Unset testing mode flag
    Logger::isTestingNow = false;

    // Sleep
    systemSleep();
}


// ===================================================================== //
// Convience functions to call several of the above functions
// ===================================================================== //

// This does all of the setup that can't happen in the constructors
// That is, things that require the actual processor/MCU to do something
// rather than the compiler to do something.
void Logger::begin(const char* loggerID, uint16_t loggingIntervalMinutes,
                   VariableArray* inputArray) {
    setLoggerID(loggerID);
    setLoggingInterval(loggingIntervalMinutes);
    begin(inputArray);
}
void Logger::begin(VariableArray* inputArray) {
    setVariableArray(inputArray);
    begin();
}

#if defined ARDUINO_ARCH_SAMD
// This needs to be invoked to enable interrupt processing
bool alarmUpdate_sema=false;
void alarmMatch(uint32_t flag)
{
    //Need the handler for RTC_SAMD51 interrupt handling
    alarmUpdate_sema= true;
    // This is intterrupt level, so becautious
#if defined DBG_LOGGERBASE_ALARM_MATCH_PRINT
    Serial.print("Alarm Match! ");
    DateTime now = zero_sleep_rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
 #endif // DBG_LOGGERBASE_ALARM_MATCH_PRINT
}
#endif // ARDUINO_ARCH_SAMD

void Logger::begin() {
    MS_DBG(F("Logger ID is:"), _loggerID);
    MS_DBG(F("Logger is set to record at"), _loggingIntervalMinutes,
           F("minute intervals."));

    MS_DBG(F(
        "Setting up a watch-dog timer to fire after 5minutes after loggingInterval"),_loggingIntervalMinutes);
    watchDogTimer.setupWatchDog(((uint32_t)_loggingIntervalMinutes+5)*60);
    // Enable the watchdog
    watchDogTimer.enableWatchDog();

#if defined ARDUINO_ARCH_SAMD
    MS_DBG(F("Beginning internal real time clock"));
    zero_sleep_rtc.begin(); //Soft start, preserve good time
#endif
    watchDogTimer.resetWatchDog();

    // Set the pins for I2C
    MS_DBG(F("Setting I2C Pins to INPUT_PULLUP"));
#ifdef SDA
    pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
#endif
#ifdef SCL
    pinMode(SCL, INPUT_PULLUP);
#endif
    MS_DBG(F("Beginning wire (I2C)"));
    Wire.begin();
    watchDogTimer.resetWatchDog();

    // Eliminate any potential extra waits in the wire library
    // These waits would be caused by a readBytes or parseX being called
    // on wire after the Wire buffer has emptied.  The default stream
    // functions - used by wire - wait a timeout period after reading the
    // end of the buffer to see if an interrupt puts something into the
    // buffer.  In the case of the Wire library, that will never happen and
    // the timeout period is a useless delay.
    Wire.setTimeout(0);

    // Set all of the pin modes
    // NOTE:  This must be done here at run time not at compile time
    setLoggerPins(_mcuWakePin, _SDCardSSPin, _SDCardPowerPin, _buttonPin,
                  _ledPin);

#if defined(MS_SAMD_DS3231) || not defined(ARDUINO_ARCH_SAMD)
    MS_DBG(F("Beginning DS3231 real time clock"));
    rtcExtPhy.begin();
#endif
    watchDogTimer.resetWatchDog();

    // Reset the watchdog
    watchDogTimer.resetWatchDog();

#if defined ARDUINO_ARCH_SAMD
    /* Internal RTCZero Mode3       class Time relative to 2000 UTC
     * External RTC_PCF8523/PCF2127 class Time relative to 2000 UTC/GMT/TZ0
     * Seconds, Minutes 0-59,  Hours 0-23,  Days 1-31, Months 1-12, Years
     * 0-99
     */

    // eg Apr 22 2019 16:46:09 in this TZ
    DateTime ccTimeTZ(__DATE__, __TIME__); //base is Y2K, set local Time
    DateTime ccTimeUTC(((uint32_t)ccTimeTZ.unixtime()) -
                       ((int32_t)getLoggerTimeZone() *
                        HOURS_TO_SECS));  // set to secs from UST/GMT Year 2000
#define COMPILE_TIME_UTC ((uint32_t)ccTimeUTC.unixtime() - (24 * HOURS_TO_SECS))
#define TIME_FUT_UPPER_UTC (COMPILE_TIME_UTC + 50 * 365 * 24 * 60 * 60)
    // MS_DBG("Sw Build Time Tz:
    // ",ccTimeTZ.year(),"/",ccTimeTZ.month(),"/",ccTimeTZ.date(),"
    // ",ccTimeTZ.hour(),":",ccTimeTZ.minute(),":",ccTimeTZ.second(), "
    // secs2kTz
    // ",ccTimeTZ.unixtime()); MS_DBG("Sw Build Time UTC:
    // ",ccTimeUTC.year(),"/",ccTimeUTC.month(),"/",ccTimeUTC.date(),"
    // ",ccTimeUTC.hour(),":",ccTimeUTC.minute(),":",ccTimeUTC.second(),"
    // secs2kUTC
    // ",ccTimeUTC.unixtime(),"Tz=",getTimeZone());

#if defined ADAFRUIT_FEATHERWING_RTC_SD || defined USE_RTCLIB
    MS_DBG("ExtRTC init");
    if (!rtcExtPhy.begin()) {
        PRINTOUT(F("*** extRTC not found. Equipment Error"));
        // reboot or ?
    } else {
        delay(100);
        bool cold_init = false;  // Simple test for time being - expand with
                                 // RAM signature
        USE_RTCLIB::ErrorNum errRtc;  // = rtcExtPhy.initialized();
#define RTC_INIT_MAX_NUM 10
        uint8_t init_counter = 0;
        do {
            errRtc = rtcExtPhy.initialized();
            if (USE_RTCLIB::NO_ERROR == errRtc) break;
            cold_init = true;  // As oscillator wasn't working
            MS_DBG(init_counter, "] ExtRTC !init. err=", errRtc,
                   " waiting for stability");
            delay(100);
        } while (++init_counter < RTC_INIT_MAX_NUM);

        if (cold_init) {
            MS_DBG("ExtRTC cold !init. set to compile time UTC ",
                   COMPILE_TIME_UTC, " which is Tz ", __DATE__, " ", __TIME__);
            rtcExtPhy.init();
            rtcExtPhy.adjust(ccTimeUTC);

        } else {
            DateTime rNow_dt    = rtcExtPhy.now();
            uint32_t rnow_usecs = rNow_dt.unixtime();

            MS_DBG("ExtRTC UTC ", rNow_dt.year(), "/", rNow_dt.month(), "/",
                   rNow_dt.date(), " ", rNow_dt.hour(), ":", rNow_dt.minute(),
                   ":", rNow_dt.second(), " or epoch ", rnow_usecs);
            MS_DBG("Good if between ", COMPILE_TIME_UTC, "<", rnow_usecs, "<",
                   TIME_FUT_UPPER_UTC);
            if ((rnow_usecs < COMPILE_TIME_UTC) ||
                (rnow_usecs > TIME_FUT_UPPER_UTC)) {
                rtcExtPhy.adjust(ccTimeUTC);
                MS_DBG("ExtRTC UTC set to compile time UTC ", COMPILE_TIME_UTC,
                       " which is Tz ", __DATE__, " ", __TIME__);
            }
        }

        watchDogTimer.resetWatchDog();

        DateTime now = rtcExtPhy.now();

        MS_DBG("Set internal rtc from ext rtc ", now.year(), "-", now.month(),
               "-", now.date(), " ", now.hour(), ":", now.minute(), ":",
               now.second());
        zero_sleep_rtc.setTime(now.hour(), now.minute(), now.second());
        zero_sleep_rtc.setDate(now.date(), now.month(), now.year() - 2000);
#define zr zero_sleep_rtc
        MS_DBG("Read internal rtc UTC ", 2000 + zr.getYear(), "-",
               zr.getMonth(), "-", zr.getDay(), " ", zr.getHours(), ":",
               zr.getMinutes(), ":", zr.getSeconds());
    }
#else  // no external _RTC
       // If Power-on Reset Rcause.Bit0 have
       // specific processing

    if (true)//((0 == zr.now().year()) && (1 == zr.now().month()) && (1 == zr.now().day())) 
    {
        // Assume Wio Terminal - init to DEFAULT
        MS_DBG("RTC set to CC time for Power-On Reset case ",__DATE__, __TIME__);
        PRINTOUT(("Def UTC :"), formatDateTime_ISO8601(ccTimeUTC));
        zr.adjust(ccTimeUTC); //UTC ref 2000 @ T0

    } else { MS_DBG("RTC already running ");}
#endif  // ADAFRUIT_FEATHERWING_RTC_SD
#endif  // ARDUINO_ARCH_SAMD

    // Print out the current time
    PRINTOUT(F("RTC valid range"), 
    formatDateTime_ISO8601(EPOCH_TIME_LOWER_SANITY_SECS),F("between"),
    formatDateTime_ISO8601(EPOCH_TIME_UPPER_SANITY_SECS));
    PRINTOUT(F("Current RTC time is:"),
             formatDateTime_ISO8601(getNowUTCEpoch()));
    PRINTOUT(F("Current localized logger time is:"),
             formatDateTime_ISO8601(getNowLocalEpoch()));

//#define ARCH_SAMD_SET_RTC_EACH_ALARM
#if defined ARDUINO_ARCH_SAMD & !defined ARCH_SAMD_SET_RTC_EACH_ALARM
    //set an alarm to go off every 1minute, keeps time accurate.
    DateTime now = zr.now();
    DateTime alarm = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), RTC_ALARM_SEC );
    // The SAMD51 has two hardware alarms

    zr.setAlarm(RTC_ALM_ID,alarm);
    zr.attachInterrupt(alarmMatch); //Need for internal RTC_SAMD51 processing
    zr.enableAlarm(RTC_ALM_ID, zr.MATCH_SS); // match Every minute 
    PRINTOUT(F("Set RTC Alarm Every Miunute")); 
    // This is needed by protocols: OneWire
    // DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;

#endif // ARDUINO_ARCH_SAMD

    // Reset the watchdog
    watchDogTimer.resetWatchDog();

    // Begin the internal array
    _internalArray->begin();
    PRINTOUT(F("This logger has a variable array with"), getArrayVarCount(),
             F("variables, of which"),
             getArrayVarCount() - _internalArray->getCalculatedVariableCount(),
             F("come from"), _internalArray->getSensorCount(), F("sensors and"),
             _internalArray->getCalculatedVariableCount(),
             F("are calculated."));

    if (_samplingFeatureUUID != nullptr) {
        PRINTOUT(F("Sampling feature UUID is:"), _samplingFeatureUUID);
    }

    PRINTOUT(F("Logger portion of setup finished.\n"));
}


// This is a one-and-done to log data
void Logger::logData(void) {
    // Reset the watchdog
    watchDogTimer.resetWatchDog();

    // Assuming we were woken up by the clock, check if the current time is an
    // even interval of the logging interval
    if (checkInterval()) {
        // Flag to notify that we're in already awake and logging a point
        Logger::isLoggingNow = true;
        // Reset the watchdog
        watchDogTimer.resetWatchDog();

        // Print a line to show new reading
        PRINTOUT(F("------------------------------------------"));
        // Turn on the LED to show we're taking a reading
        alertOn();
        // Power up the SD Card
        // TODO(SRGDamia1):  Decide how much delay is needed between turning on
        // the card and writing to it.  Could we turn it on just before writing?
        turnOnSDcard(false);

        // Do a complete sensor update
        MS_DBG(F("    Running a complete sensor update..."));
        watchDogTimer.resetWatchDog();
        _internalArray->completeUpdate();
        watchDogTimer.resetWatchDog();

        // Create a csv data record and save it to the log file
        logToSD();
        // Cut power from the SD card, waiting for housekeeping
        turnOffSDcard(true);

        // Turn off the LED
        alertOff();
        // Print a line to show reading ended
        PRINTOUT(F("---LogToSD Complete\n"));

        // Unset flag
        Logger::isLoggingNow = false;
    }

    // Check if it was instead the testing interrupt that woke us up
    if (Logger::startTesting) testingMode();

    // Sleep
    systemSleep();
}
// This is a one-and-done to log data
void Logger::logDataAndPublish(void) {
    // Reset the watchdog
    watchDogTimer.resetWatchDog();

    // Assuming we were woken up by the clock, check if the current time is an
    // even interval of the logging interval
    if (checkInterval()) {
        // Flag to notify that we're in already awake and logging a point
        Logger::isLoggingNow = true;
        // Reset the watchdog
        watchDogTimer.resetWatchDog();

        // Print a line to show new reading
        PRINTOUT(F("---Log & Post Readings Best Effort----------"));
        // Turn on the LED to show we're taking a reading
        alertOn();
        // Power up the SD Card
        // TODO(SRGDamia1):  Decide how much delay is needed between turning on
        // the card and writing to it.  Could we turn it on just before writing?
        turnOnSDcard(false);

        // Do a complete update on the variable array.
        // This this includes powering all of the sensors, getting updated
        // values, and turing them back off.
        // NOTE:  The wake function for each sensor should force sensor setup to
        // run if the sensor was not previously set up.
        MS_DBG(F("Running a complete sensor update..."));
        watchDogTimer.resetWatchDog();
        _internalArray->completeUpdate();
        watchDogTimer.resetWatchDog();

// Print out the sensor data
#if defined(STANDARD_SERIAL_OUTPUT)
        MS_DBG('\n');
        _internalArray->printSensorData(&STANDARD_SERIAL_OUTPUT);
        MS_DBG('\n');
#endif

        // Create a csv data record and save it to the log file
        logToSD();

        if (_logModem != nullptr) {
            MS_DBG(F("Waking up"), _logModem->getModemName(), F("..."));
            if (_logModem->modemWake()) {
                // Connect to the network
                watchDogTimer.resetWatchDog();
                PRINTOUT(F("Connecting to the Internet with"),_logModem->getModemName());
                if (_logModem->connectInternet()) {
                    // Publish data to remotes
                    watchDogTimer.resetWatchDog();
                    publishDataToRemotes();
                    watchDogTimer.resetWatchDog();

                    if ((Logger::markedLocalEpochTime != 0 &&
                         Logger::markedLocalEpochTime % 86400 == 43200) ||
                        !isRTCSane(Logger::markedLocalEpochTime)) {
                        // Sync the clock at noon
                        MS_DBG(F("Running a daily clock sync..."));
                        setRTClock(_logModem->getNISTTime());
                        watchDogTimer.resetWatchDog();
                    }

                    // Update the modem metadata
                    MS_DBG(F("Updating modem metadata..."));
                    _logModem->updateModemMetadata();

                    // Disconnect from the network
                    MS_DBG(F("Disconnecting from the Internet..."));
                    _logModem->disconnectInternet();
                } else {
                    PRINTOUT(F("Connect to the internet failed with"),_logModem->getModemName());
                    watchDogTimer.resetWatchDog();
                }
            } else {
                PRINTOUT(F("Failed to wake "), _logModem->getModemName());
            }
            // Turn the modem off
            _logModem->modemSleepPowerDown();
        }


        // Cut power from the SD card - without additional housekeeping wait
        // TODO(SRGDamia1):  Do some sort of verification that minimum 1 sec has
        // passed for internal SD card housekeeping before cutting power -
        // although it seems very unlikely based on my testing that less than
        // one second would be taken up in publishing data to remotes.
        turnOffSDcard(false);

        // Turn off the LED
        alertOff();
        // Print a line to show reading ended
        PRINTOUT(F("---Log & Post Readings End---------------"));

        // Unset flag
        Logger::isLoggingNow = false;
    }

    // Check if it was instead the testing interrupt that woke us up
    if (Logger::startTesting) testingMode();

    // Call the processor sleep
    systemSleep();
}

#include "LoggerBaseExtCpp.h"
#include "LoggerBaseSDcpp.h"
// End of LoggerBase.cpp
