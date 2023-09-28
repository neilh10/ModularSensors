/**
 * @file SIMComSIM7080.cpp
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the SIMComSIM7080 class.
 */

// Included Dependencies
#include "SIMComSIM7080.h"
#include "LoggerModemMacros.h"

// Constructor
SIMComSIM7080::SIMComSIM7080(Stream* modemStream, int8_t powerPin,
                             int8_t statusPin, int8_t modemSleepRqPin,
                             const char* apn)
    : loggerModem(powerPin, statusPin, SIM7080_STATUS_LEVEL, modemSleepRqPin,
                  SIM7080_RESET_LEVEL, SIM7080_RESET_PULSE_MS, modemSleepRqPin,
                  SIM7080_WAKE_LEVEL, SIM7080_WAKE_PULSE_MS,
                  SIM7080_STATUS_TIME_MS, SIM7080_DISCONNECT_TIME_MS,
                  SIM7080_WAKE_DELAY_MS, SIM7080_ATRESPONSE_TIME_MS),
#ifdef MS_SIMCOMSIM7080_DEBUG_DEEP
      _modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      gsmModem(_modemATDebugger),
#else
      gsmModem(*modemStream),
#endif
      gsmClient(gsmModem),
      _apn(apn) {
}
SIMComSIM7080::SIMComSIM7080(Stream* modemStream, int8_t powerPin,
                             int8_t statusPin, bool useCTSStatus,
                             int8_t modemResetPin, int8_t modemSleepRqPin)
    : loggerModem(powerPin, statusPin, SIM7080_STATUS_LEVEL, modemSleepRqPin,
                  SIM7080_RESET_LEVEL, SIM7080_RESET_PULSE_MS, modemSleepRqPin,
                  SIM7080_WAKE_LEVEL, SIM7080_WAKE_PULSE_MS,
                  SIM7080_STATUS_TIME_MS, SIM7080_DISCONNECT_TIME_MS,
                  SIM7080_WAKE_DELAY_MS, SIM7080_ATRESPONSE_TIME_MS),
#ifdef MS_SIMCOMSIM7080_DEBUG_DEEP
      _modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      gsmModem(_modemATDebugger),
#else
      gsmModem(*modemStream),
#endif
      gsmClient(gsmModem) {
    //apn needs setting;
}

// Destructor
SIMComSIM7080::~SIMComSIM7080() {}

MS_MODEM_EXTRA_SETUP(SIMComSIM7080);
MS_IS_MODEM_AWAKE(SIMComSIM7080);
MS_MODEM_WAKE(SIMComSIM7080);

MS_MODEM_CONNECT_INTERNET(SIMComSIM7080);
MS_MODEM_DISCONNECT_INTERNET(SIMComSIM7080);
MS_MODEM_IS_INTERNET_AVAILABLE(SIMComSIM7080);

MS_MODEM_GET_NIST_TIME(SIMComSIM7080);

MS_MODEM_GET_MODEM_SIGNAL_QUALITY(SIMComSIM7080);
MS_MODEM_GET_MODEM_BATTERY_DATA(SIMComSIM7080);
MS_MODEM_GET_MODEM_TEMPERATURE_DATA(SIMComSIM7080);

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool SIMComSIM7080::modemWakeFxn(void) {
    // Must power on and then pulse on
    if (_modemSleepRqPin >= 0) {
        MS_DBG(F("Sending a"), _wakePulse_ms, F("ms"),
               _wakeLevel ? F("HIGH") : F("LOW"), F("wake-up pulse on pin"),
               _modemSleepRqPin, F("for"), _modemName);
        digitalWrite(_modemSleepRqPin, _wakeLevel);
        delay(_wakePulse_ms);  // >1s
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        return gsmModem.waitResponse(30000L, GF("SMS Ready")) == 1;
    }
    return true;
}


bool SIMComSIM7080::modemSleepFxn(void) {
    if (_modemSleepRqPin >= 0) {
        // Must have access to `PWRKEY` pin to sleep
        // Easiest to just go to sleep with the AT command rather than using
        // pins
        MS_DBG(F("Asking SIM7080 to power down"));
        return gsmModem.poweroff();
    } else {  // DON'T go to sleep if we can't wake up!
        return true;
    }
}

// Az extensions
void SIMComSIM7080::setApn(const char* newAPN, bool copyId) {
    uint8_t newAPN_sz = strlen(newAPN);
    _apn              = newAPN;
    // TODO: njh test setAPN CopyID functons
    MS_DBG(F("\nsetAPN "),_apn);
    if (copyId) {
        /* Do size checks, allocate memory for the LoggerID, copy it there
         *  then set assignment.
         */
        // For cell phone note clear what max size is.
#define CELLAPN_MAX_sz 99
        if (newAPN_sz > CELLAPN_MAX_sz) {
            char* apn2 = (char*)newAPN;
            PRINTOUT(F("\n\r   LoggerModem:setAPN too long: Trimmed to "),
                     newAPN_sz);
            apn2[newAPN_sz] = 0;  // Trim max size
            newAPN_sz       = CELLAPN_MAX_sz;
        }
        if (NULL == _apn_buf) {
            _apn_buf = new char[newAPN_sz + 2];  // Allow for trailing 0
        } else {
            PRINTOUT(F("\nLoggerModem::setAPN error - expected NULL ptr"));
        }
        if (NULL == _apn_buf) {
            // Major problem
            PRINTOUT(F("\nLoggerModem::setAPN error -no buffer "), _apn_buf);
        } else {
            strcpy(_apn_buf, newAPN);
            _apn = _apn_buf;
        }
        MS_DBG(F("\nsetAPN cp "), _apn, " sz: ", newAPN_sz);
    }
}

String SIMComSIM7080::getApn(void) {
    return _apn;
}