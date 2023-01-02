/**
 * @file DigiXBeeCellularTransparent.cpp
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * @author Greg Cutrell <gcutrell@limno.com>
 *
 * @brief Implements the DigiXBeeCellularTransparent class.
 */

// Included Dependencies
#include "DigiXBeeCellularTransparent.h"
#include "LoggerModemMacros.h"

// Constructor/Destructor
DigiXBeeCellularTransparent::DigiXBeeCellularTransparent(
    Stream* modemStream, int8_t powerPin, int8_t statusPin, bool useCTSStatus,
    int8_t modemResetPin, int8_t modemSleepRqPin, const char* apn,
    const char* user, const char* pwd)
    : DigiXBee(powerPin, statusPin, useCTSStatus, modemResetPin,
               modemSleepRqPin),
#ifdef MS_DIGIXBEECELLULARTRANSPARENT_DEBUG_DEEP
      _modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      gsmModem(_modemATDebugger, modemResetPin),
#else
      gsmModem(*modemStream, modemResetPin),
#endif
      gsmClient(gsmModem),
      _apn(apn),
      _user(user),
      _pwd(pwd) {
}

DigiXBeeCellularTransparent::DigiXBeeCellularTransparent(
    Stream* modemStream, int8_t powerPin, 
    int8_t statusPin, bool useCTSStatus,
    int8_t modemResetPin, int8_t modemSleepRqPin)
    : DigiXBee(powerPin, statusPin, useCTSStatus, modemResetPin,
               modemSleepRqPin),
#ifdef MS_DIGIXBEECELLULARTRANSPARENT_DEBUG_DEEP
      _modemATDebugger(*modemStream, DEEP_DEBUGGING_SERIAL_OUTPUT),
      gsmModem(_modemATDebugger, modemResetPin),
#else
      gsmModem(*modemStream, modemResetPin),
#endif
      gsmClient(gsmModem) {
    ///apn needs setting;
}

// Destructor
DigiXBeeCellularTransparent::~DigiXBeeCellularTransparent() {}

MS_IS_MODEM_AWAKE(DigiXBeeCellularTransparent);
MS_MODEM_WAKE(DigiXBeeCellularTransparent);

MS_MODEM_CONNECT_INTERNET(DigiXBeeCellularTransparent);
MS_MODEM_DISCONNECT_INTERNET(DigiXBeeCellularTransparent);
MS_MODEM_IS_INTERNET_AVAILABLE(DigiXBeeCellularTransparent);

MS_MODEM_GET_MODEM_SIGNAL_QUALITY(DigiXBeeCellularTransparent);
MS_MODEM_GET_MODEM_BATTERY_DATA(DigiXBeeCellularTransparent);
MS_MODEM_GET_MODEM_TEMPERATURE_DATA(DigiXBeeCellularTransparent);

// We turn off airplane mode in the wake.
bool DigiXBeeCellularTransparent::modemWakeFxn(void) {
    if (_modemSleepRqPin >= 0) {
        MS_DBG(F("Setting pin"), _modemSleepRqPin,
               _wakeLevel ? F("HIGH") : F("LOW"), F("to wake"), _modemName);
        digitalWrite(_modemSleepRqPin, _wakeLevel);
        return true;
    } else {
        // no wake pin, use airplane mode command 
        MS_DBG(F("Turning off airplane mode..."));
        if (gsmModem.commandMode()) {
            gsmModem.sendAT(GF("AM"), 0);
            gsmModem.waitResponse(TGWRIDT+0x01);
            // apply change
            gsmModem.sendAT(GF("AC"));
            gsmModem.waitResponse(TGWRIDT+0x01);
            gsmModem.exitCommand();
        }
        return true;
    }
}


// We turn on airplane mode in before sleep
bool DigiXBeeCellularTransparent::modemSleepFxn(void) {
    if (_modemSleepRqPin >= 0) {
        MS_DBG(F("Setting pin"), _modemSleepRqPin,
               !_wakeLevel ? F("HIGH") : F("LOW"), F("to put"), _modemName,
               F("to sleep"));
        digitalWrite(_modemSleepRqPin, !_wakeLevel);
        return true;
    } else {
        // no wake pin, use airplane mode command 
        MS_DBG(F("Turning on airplane mode..."));
        if (gsmModem.commandMode()) {
            gsmModem.sendAT(GF("AM"), 1);
            gsmModem.waitResponse(TGWRIDT+0x00);
            gsmModem.exitCommand();
        }
        return true;
    }
}


bool DigiXBeeCellularTransparent::extraModemSetup(void) {
    bool success = true;
    String ui_vers;
    /** First run the TinyGSM init() function for the XBee. */
    MS_DBG(F("Initializing the XBee..."));
    success &= gsmModem.init();
    gsmClient.init(&gsmModem);
    _modemName = gsmModem.getModemName();
    /** Then enter command mode to set pin outputs. */
    MS_DBG(F("Putting XBee into command mode..."));
    if (gsmModem.commandMode()) {
        //MS_DBG(F("Setting I/O Pins..."));
        gsmModem.getSeries();
        _modemName = gsmModem.getModemName();
        gsmModem.sendAT(F("IM"));  // Request Module Serial Number
        gsmModem.waitResponse(TGWRIDT+0x02,1000, _modemSerialNumber);
        // gsmModem.sendAT(F("S#"));  // Request Module ICCID

        gsmModem.sendAT(F("HV"));  // Request Module Hw Version
        gsmModem.waitResponse(TGWRIDT+0x03,1000, _modemHwVersion);
        gsmModem.sendAT(F("VR"));  // Firmware Version
        gsmModem.waitResponse(TGWRIDT+0x04,1000, _modemFwVersion);
        // gsmModem.sendAT(F("MR"));  // Firmware VersionModuleModem 
        PRINTOUT(F("Lte internet comms with"),_modemName, 
                 F("IMEI "), _modemSerialNumber,F("HwVer"),_modemHwVersion, F("FwVer"), _modemFwVersion);

        //MS_DBG(F("'"), _modemName, F("' in command mode. Setting I/O Pins..."));
        /** Enable pin sleep functionality on `DIO9`.
         * NOTE: Only the `DTR_N/SLEEP_RQ/DIO8` pin (9 on the bee socket) can be
         * used for this pin sleep/wake. */
        gsmModem.sendAT(GF("D8"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x05) == 1;
        /** Enable status indication on `DIO9` - it will be HIGH when the XBee
         * is awake.
         * NOTE: Only the `ON/SLEEP_N/DIO9` pin (13 on the bee socket) can be
         * used for direct status indication. */
        gsmModem.sendAT(GF("D9"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x06) == 1;
        /** Enable CTS on `DIO7` - it will be `LOW` when it is clear to send
         * data to the XBee.  This can be used as proxy for status indication if
         * that pin is not readable.
         * NOTE: Only the `CTS_N/DIO7` pin (12 on the bee socket) can be used
         * for CTS. */
        gsmModem.sendAT(GF("D7"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x07) == 1;
        /** Enable association indication on `DIO5` - this is should be directly
         * attached to an LED if possible.
         * - Solid light indicates no connection
         * - Single blink indicates connection
         * - double blink indicates connection but failed TCP link on last
         * attempt
         *
         * NOTE: Only the `Associate/DIO5` pin (15 on the bee socket) can be
         * used for this function. */
        gsmModem.sendAT(GF("D5"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x08) == 1;
        /** Enable RSSI PWM output on `DIO10` - this should be directly attached
         * to an LED if possible.  A higher PWM duty cycle (and thus brighter
         * LED) indicates better signal quality.
         * NOTE: Only the `DIO10/PWM0` pin (6 on the bee socket) can be used for
         * this function. */
        gsmModem.sendAT(GF("P0"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x09) == 1;
        /** Enable pin sleep on the XBee. */
        MS_DBG(F("Setting Sleep Options..."));
        gsmModem.sendAT(GF("SM"), 1);
        success &= gsmModem.waitResponse(TGWRIDT+0x0A) == 1;
        /** Disassociate from the network for the lowest power deep sleep. */
        gsmModem.sendAT(GF("SO"), 0);
        success &= gsmModem.waitResponse(TGWRIDT+0x0B) == 1;
        MS_DBG(F("Setting Other Options..."));
        /** Disable remote manager, USB Direct, and LTE PSM
         * NOTE:  LTE-M's PSM (Power Save Mode) sounds good, but there's no easy
         * way on the LTE-M Bee to wake the cell chip itself from PSM, so we'll
         * use the Digi pin sleep instead. */
        gsmModem.sendAT(GF("DO"), 0);
        success &= gsmModem.waitResponse(TGWRIDT+0x0C) == 1;
        /** Ask data to be "packetized" and sent out with every new line (0x0A)
         * character. */
        gsmModem.sendAT(GF("TD0A"));
        success &= gsmModem.waitResponse(TGWRIDT+0x0D) == 1;
        /* Make sure USB direct is NOT enabled on the XBee3 units. */
        gsmModem.sendAT(GF("P1"), 0);
        success &= gsmModem.waitResponse(TGWRIDT+0x0E) == 1;
        /** Set the socket timeout to 10s (this is default). */
        gsmModem.sendAT(GF("TM"), 64);
        success &= gsmModem.waitResponse(TGWRIDT+0x0F) == 1;
        // MS_DBG(F("Setting Cellular Carrier Options..."));
        // // Carrier Profile - 1 = No profile/SIM ICCID selected
        // gsmModem.sendAT(GF("CP"),0);
        // gsmModem.waitResponse(TGWRIDT+0x00);  // Don't check for success - only works on
        // LTE
        // // Cellular network technology - LTE-M/NB IoT
        // gsmModem.sendAT(GF("N#"),0);
        // gsmModem.waitResponse(TGWRIDT+0x00);  // Don't check for success - only works on
        // LTE
        MS_DBG(F("Setting the APN..."),_apn,_user?"u:!0":"",_pwd?"p!0":"");

        /** Save the network connection parameters. */
        success &= gsmModem.gprsConnect(_apn, _user, _pwd);
        MS_DBG(F("Ensuring XBee is in transparent mode..."));
        /* Make sure we're really in transparent mode. */
        gsmModem.sendAT(GF("AP0"));
        success &= gsmModem.waitResponse(TGWRIDT+0x10) == 1;
        /** Write all changes to flash and apply them. */
        MS_DBG(F("Applying changes..."));
        gsmModem.writeChanges();

#if defined MS_DIGIXBEECELLULARTRANSPARENT_DEBUG
        ui_vers = gsmModem.getIMEI();
        PRINTOUT(F("IM "), ui_vers);
        ui_vers = gsmModem.getSimCCID();
        PRINTOUT(F("CCID "), ui_vers);
        gsmModem.sendAT(GF("%V"));  // Get Voltage requires ver 11415
        ui_vers = gsmModem.readResponseInt(2000L);
        PRINTOUT(F("ModemV '"), ui_vers, "'");
        gsmModem.sendAT(GF("%L"));  // Low Voltage Threshold shutoff
        ui_vers = gsmModem.readResponseInt(2000L);
        PRINTOUT(F("LowV Threshold '"), ui_vers, "'");
        ui_vers = gsmModem.sendATGetString(GF("VR"));
        // ui_vers += " "+gsmModem.sendATGetString(F("VL"));
        MS_DBG(F("Version "), ui_vers);
#endif
        uint16_t loops = 0;
        int16_t  ui_db=0;
        uint8_t  status;
        String   ui_op;
        bool     cellRegistered = false;
        PRINTOUT(F("Loop=Sec] rx db : Status ' Operator ' #Polled Cell Status "
                   "every 1sec"));
        uint8_t reg_count = 1;
        for (unsigned long start = millis(); millis() - start < 300000;
             ++loops) {
            //ui_db =  gsmModem.getSignalQuality();
            //Read the uncached cell tower signal strength in hex
            //gsmModem.sendAT(GF("DB"), 0);
            //ui_db = gsmModem.readResponseInt(10000L);
            gsmModem.sendAT(GF("AI"));
            status = gsmModem.readResponseInt(10000L);
            ui_op  = String(loops) + "=" + String((float)millis() / 1000) +
                "] " + String(ui_db) + ":0x" + String(status, HEX) + " '" +
                gsmModem.getOperator() + "'";
            if ((0 == status) || (0x23 == status)) {
                ui_op += " Cnt=" + String(reg_count);
                PRINTOUT(ui_op);
                if (++reg_count > 3) {
                    cellRegistered = true;
                    break;
                }
            } else {
                reg_count = 1;
                // String ui_scan = gsmModem.sendATGetString(GF("AS")); //Scan
                // ui_op += " Cell Scan "+ui_scan;
                PRINTOUT(ui_op);
                if (10 == loops) {
                    /*Not clear why this may force a registration
                    Early experience with Hologram SIMs was they aren't
                    registering, However throwing this in, might do something or
                    maybe just coincedence that it started working after this
                    */
//                    PRINTOUT(F("Try +CREG '"));
//                    gsmModem.sendAT(GF("+CREG"));
//                    status = gsmModem.readResponseInt(10000L);
                    // String ui_creg=gsmModem.readResponseInt(10000L);
                    // PRINTOUT(F("UseRandom +CREG '"), ui_creg,"'");
                }
            }
            delay(1000);
        }
        if (cellRegistered) {
            ui_vers = gsmModem.getRegistrationStatus();
            PRINTOUT(F("Digi Xbee3 setup Sucess. Registration '"), ui_vers,
                     "'");
#if defined MS_DIGIXBEECELLULARTRANSPARENT_SHOW_NETWORK
            //Doesn't work reliably, shows a number for operator
            // AN (access point name) 
            // FC (Frequency Channel Number)
            // AS (Active scan for network environment data) doesn't seem to work Cell
            String ui_scan = gsmModem.sendATGetString(GF("AS")); //Scan
            PRINTOUT(F("Cell scan '"),ui_scan,"' success",success);
            delay(1000); //Allow for scanning
            int8_t scanLp=6;
            int16_t ui_len;
            while(--scanLp) {
                ui_scan=gsmModem.readResponseInt(1000L);
                ui_len=ui_scan.length();
                PRINTOUT(scanLp,F(":"),ui_len,F("["),ui_scan,F("]"));
                if (1 < ui_scan.length() ) break;
            }
#endif
            MS_DBG(F("Get IP number"));
            String xbeeRsp;
            bool   AllocatedIpSuccess = false;
// Checkfor IP allocation
#define MDM_IP_STR_MIN_LEN 7
#define MDM_LP_IPMAX 24
            gsmModem.sendAT(F("MY"));  // Request IP #
            gsmModem.waitResponse(TGWRIDT+0x11,1000, xbeeRsp);
            MS_DBG(F("Flush rsp "), xbeeRsp);
            for (int mdm_lp = 1; mdm_lp <= MDM_LP_IPMAX; mdm_lp++) {
                xbeeRsp = "";
                delay(mdm_lp * 500);
                gsmModem.sendAT(F("MY"));  // Request IP #
                gsmModem.waitResponse(TGWRIDT+0x12,1000, xbeeRsp);
                PRINTOUT(F("mdmIP["), mdm_lp, "/", MDM_LP_IPMAX, F("] '"),
                         xbeeRsp, "'=", xbeeRsp.length());
                if (0 != xbeeRsp.compareTo("0.0.0.0") &&
                    (xbeeRsp.length() > MDM_IP_STR_MIN_LEN)) {
                    AllocatedIpSuccess = true;
                    break;
                }
            }
            if (!AllocatedIpSuccess) {
                PRINTOUT(
                    F("XbeeLTE not received IP# -hope it works next time"));
                // delay(1000);
                // NVIC_SystemReset();
                success = false;
            } else {
                // success &= AllocatedIpSuccess;
                PRINTOUT(F("XbeeWLTE IP# ["), xbeeRsp, F("]"));
                /*

                xbeeRsp = "";
                // Display DNS allocation
                bool DnsIpSuccess = false;
                uint8_t index              = 0;
#define MDM_LP_DNSMAX 11
                for (int mdm_lp = 1; mdm_lp <= MDM_LP_DNSMAX; mdm_lp++) {
                    delay(mdm_lp * 500);
                    gsmModem.sendAT(F("NS"));  // Request DNS #
                    index &= gsmModem.waitResponse(TGWRIDT+0x00,1000, xbeeRsp);
                    MS_DBG(F("mdmDNS["), mdm_lp, "/", MDM_LP_DNSMAX, F("] '"),
                           xbeeRsp, "'");
                    if (0 != xbeeRsp.compareTo("0.0.0.0") &&
                        (xbeeRsp.length() > MDM_IP_STR_MIN_LEN)) {
                        DnsIpSuccess = true;
                        break;
                    }
                    xbeeRsp = "";
                }

                if (false == DnsIpSuccess) {
                    success = false;
                    PRINTOUT(F(
                        "XbeeWifi init test FAILED - hope it works next time"));
                } else {
                    PRINTOUT(F("XbeeWifi init test PASSED"));
                }

                */
            }
            success = true;  // Not sure why need to force this
        } else {
            success = false;
        }

        // uint32_t time_epoch_sec=getNISTTime();//getTimeCellTower();
        // Logger::setRTClock(time_epoch_sec); ////#include "LoggerBase.h"
        // PRINTOUT(F("Startup Time & thrownaway "),time_epoch_sec,"sec" ); /**/

        /** Finally, exit command mode. */
        gsmModem.exitCommand();
    } else {
        success = false;
        PRINTOUT(F("Digi Xbee3 setup failed!"));
    }
    return success;
}


// Get the time from NIST via TIME protocol (rfc868)
// This would be much more efficient if done over UDP, but I'm doing it
// over TCP because I don't have a UDP library for all the modems.
/*uint32_t DigiXBeeCellularTransparent::getNISTTime(void)
{
    // bail if not connected to the internet
    gsmModem.commandMode();
    if (!gsmModem.isNetworkConnected())
    {
        MS_DBG(F("No internet connection, cannot connect to NIST."));
        gsmModem.exitCommand();
        return 0;
    }

    // We can get the NIST timestamp directly from the XBee
    gsmModem.sendAT(GF("DT0"));
    String res = gsmModem.readResponseString();
    gsmModem.exitCommand();
    MS_DBG(F("Raw hex response from XBee:"), res);
    char buf[9] = {0,};
    res.toCharArray(buf, 9);
    uint32_t secFrom2000 = strtol(buf, 0, 16);
    MS_DBG(F("Seconds from Jan 1, 2000 from XBee (UTC):"), secFrom2000);

    // Convert from seconds since Jan 1, 2000 to 1970
    uint32_t unixTimeStamp = secFrom2000 + 946684800 ;
    MS_DBG(F("Unix Timestamp returned by NIST (UTC):"), unixTimeStamp);

    // If before Jan 1, 2019 or after Jan 1, 2030, most likely an error
    if (unixTimeStamp < 1546300800)
    {
        return 0;
    }
    else if (unixTimeStamp > 1893456000)
    {
        return 0;
    }
    else
    {
        return unixTimeStamp;
    }
}*/
#define timeHost "time.nist.gov"
#define TIME_PROTOCOL_PORT 37

#if !defined NIST_SERVER_RETRYS
#define NIST_SERVER_RETRYS 10
#endif  // NIST_SERVER_RETRYS

uint32_t DigiXBeeCellularTransparent::getNISTTime(void) {
    /* bail if not connected to the internet */
    if (!isInternetAvailable()) {
        PRINTOUT(F("No internet connection, cannot connect to NIST."));
        return 0;
    }

    for (uint8_t i = 0; i < NIST_SERVER_RETRYS; i++) {
        /* Must ensure that we do not ping the daylight more than once every 4 seconds
        * NIST clearly specifies here that this is a requirement for all software 
        * that accesses its servers:  https://tf.nist.gov/tf-cgi/servers.cgi 
        * Uses "TIME" protocol on port 37 NIST: This protocol is very expensive in
        * terms of network bandwidth, since it uses the complete tcp machinery to
        * transmit only 32 bits of data. Users are *strongly* encouraged to upgrade to
        * the network time protocol (NTP), which is both more accurate and more
        * robust.UDP required, and UDP not supported by TinyGSM */

        while (millis() < _lastNISTrequest + 4000) {}

        /* Make TCP connection */
        MS_DBG(F("\nConnecting to NIST daytime Server @"), millis());
        bool connectionMade = false;

        /* This is the IP address of time-e-wwv.nist.gov  */
        /* If it fails, options here https://tf.nist.gov/tf-cgi/servers.cgi */
        /* Uses "TIME" protocol on port 37 NIST: This protocol is expensive,
          since it uses the complete tcp machinery to transmit only 32 bits of
          data. FUTURE Users are *strongly* encouraged to upgrade to the network
          time protocol (NTP), which is both more accurate and more robust.*/
        PRINTOUT(i, F("] Connect "), timeHost);
        connectionMade = gsmClient.connect(timeHost, TIME_PROTOCOL_PORT, 15);

        /* Wait up to 5 seconds for a response */
        if (connectionMade) {
            /* Wait so port can be opened! */
            delay((i + 1) * 100L);
            uint32_t start = millis();
            /* Try sending something to ensure connection */
            gsmClient.println('!');

            while (gsmClient && gsmClient.available() < 4 &&
                   millis() - start < 5000L) {
                // wait
            }

            if (gsmClient.available() >= 4) {
                PRINTOUT(F("NIST responded after"), millis() - start, F("ms"));
                byte response[4] = {0};
                gsmClient.read(response, 4);
                gsmClient.stop();
                return parseNISTBytes(response);
            } else {
                PRINTOUT(F("NIST Time server did not respond!"));
                gsmClient.stop();
            }
        } else {
            MS_DBG(F("Unable to open TCP to NIST!"));
        }
    }
    return 0;
}


bool DigiXBeeCellularTransparent::updateModemMetadata(void) {
    bool success = true;

    // Unset whatever we had previously
    loggerModem::_priorRSSI          = SENSOR_DEFAULT_I;
    loggerModem::_priorSignalPercent = SENSOR_DEFAULT_I;
    // loggerModem::_priorBatteryState = SENSOR_DEFAULT;
    // loggerModem::_priorBatteryPercent = SENSOR_DEFAULT;
    // loggerModem::_priorBatteryPercent = SENSOR_DEFAULT;
    loggerModem::_priorModemTemp = SENSOR_DEFAULT_F;

    // Initialize variable
    int16_t signalQual = SENSOR_DEFAULT_I;

    // if not enabled don't collect data
    if (!loggerModem::_pollModemMetaData) return false;

    // Enter command mode only once
    MS_DBG(F("updateModemMetadata:"));
    gsmModem.commandMode();

    // Try for up to 15 seconds to get a valid signal quality
    // NOTE:  We can't actually distinguish between a bad modem response, no
    // modem response, and a real response from the modem of no service/signal.
    // The TinyGSM getSignalQuality function returns the same "no signal"
    // value (99 CSQ or 0 RSSI) in all 3 cases.
    uint32_t startMillis = millis();
    do {
        MS_DBG(F("Getting signal quality:"));
        signalQual = gsmModem.getSignalQuality();
        MS_DBG(F("Raw signal quality:"), signalQual);
        if (signalQual != 0 && signalQual != -9999) break;
        delay(250);
    } while ((signalQual == 0 || signalQual == -9999) &&
             millis() - startMillis < 15000L && success);

    // Convert signal quality to RSSI
    loggerModem::_priorRSSI = signalQual;
    MS_DBG(F("CURRENT RSSI:"), signalQual);
    loggerModem::_priorSignalPercent = getPctFromRSSI(signalQual);
    MS_DBG(F("CURRENT Percent signal strength:"), getPctFromRSSI(signalQual));

    MS_DBG(F("Getting chip temperature:"));
    loggerModem::_priorModemTemp = getModemChipTemperature();
    MS_DBG(F("CURRENT Modem temperature:"), loggerModem::_priorModemTemp);

    // Exit command modem
    MS_DBG(F("Leaving Command Mode:"));
    gsmModem.exitCommand();

    return success;
}


// Az extensions
void DigiXBeeCellularTransparent::setApn(const char* newAPN, bool copyId) {
    uint8_t newAPN_sz = strlen(newAPN);
    _apn              = newAPN;
    // TODO: njh test setAPN CopyID functons

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

String DigiXBeeCellularTransparent::getApn(void) {
    return _apn;
}
