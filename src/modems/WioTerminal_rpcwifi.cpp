/**
 * @file WioTerminal_rpcwifi.cpp
 * @copyright 2022  Neil Hancock & Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Neil Hancock https://github.com/neilh10/ModularSensors
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the WioTerminal_rpcwifi class.
 */
#if defined ARDUINO_ARCH_SAMD
// Included Dependencies
#include "WioTerminal_rpcwifi.h"
//#include "LoggerModemMacros.h" NOT used, uniquely created in this file 
#include <rpcWiFi.h>

// Constructor
WioTerminal_rpcwifi::WioTerminal_rpcwifi(/*Stream* modemStream,*/ 
                                   const char* ssid, const char* pwd
                                   )
    : loggerModem(-1, -1, ESP8266update_STATUS_LEVEL, -1,
                  ESP8266update_RESET_LEVEL, ESP8266update_RESET_PULSE_MS, -1,
                  ESP8266update_WAKE_LEVEL, ESP8266update_WAKE_PULSE_MS,
                  ESP8266update_STATUS_TIME_MS, ESP8266update_DISCONNECT_TIME_MS,
                  ESP8266update_WAKE_DELAY_MS, ESP8266update_ATRESPONSE_TIME_MS)
#ifdef MS_WIOTERMINAL_RPCWIFI_DEBUG_DEEP
      //_modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      //gsmModem(_modemATDebugger),
#else
      //gsmModem(*modemStream),
#endif
      //gsmClient(gsmModem) 
      {
    _ssid = ssid;
    _pwd  = pwd;

    //_espSleepRqPin = espSleepRqPin;
    //_espStatusPin  = espStatusPin;

    //_modemStream = modemStream;
}
WioTerminal_rpcwifi::WioTerminal_rpcwifi(/*Stream* modemStream,*/ int8_t powerPin,
                        int8_t statusPin, int8_t modemResetPin,int8_t modemSleepRqPin, 
                                   const char* ssid, const char* pwd, 
                                   int8_t espSleepRqPin, int8_t espStatusPin)
    : loggerModem(powerPin, statusPin, ESP8266update_STATUS_LEVEL, modemResetPin,
                  ESP8266update_RESET_LEVEL, ESP8266update_RESET_PULSE_MS, modemSleepRqPin,
                  ESP8266update_WAKE_LEVEL, ESP8266update_WAKE_PULSE_MS,
                  ESP8266update_STATUS_TIME_MS, ESP8266update_DISCONNECT_TIME_MS,
                  ESP8266update_WAKE_DELAY_MS, ESP8266update_ATRESPONSE_TIME_MS)
#ifdef MS_WIOTERMINAL_RPCWIFI_DEBUG_DEEP
      //_modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      //gsmModem(_modemATDebugger),
#else
      //gsmModem(*modemStream),
#endif
      //gsmClient(gsmModem) 
      {
    _ssid = ssid;
    _pwd  = pwd;

    _espSleepRqPin = espSleepRqPin;
    _espStatusPin  = espStatusPin;

    //_modemStream = modemStream;
}

// Destructor
WioTerminal_rpcwifi::~WioTerminal_rpcwifi() {}

/* the marcros are  not used, specific WioTerminal_rpcwifi
MS_IS_MODEM_AWAKE(WioTerminal_rpcwifi);
MS_MODEM_WAKE(WioTerminal_rpcwifi);

MS_MODEM_CONNECT_INTERNET(WioTerminal_rpcwifi);
MS_MODEM_DISCONNECT_INTERNET(WioTerminal_rpcwifi);
MS_MODEM_IS_INTERNET_AVAILABLE(WioTerminal_rpcwifi);

MS_MODEM_GET_NIST_TIME(WioTerminal_rpcwifi);

MS_MODEM_GET_MODEM_SIGNAL_QUALITY(WioTerminal_rpcwifi);
MS_MODEM_GET_MODEM_BATTERY_DATA(WioTerminal_rpcwifi);
MS_MODEM_GET_MODEM_TEMPERATURE_DATA(WioTerminal_rpcwifi);*/

// A helper function to wait for the esp to boot and immediately change some
// settings We'll use this in the wake function
bool WioTerminal_rpcwifi::RTLwaitForBoot(void) {
    // Wait for boot - finished when characters start coming
    // NOTE: After every "hard" reset (either power off or via RST-B), the ESP
    // sends out a boot log from the ROM on UART1 at 74880 baud.  We're not
    // going to worry about the odd baud rate since we're simply throwing the
    // characters away.
    MS_DBG(F("Waiting for boot-up message from RTL8720"));
    delay(200);  // It will take at least this long
    uint32_t start   = millis();
    bool     success = false;
    if (NULL == _modemStream) {return false;}
    while (!_modemStream->available() && ((millis() - start) < 1000) ) {}
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
bool WioTerminal_rpcwifi::modemWakeFxn(void) {
    bool success = true;
    if (_powerPin >= 0) {  // Turns on when power is applied
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        success &= RTLwaitForBoot();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, _wakeLevel);
        }
        return success;
    } else if (_modemResetPin >= 0) {
        MS_DBG(F("Sending a reset pulse to pin"), _modemResetPin,
               F("to wake ESP8266 from deep sleep"));
        digitalWrite(_modemResetPin, LOW);
        delay(_resetPulse_ms);
        digitalWrite(_modemResetPin, HIGH);
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        success &= RTLwaitForBoot();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, _wakeLevel);
        }
        return success;
    } else if (_modemSleepRqPin >= 0) {
        MS_DBG(F("Setting pin"), _modemSleepRqPin,
               _wakeLevel ? F("HIGH") : F("LOW"),
               F("to wake ESP8266 from light sleep"));
        digitalWrite(_modemSleepRqPin, _wakeLevel);
        return success;
    } else {
         MS_DBG(F("modemWakeFxn nop") );
        return true;
    }
}


bool WioTerminal_rpcwifi::modemSleepFxn(void) {
    // Use this if you have GPIO16 connected to the reset pin to wake from deep
    // sleep but no other MCU pin connected to the reset pin. NOTE:  This will
    // NOT work nicely with "testingMode"
    /*if (loggingInterval > 1)
    {
        uint32_t sleepSeconds = (((uint32_t)loggingInterval) * 60 * 1000) -
    75000L; String sleepCommand = String(sleepSeconds);
        gsmModem.sendAT(GF("+GSLP="), sleepCommand);
        // Power down for 1 minute less than logging interval
        // Better:  Calculate length of loop and power down for logging interval
    - loop time return gsmModem.waitResponse() == 1;
    }*/
    // Use this if you have an MCU pin connected to the ESP's reset pin to wake
    // from deep sleep We'll also put it in deep sleep before yanking power
    if (_modemResetPin >= 0 || _powerPin >= 0) {
        MS_DBG(F("Requesting deep sleep for ESP8266"));
       bool retVal = true;// gsmModem.poweroff();
        if (_modemSleepRqPin >= 0) {
            digitalWrite(_modemSleepRqPin, !_wakeLevel);
        }
        return retVal;
    } else if (_modemSleepRqPin >= 0 && _statusPin >= 0) {
        // Use this if you don't have access to the ESP8266's reset pin for deep
        // sleep but you do have access to another GPIO pin for light sleep.
        // This also sets up another pin to view the sleep status.
        // AT+WAKEUPGPIO=<enable>,<trigger_GPIO>,<trigger_level>[,<awake_GPIO>,<awake_level>]
        // <enable>
        //   1: ESP8266 can be woken up from light-sleep by GPIO.
        // <trigger_GPIO>
        //   Sets the GPIO to wake ESP8266 up; range of value: [0, 15].
        // <trigger_level>
        //   0: The GPIO wakes up ESP8266 on low level.
        // [<awake_GPIO>]
        //   Optional; this parameter is used to set a GPIO as a flag of
        //   ESP8266’s being awoken form Light-sleep; range of value: [0, 15].
        // [<awake_level>]
        //   Optional;
        //   0: The GPIO is set to be low level after the wakeup process.
        //   1: The GPIO is set to be high level after the wakeup process.
        // After being woken up by <trigger_GPIO> from Light-sleep, when the
        // ESP8266 attempts to sleep again, it will check the status of the
        // <trigger_GPIO>:
        // - if it is still in the wakeup status, the EP8266 will enter
        // Modem-sleep mode instead;
        // - if it is NOT in the wakeup status, the ESP8266 will enter
        // Light-sleep mode.
        MS_DBG(F("Setting pin"), _modemSleepRqPin,
               _statusLevel ? F("HIGH") : F("LOW"),
               F("to allow ESP8266 to enter light sleep"));
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        MS_DBG(F("Requesting light sleep for ESP8266 with status indication"));
        /*nh gsmModem.sendAT(GF("+WAKEUPGPIO=1,"), String(_espSleepRqPin), F(",0,"),
                        String(_espStatusPin), ',', _statusLevel);
        bool success = gsmModem.waitResponse() == 1;
        gsmModem.sendAT(GF("+SLEEP=1"));
        success &= gsmModem.waitResponse() == 1;*/
        delay(5);
        #define success true
        return success;
    } else if (_modemSleepRqPin >= 0 && _statusPin < 0) {
        // Light sleep without the status pin
        MS_DBG(F("Configuring light sleep for ESP8266"));
        /*nh gsmModem.sendAT(GF("+WAKEUPGPIO=1,"), String(_espSleepRqPin), F(",0"));
        bool success = gsmModem.waitResponse() == 1;
        gsmModem.sendAT(GF("+SLEEP=1"));
        success &= gsmModem.waitResponse() == 1;*/
        delay(5);
        MS_DBG(F("Setting pin"), _modemSleepRqPin,
               !_wakeLevel ? F("HIGH") : F("LOW"),
               F("to allow ESP8266 to enter light sleep"));
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        MS_DBG(F("Module MIGHT enter light sleep mode if it has been idle for "
                 "sufficient time."));
        return success;
    } else {  // DON'T go to sleep if we can't wake up!
        return true;
    }
}


// Set up the light-sleep status pin, if applicable
bool WioTerminal_rpcwifi::extraModemSetup(void) {
    //??if (_modemSleepRqPin >= 0) { digitalWrite(_modemSleepRqPin, !_wakeLevel); }
    /*nh gsmModem.init();
    gsmClient.init(&gsmModem);
    _modemName = gsmModem.getModemName();*/
    // // And make sure we're staying in station mode so sleep can happen
    // gsmModem.sendAT(GF("+CWMODE_DEF=1"));
    // gsmModem.waitResponse();
    // // Make sure that, at minimum, modem-sleep is on
    // gsmModem.sendAT(GF("+SLEEP=2"));
    // gsmModem.waitResponse();
    // // Set the wifi settings as default
    // // This will speed up connecting after resets
    // gsmModem.sendAT(GF("+CWJAP_DEF=\""), _ssid, GF("\",\""), _pwd, GF("\""));
    WiFi.disconnect(true);
    uint16_t wifi_times = 1;
    #define WIFI_CONNECTION_ATTEMPTS 5
    #define WIFI_DELAY_MS 500
    do {

        MS_DBG(_ssid, " connection #",wifi_times);
        WiFi.begin(_ssid, _pwd);
        if (++wifi_times > WIFI_CONNECTION_ATTEMPTS) {
            MS_DBG(_ssid, " failed");
            return false;}
        delay (WIFI_DELAY_MS);
    } while (WiFi.status() != WL_CONNECTED);

    MS_DBG(_ssid, " connected :",WiFi.localIP());
    // if (gsmModem.waitResponse(30000L, GFP(GSM_OK), GF(GSM_NL "FAIL" GSM_NL))
    // !=
    //     1) {
    //     gsmModem.sendAT(GF("+CWJAP=\""), _ssid, GF("\",\""), _pwd, GF("\""));
    //     if (gsmModem.waitResponse(30000L, GFP(GSM_OK),
    //                               GF(GSM_NL "FAIL" GSM_NL)) != 1) {
    //         return false;
    //     }
    // }
    // Slow down the baud rate for slow processors - and save the change to
    // the ESP's non-volatile memory so we don't have to do it every time
    // #if F_CPU == 8000000L
    // if (modemBaud > 57600)
    // {
    //     _modemSerial->begin(modemBaud);
    //     gsmModem.sendAT(GF("+UART_DEF=9600,8,1,0,0"));
    //     gsmModem.waitResponse();
    //     _modemSerial->end();
    //     _modemSerial->begin(9600);
    // }
    // #endif
    return true;
}


inline bool WioTerminal_rpcwifi::isInternetAvailable(void) {
    return WiFi.isConnected(); 
    }
//The udp library class
WiFiUDP udp;
uint32_t WioTerminal_rpcwifi::getNISTTime(void) {

    if (!isInternetAvailable()) {                                         \
        MS_DBG("No internet connection, cannot connect to NIST.");     \
        return 0;                                                         \
    }        
    MS_DBG("get NIST Time UDP ");
    // see getNPTtime udp https://wiki.seeedstudio.com/Connect-Wio-Terminal-to-Google-Cloud-IoT-Core/

       //initializes the UDP state
        //This initializes the transfer buffer
        udp.begin(WiFi.localIP(), localPort);

        sendNTPpacket(timeServer); // send an NTP packet to a time server
        // wait to see if a reply is available
        delay(1000);
        if (udp.parsePacket()) {
            Serial.println("udp packet received");
            Serial.println("");
            // We've received a packet, read the data from it
            udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

            //the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, extract the two words:

            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            unsigned long epoch = secsSince1900 - seventyYears;
#if defined ADJUST_TIME
            // adjust time for timezone offset in secs +/- from UTC
            // WA time offset from UTC is +8 hours (28,800 secs)
            // + East of GMT
            // - West of GMT
            long tzOffset = 28800UL;

            // WA local time 
            unsigned long adjustedTime;
            return adjustedTime = epoch + tzOffset;
#else
            return epoch;
#endif
        }
        else {
            // were not able to parse the udp packet successfully
            // clear down the udp connection
            udp.stop();
            return 0; // zero indicates a failure
        }
        // not calling ntp time frequently, stop releases resources
        udp.stop();


    return 0;
} //getNISTTime

// send an NTP request to the time server at the given address
unsigned long WioTerminal_rpcwifi::sendNTPpacket(const char* address) {
    // set all bytes in the buffer to 0
    for (int i = 0; i < NTP_PACKET_SIZE; ++i) {
        packetBuffer[i] = 0;
    }
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    return udp.endPacket();
} // sendNTPpacket


// Holders - need to be filled in
// for streams somewhere there may be caller to .write(const uint8_t *buf, size_t size)
bool WioTerminal_rpcwifi::modemWake(void) {
    MS_DBG(F("modemWake")); 
    modemWakeFxn();
    MS_DBG(F("RTL8720 Firmware Version:"),  rpc_system_version());
    return true;
}


bool WioTerminal_rpcwifi::connectInternet(uint32_t maxConnectionTime) {
    MS_DBG("connectInternet WiFi"); 
    WiFi.disconnect(true);
    Serial.println("Waiting for WIFI connection...");
    delay(500);
    //Initiate connection
    WiFi.begin(_ssid, _pwd);
#define CONNECT_RETRYS 20
    uint16_t connect_try=CONNECT_RETRYS;
    do {
        if (++connect_try > 20) {
            connect_try =0;
            WiFi.disconnect(true);
            delay(500);
            WiFi.begin(_ssid, _pwd);
            Serial.println("Retry");
        } else {
            Serial.print(".");
        }
        delay(200);
    } while (WiFi.status() != WL_CONNECTED) ;

    Serial.println("Connected.");
    printStatus();    
    return true;
    }
void  WioTerminal_rpcwifi::printStatus() {
    // print the SSID of the network you're attached to:
    Serial.println("");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println("");
}



void WioTerminal_rpcwifi::disconnectInternet(void) { MS_DBG("tbd need to disconnectInternet ");}

//WiFi.RSSI()) + "db";
bool  WioTerminal_rpcwifi::getModemSignalQuality(int16_t& rssi, int16_t& percent)
{
    MS_DBG("getModemSQ rpcWiFi"); 
    rssi = WiFi.RSSI();
    percent = 0;
    return true;
}

bool  WioTerminal_rpcwifi::getModemBatteryStats(uint8_t& chargeState, int8_t& percent,
                               uint16_t& milliVolts) 
{
    MS_DBG("getModemBatteryStats "); 
    chargeState =0;
    percent =88;
    milliVolts =3999;
    return true;
}

float WioTerminal_rpcwifi::getModemChipTemperature(void) {MS_DBG("getModemChipTemperature tbd"); return 0.0;}


bool WioTerminal_rpcwifi::isModemAwake(void) {MS_DBG("isModemWake tbd"); return true;}

#endif // ARDUINO_ARCH_SAMD