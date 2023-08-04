/**
 * @file EspressifESP8266.cpp
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the EspressifESP8266 class.
 */

// Included Dependencies
#include "EspressifESP8266.h"
#include "LoggerModemMacros.h"

// Constructors
EspressifESP8266::EspressifESP8266(Stream* modemStream, int8_t powerPin,
                                   int8_t modemResetPin, const char* ssid,
                                   const char* pwd)
    : loggerModem(powerPin, -1, ESP8266_STATUS_LEVEL, modemResetPin,
                  ESP8266_RESET_LEVEL, ESP8266_RESET_PULSE_MS, -1,
                  ESP8266_WAKE_LEVEL, ESP8266_WAKE_PULSE_MS,
                  ESP8266_STATUS_TIME_MS, ESP8266_DISCONNECT_TIME_MS,
                  ESP8266_WAKE_DELAY_MS, ESP8266_ATRESPONSE_TIME_MS),
#ifdef MS_ESPRESSIFESP8266_DEBUG_DEEP
      _modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      gsmModem(_modemATDebugger),
#else
      gsmModem(*modemStream),
#endif
      gsmClient(gsmModem),
      _modemStream(modemStream),
      _ssid(ssid),
      _pwd(pwd) {
}

// Destructor
EspressifESP8266::~EspressifESP8266() {}

MS_IS_MODEM_AWAKE(EspressifESP8266);
MS_MODEM_WAKE(EspressifESP8266);

MS_MODEM_CONNECT_INTERNET(EspressifESP8266);
MS_MODEM_DISCONNECT_INTERNET(EspressifESP8266);
MS_MODEM_IS_INTERNET_AVAILABLE(EspressifESP8266);

MS_MODEM_GET_NIST_TIME(EspressifESP8266);

MS_MODEM_GET_MODEM_SIGNAL_QUALITY(EspressifESP8266);
MS_MODEM_GET_MODEM_BATTERY_DATA(EspressifESP8266);
MS_MODEM_GET_MODEM_TEMPERATURE_DATA(EspressifESP8266);

// A helper function to wait for the esp to boot and immediately change some
// settings We'll use this in the wake function
bool EspressifESP8266::ESPwaitForBoot(void) {
    // Wait for boot - finished when characters start coming
    // NOTE: After every "hard" reset (either power off or via RST-B), the ESP
    // sends out a boot log from the ROM on UART1 at 74880 baud.  We're not
    // going to worry about the odd baud rate since we're simply throwing the
    // characters away.
    MS_DBG(F("Waiting for boot-up message from ESP8266"));
    delay(200);  // It will take at least this long
    uint32_t start   = millis();
    bool     success = false;
    while (!_modemStream->available() && millis() - start < 1000) {
        // wait
    }
    if (_modemStream->available()) {
        success = true;
        // Read the boot log to empty it from the serial buffer
        while (_modemStream->available()) {
            _modemStream->read();
            delay(2);
        }
    }
    return success;
}

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool EspressifESP8266::modemWakeFxn(void) {
    bool success = true;
    if (_powerPin >= 0) {  // Turns on when power is applied
        uint8_t pwrState= digitalRead(_powerPin);
        MS_DBG(F("modemWakeFxn1"),_powerPin,pwrState,_modemSleepRqPin);
        //digitalWrite(_modemSleepRqPin, !_wakeLevel);
        digitalWrite(_powerPin, 1);
        delay(1000);
        success &= ESPwaitForBoot();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, _wakeLevel);
        }
        //return success;
    } else if (_modemResetPin >= 0) {
        MS_DBG(F("modemWakeFxn2 Sending a reset pulse to pin"), _modemResetPin,
               F("to wake ESP8266 from deep sleep"));
        digitalWrite(_modemResetPin, LOW);
        delay(_resetPulse_ms);
        digitalWrite(_modemResetPin, HIGH);
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        success &= ESPwaitForBoot();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, _wakeLevel);
        }
        //return success;
    } else if (_modemSleepRqPin >= 0) {
        MS_DBG(F("modemWakeFxn3 Setting pin"), _modemSleepRqPin,
               _wakeLevel ? F("HIGH") : F("LOW"),
               F("to wake ESP8266 from light sleep"));
        digitalWrite(_modemSleepRqPin, _wakeLevel);
        return success;
    } else {
         MS_DBG(F("modemWakeFxn4 NoOp"));
    }
    return success;
}

bool EspressifESP8266::modemSleepFxn(void) {
    // Use this if you have an MCU pin connected to the ESP's reset pin to wake
    // from deep sleep.  We'll also put it in deep sleep before yanking power.
    if (_modemResetPin >= 0 || _powerPin >= 0) {
        MS_DBG(F("Requesting deep sleep for ESP8266"));
        bool retVal = gsmModem.poweroff();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, !_wakeLevel);
        }
        return retVal;
    } else {  // DON'T go to sleep if we can't wake up!
        return true;
    }
}

// Set up the light-sleep status pin, if applicable
bool EspressifESP8266::extraModemSetup(void) {
    if (_modemSleepRqPin >= 0) { digitalWrite(_modemSleepRqPin, !_wakeLevel); }
    gsmModem.init();
    gsmClient.init(&gsmModem);
    _modemName = gsmModem.getModemName();

    // ?? if (gsmModem.commandMode()) {
    String modemInfo = gsmModem.getModemInfo();
    MS_DBG(F("ESP32-WROOM  extra Initializing"),_modemName, modemInfo);

    return true;
}

// Az extensions
void EspressifESP8266::setWiFiId(const char* newSsid, bool copyId) {
    uint8_t newSsid_sz = strlen(newSsid);
    _ssid              = newSsid;
    if (copyId) {
/* Do size checks, allocate memory for the LoggerID, copy it there
 *  then set assignment.
 */
#define WIFI_SSID_MAX_sz 32
        if (newSsid_sz > WIFI_SSID_MAX_sz) {
            char* WiFiId2 = (char*)newSsid;
            PRINTOUT(F("\n\r   LoggerModem:setWiFiId too long: Trimmed to "),
                     newSsid_sz);
            WiFiId2[newSsid_sz] = 0;  // Trim max size
            newSsid_sz          = WIFI_SSID_MAX_sz;
        }
        if (NULL == _ssid_buf) {
            _ssid_buf = new char[newSsid_sz + 2];  // Allow for trailing 0
        } else {
            PRINTOUT(F("\nLoggerModem::setWiFiId error - expected NULL ptr"));
        }
        if (NULL == _ssid_buf) {
            // Major problem
            PRINTOUT(F("\nLoggerModem::setWiFiId error -no buffer "),
                     _ssid_buf);
        } else {
            strcpy(_ssid_buf, newSsid);
            _ssid = _ssid_buf;
            //_ssid2 =  _ssid_buf;
        }
        MS_DBG(F("\nsetWiFiId cp "), _ssid, " sz: ", newSsid_sz);
    }
}

void EspressifESP8266::setWiFiPwd(const char* newPwd, bool copyId) {
    uint8_t newPwd_sz = strlen(newPwd);
    _pwd              = newPwd;

    if (copyId) {
/* Do size checks, allocate memory for the LoggerID, copy it there
 *  then set assignment.
 */
#define WIFI_PWD_MAX_sz 63  // Len 63 printable chars + 0
        if (newPwd_sz > WIFI_PWD_MAX_sz) {
            char* pwd2 = (char*)newPwd;
            PRINTOUT(F("\n\r   LoggerModem:setWiFiPwd too long: Trimmed to "),
                     newPwd_sz);
            pwd2[newPwd_sz] = 0;  // Trim max size
            newPwd_sz       = WIFI_PWD_MAX_sz;
        }
        if (NULL == _pwd_buf) {
            _pwd_buf = new char[newPwd_sz + 2];  // Allow for trailing 0
        } else {
            PRINTOUT(F("\nLoggerModem::setWiFiPwd error - expected NULL ptr"));
        }
        if (NULL == _pwd_buf) {
            // Major problem
            PRINTOUT(F("\nLoggerModem::setWiFiPwd error -no buffer "),
                     _pwd_buf);
        } else {
            strcpy(_pwd_buf, newPwd);
            _pwd = _pwd_buf;
        }
        MS_DEEP_DBG(F("\nsetWiFiPwd cp "), _ssid, " sz: ", newPwd_sz);
    }
}

String EspressifESP8266::getWiFiId(void) {
    return _ssid;
}
String EspressifESP8266::getWiFiPwd(void) {
    return _pwd;
}