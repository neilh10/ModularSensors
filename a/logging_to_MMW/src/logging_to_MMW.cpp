/** =========================================================================
 * @file logging_to_MMW.ino
 * @brief Mayfly & WioT logging data and publishing to Monitor My Watershed 
 *
 * @author Neil Hancock port to Wio Terminal
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * @copyright (c) 2017-2022 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 *
 * Build Environment: Visual Studios Code with PlatformIO
 * Hardware Platform: default_envs =seeed_wio_terminal
 * Tasks: 
 * * MS soak test  ie reliable
 * * use ms_cfg.ini
 * * DS18 Temperature logger  into J5/D0 d1 3V3 - Seeed SKU 101990578
 *    J5 D1=PB09
 * * Noise Level  internal micrcophone 
 * * Uses USB port for programming/monitoring OR Serial1 UART for low power debug 
 * * sleep low power and wake
 * * Sleep and wake ~ 
 *    import from a\PlatformIO\Projects\afM4\lowPower\src\standbyExternalInterruptSAMD51.cpp
 * * WiFi subsystem, post to MMW - complete
 * * WiFi subystem, ntp/udp - complete
 * 
  * 2023Jan2 Power Measured USB Stick on USB-C
 * ??Sleeping 50mA
 * WiFi running 64mA, startup is 100mA
 *
 * DISCLAIMER:
 * THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
 * ======================================================================= */

// ==========================================================================
//  Defines for the Arduino IDE
//  NOTE:  These are ONLY needed to compile with the Arduino IDE.
//         If you use PlatformIO, you should set these build flags in your
//         platformio.ini
// ==========================================================================
/** Start [defines] */
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif
/** End [defines] */

// ==========================================================================
//  Include the libraries required for any data logger
// ==========================================================================
/** Start [includes] */
#include "ms_cfg.h"  //must be before ms_common.h & Arduino.h
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>

// EnableInterrupt is used by ModularSensors for external and pin change
// interrupts and must be explicitly included in the main program.
#include <EnableInterrupt.h>

// Include the main header for ModularSensors
#include <ModularSensors.h>
/** End [includes] */


// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
// The name of this file
extern const String build_ref = "a\\" __FILE__ " " __DATE__ " " __TIME__ " ";
#ifdef PIO_SRC_REV
const char git_branch[] = PIO_SRC_REV;
#else
const char git_branch[] = "brnch";
#endif
#ifdef PIO_SRC_USR
const char git_usr[] = PIO_SRC_USR;
#else
const char git_usr[] = "usr";
#endif

// The name of this program file
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID          = LOGGERID_DEF_STR;
const char* configIniID_def   = configIniID_DEF_STR;
const char* configDescription = CONFIGURATION_DESCRIPTION_STR;

// How frequently (in minutes) to log data
const uint8_t loggingIntervaldef = loggingInterval_CDEF_MIN;
// Your logger's timezone.
const int8_t timeZone = CONFIG_TIME_ZONE_DEF;  
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

// STANDARD_SERIAL_OUTPUT defined on the command line (or .ini), 
// generally
// if STANDARD_SERIAL_OUTPUT is Serial then its USB
// for USB requires special handling for USBDevice Driver
// else could be Serial1 - com1 etc

#define SerialStd STANDARD_SERIAL_OUTPUT

// Set the input and output pins for the logger
// NOTE:  Use -1 for pins that do not apply
const int32_t serialBaud = serialBaudDebugDef;  // Baud rate for debugging
const int8_t  greenLED   = greenLEDPinDef;
const int8_t  redLED     = redLEDPinDef; 
const int8_t  buttonPin  = buttonPinDef; // Pin for debugging mode (ie, button pin)
//const int8_t  buttonWakePin  = -1; // Pin for debugging mode (ie, button pin)
const int8_t  wakePin    = -1;//wakePinDef ;  // MCU interrupt/alarm pin to wake from sleep also used for setting TestMode
// Mayfly 0.x D31 = A7
// Set the wake pin to -1 if you do not want the main processor to sleep.
// In a SAMD system where you are using the built-in rtc, set wakePin to 1
const int8_t sdCardPwrPin   = sdCardPwrPinDef; // MCU SD card power pin
const int8_t sdCardSSPin    = sdCardSSPinDef;  // SD card chip select/slave select pin
const int8_t sensorPowerPin = sensorPowerPin_DEF;  // MCU pin controlling main sensor power
/** End [logging_options] */


// ==========================================================================
//  Wifi/Cellular Modem Options
// ==========================================================================
#if defined WIO_TERMINAL 
/** Start [WIO_TERMINAL_COMMS] */
// For WIO_TERMINAL that has WiFi and BT
#include <modems/WioTerminal_rpcwifi.h>
//Has an API not serial
//#include "ntpHelper.h"

// Create a reference to the serial port for the modem
//HardwareSerial& modemSerial = modemSerial_Upstream_DEF;  // Use hardware serial if possible
//HardwareSerial& modemSerial = NULL;  

//WioTerminal_rpcwifi.cpp rpcWifi.h WiFi.h class client

//const int32_t   modemBaud   = modemBaud_Upstream_DEF ;   // All XBee's use 9600 by default

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin    = modemVccPin_DEF;    // MCU pin controlling modem power
const int8_t modemStatusPin = -1;//modemStatusPin_DEF; // MCU pin used to read modem status
const bool useCTSforStatus  = false;  // Flag to use the XBee CTS pin for status
const int8_t modemResetPin  = -1;//modemResetPin_DEF;     // MCU pin connected to modem reset pin
const int8_t modemSleepRqPin = -1;//modemSleepRqPin_DEF;    // MCU pin for modem sleep/wake request
//const int8_t modemLEDPin = redLED;    // MCU pin connected an LED to show modem
                                      // status (-1 if unconnected)
const int8_t espSleepRqPin = -1;  // ESP8266 light sleep request
const int8_t espStatusPin = -1;   // ESP8266 light sleep status
// Network connection information
const char* wifi_ssid = WIFIID_CDEF;  // The WiFi access point
const char* wifi_pwd  = WIFIPWD_CDEF;  // The password for connecting to WiFi

// Create the loggerModem object


#if 1
/*WioTerminal_rpcwifi modemWIOT( modemVccPin, 
                        modemStatusPin, modemResetPin, modemSleepRqPin,  
                        wifi_ssid, wifi_pwd, 
                        espSleepRqPin, espStatusPin); */
WioTerminal_rpcwifi modemWIOT( RTL8720D_CHIP_PU, 
                        modemStatusPin, modemResetPin, modemSleepRqPin,  
                        wifi_ssid, wifi_pwd
                        //,espSleepRqPin, espStatusPin
                        );
#else
WioTerminal_rpcwifi modemWIOT(/*&modemSerial,*/ 
                        wifi_ssid, wifi_pwd);
#endif
WioTerminal_rpcwifi modemPhy = modemWIOT;
/** End [WIO_TERMINAL_COMMS] */
#elif defined(ARDUINO_AVR_ENVIRODIY_MAYFLY)
// Create a reference to the serial port for the modem
HardwareSerial& modemSerial = modemSerial_Upstream_DEF;  // Use hardware serial if possible
const int32_t   modemBaud   = modemBaud_Upstream_DEF ;   // All XBee's use 9600 by default

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin    = modemVccPin_DEF;    // MCU pin controlling modem power
const int8_t modemStatusPin = modemStatusPin_DEF; // MCU pin used to read modem status
const bool useCTSforStatus  = true;  // Flag to use the XBee CTS pin for status
const int8_t modemResetPin  = modemResetPin_DEF;     // MCU pin connected to modem reset pin
const int8_t modemSleepRqPin = modemSleepRqPin_DEF;    // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;    // MCU pin connected an LED to show modem
                                      // status (-1 if unconnected)
#if 0
/** Start [digi_xbee_cellular_transparent] */
// For any Digi Cellular XBee's
// NOTE:  The u-blox based Digi XBee's (3G global and LTE-M global) can be used
// in either bypass or transparent mode, each with pros and cons
// The Telit based Digi XBees (LTE Cat1) can only use this mode.
#include <modems/DigiXBeeCellularTransparent.h>



//njh need to make WiFI
// Network connection information
const char* apn = "xxxxx";  // The APN for the gprs connection


// NOTE:  If possible, use the `STATUS/SLEEP_not` (XBee pin 13) for status, but
// the `CTS` pin can also be used if necessary
DigiXBeeCellularTransparent modemXBCT(&modemSerial, modemVccPin, modemStatusPin,
                                      useCTSforStatus, modemResetPin,
                                      modemSleepRqPin, apn);
// Create an extra reference to the modem by a generic name
DigiXBeeCellularTransparent modemPhy = modemXBCT;
/** End [digi_xbee_cellular_transparent] */
#else 
/** Start [digi_xbee_wifi] */
// For the Digi Wifi XBee (S6B)
#include <modems/DigiXBeeWifi.h>


// Network connection information
const char* wifiId  = "xxxxx";  // WiFi access point name
const char* wifiPwd = "xxxxx";  // WiFi password (WPA2)

// Create the modem object
DigiXBeeWifi modemXBWF(&modemSerial, modemVccPin, modemStatusPin,
                       useCTSforStatus, modemResetPin, modemSleepRqPin, wifiId,
                       wifiPwd);
// Create an extra reference to the modem by a generic name
DigiXBeeWifi modemPhy = modemXBWF;
/** End [digi_xbee_wifi] */
#endif //digi
#endif //ARDUINO_AVR_ENVIRODIY_MAYFLY

// ==========================================================================
//  Using the Processor as a Sensor
// ==========================================================================
#if 1
/** Start [processor_sensor] */
#include <sensors/ProcessorStats.h>

// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "v1.1";
ProcessorStats mcuBoard(mcuBoardVersion);
/** End [processor_sensor] */
#endif

// ==========================================================================
//  Maxim DS3231 RTC (Real Time Clock)
// ==========================================================================
#if 0
/** Start [ds3231] */
#include <sensors/MaximDS3231.h>

// Create a DS3231 sensor object
MaximDS3231 ds3231(1);
/** End [ds3231] */
#endif //00

// ==========================================================================
//  Bosch BME280 Environmental Sensor
// ==========================================================================
#if 0
/** Start [bme280] */
#include <sensors/BoschBME280.h>

const int8_t I2CPower    = sensorPowerPin;  // Power pin (-1 if unconnected)
uint8_t      BMEi2c_addr = 0x76;
// The BME280 can be addressed either as 0x77 (Adafruit default) or 0x76 (Grove
// default) Either can be physically mofidied for the other address

// Create a Bosch BME280 sensor object
BoschBME280 bme280(I2CPower, BMEi2c_addr);
/** End [bme280] */
#endif //0

// ==========================================================================
//  Maxim DS18 One Wire Temperature Sensor
// ==========================================================================
/** Start [ds18] */
#include <sensors/MaximDS18.h>

// OneWire Address [array of 8 hex characters]
// If only using a single sensor on the OneWire bus, you may omit the address
// DeviceAddress OneWireAddress1 = {0x28, 0xFF, 0xBD, 0xBA, 0x81, 0x16, 0x03,
// 0x0C};
const int8_t OneWirePower = -1;//sensorPowerPin;  // Power pin (-1 if unconnected)
const int8_t OneWireBus   = OneWireBus_DEF;  // OneWire Bus Pin (-1 if unconnected)

// Create a Maxim DS18 sensor objects (use this form for a known address)
// MaximDS18 ds18(OneWireAddress1, OneWirePower, OneWireBus);

// Create a Maxim DS18 sensor object (use this form for a single sensor on bus
// with an unknown address)
// tbd how to do this for a number of same sensors.
// Could configure in .ini ~ which means 1) determining number of sensors 2) each sensors address
//Address OneWireSearch: 0x28, 0x8A, 0xAB, 0xD9, 0x06, 0x00, 0x00, 0x3B
uint8_t Dev1_Ds18Addr[8]= {0x28, 0x8A, 0xAB, 0xD9, 0x06, 0x00, 0x00, 0x3B};
MaximDS18 ds18(Dev1_Ds18Addr,OneWirePower, OneWireBus);

/** End [ds18] */


// ==========================================================================
//  Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
/** Start [variable_arrays] */
Variable* variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard, SEQUENCE_NUMBER_UUID),
    //new BoschBME280_Temp(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
    //new BoschBME280_Humidity(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
    //new BoschBME280_Pressure(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
    //new BoschBME280_Altitude(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
    new MaximDS18_Temp(&ds18, TEMPERATURE_UUID),
    //new MaximDS3231_Temp(&ds3231, "12345678-abcd-1234-ef00-1234567890ab"),
    //new ProcessorStats_Battery(&mcuBoard,BAT_VOLTAGE_UUID ),
    #if defined(ARDUINO_AVR_ENVIRODIY_MAYFLY)
    new Modem_RSSI(&modemPhy, "12345678-abcd-1234-ef00-1234567890ab"),
    //new Modem_SignalPercent(&modem, "12345678-abcd-1234-ef00-1234567890ab"),
    #endif // ARDUINO_AVR_ENVIRODIY_MAYFLY
};


// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);

// Create the VariableArray object
VariableArray varArray(variableCount, variableList);
/** End [variable_arrays] */


// ==========================================================================
//  The Logger Object[s]
// ==========================================================================
/** Start [loggers] */
// Create a new logger instance
Logger dataLogger(LoggerID, loggingIntervaldef, &varArray);
/** End [loggers] */


// ==========================================================================
//  Creating Data Publisher[s]
// ==========================================================================
/** Start [publishers] */
// A Publisher to Monitor My Watershed / EnviroDIY Data Sharing Portal
// Device registration and sampling feature information can be obtained after
// registration at https://monitormywatershed.org or https://data.envirodiy.org
const char* registrationToken = registrationToken_UUID;
const char* samplingFeature =   samplingFeature_UUID;

// Create a data publisher for the Monitor My Watershed/EnviroDIY POST endpoint
#include <publishers/EnviroDIYPublisher.h>
//EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modemPhy.Client, //WiFiClient,
//                                 registrationToken, samplingFeature);

//Add later EnviroDIYPOST.setClient(&modemPhy.(Class *inClient))
//An Arduino client instance to use to print data to.
//     * Allows the use of any type of client and multiple clients tied to a
//     * single TinyGSM modem instance 
EnviroDIYPublisher EnviroDIYPOST(dataLogger, 15, 0);

//EnviroDIYPublisher EnviroDIYPOST(dataLogger, registrationToken, samplingFeature);

/** End [publishers] */


// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */
#if defined USE_LEDS
// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}
#endif //USE_LEDS

// Reads the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage() {
    if (mcuBoard.sensorValues[0] == -9999) mcuBoard.update();
    return mcuBoard.sensorValues[0];
}
/** End [working_functions] */


// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
/** Start [setup] */
void setup() {
// SerialDb could be Serial1 or USB connection established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
    bool statusUsb=false;
#if !defined USE_SERIAL1  
#pragma message "Output to USB "
    //Need to detect if USB plugged in
    delay(10);
    statusUsb = USBDevice.ready();
    uint32_t start_ms=millis();
    uint32_t startupUsbDelay_ms;
    while (!SerialStd && (millis() < 10000)) {}
    startupUsbDelay_ms = millis()-start_ms;

#else

#pragma message ("Output to UART ") 
    statusUsb = USBDevice.ready();
    USBDevice.detach();
    //Serial.end();
    //statusUsb &= USBDevice.end(); !not uspported 
#endif

    // Start the primary serial connection
    SerialStd.begin(serialBaud);
    //while (!SerialStd); // debug wait for serial port to connect. Needed for native USB


    // Print a start-up note to the first serial port
    SerialStd.print(F("\n---Boot("));
    //SerialStd.print(mcu_status,HEX);
    SerialStd.print(F(") Sw Build: "));
    SerialStd.print(build_ref);
    SerialStd.print(" ");
    SerialStd.println(git_usr);
    SerialStd.print(" ");
    SerialStd.println(git_branch);

    SerialStd.print(F("Sw Name: "));
    SerialStd.println(configDescription);
    SerialStd.print("  ***** Low Power RTC SAMD51 ");
    SerialStd.print(F_CPU);
    SerialStd.println("MHz ***** ");
    SerialStd.print(F("Using ModularSensors Library version "));
    SerialStd.println(MODULAR_SENSORS_VERSION);
    //SerialStd.print(F("TinyGSM Library version "));
    //SerialStd.println(TINYGSM_VERSION);
#if defined USB_SERIALSTD
    SerialStd.print(" USB UsbStat=");
    SerialStd.print(statusUsb);
    SerialStd.print(" StartDelay=");
    SerialStd.print(startupUsbDelay_ms);
#else
    SerialStd.print(" UART UsbStat=");
    SerialStd.print(statusUsb);
#endif // USB_SERIALSTD
    SerialStd.println();

// Allow interrupts for software serial
#if defined SoftwareSerial_ExtInts_h
    enableInterrupt(softSerialRx, SoftwareSerial_ExtInts::handle_interrupt,
                    CHANGE);
#endif
#if defined NeoSWSerial_h
    enableInterrupt(neoSSerial1Rx, neoSSerial1ISR, CHANGE);
#endif

    // Start the serial connection with the modem
    // nh modemSerial.begin(modemBaud);

    // Set up pins for the LED's
    #if defined USE_LEDS
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();
    #endif //USE_LEDS
    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);

    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modemPhy);
    //modemPhy.setModemLED(modemLEDPin);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, wakePin,
                             greenLED);
    dataLogger.setLoggerID("logdef");
    dataLogger.setLoggingInterval(2);
    delay(500);
    // Begin the logger
    dataLogger.begin();

    SerialStd.println(F("Setting up modemPhy as WiFiClient..."));
    //EnviroDIYPOST.setClient(&modemPhy.endClient);
    EnviroDIYPOST.begin(dataLogger, &modemPhy.endClient, registrationToken, samplingFeature);
    //EnviroDIYPOST.setDIYHost("data.envirodiy.org"); //use default & port
    EnviroDIYPOST.setQuedState(true);
    EnviroDIYPOST.setTimerPostTimeout_mS(5432); //5.4Sec
    EnviroDIYPOST.setTimerPostPacing_mS(500);
    dataLogger.setLoggingInterval(2); //Set every minute, default 5min
    //dataLogger.setSendQueSz_num(ps_ram.app.msn.s.sendQueSz_num); 
    dataLogger.setSendEveryX(1); //Default 2
    //dataLogger.setSendOffset(ps_ram.app.msn.s.sendOffset_min);  // delay Minutes
    //dataLogger.setPostMax_num(ps_ram.app.msn.s.postMax_num); 

    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    //if (getBatteryVoltage() > 3.4) 
    {
        SerialStd.println(F("Setting up sensors..."));
        delay(1000);
        varArray.setupSensors();
    }

    // Sync the clock if it isn't valid or we have battery to spare
    if (1)///*getBatteryVoltage() > 3.55 ||*/ !dataLogger.isRTCSane()) 
    {
        // Synchronize the RTC with NIST
        // This will also set up the modem
        SerialStd.println(F("Synchronize the RTC with NIST"));
        dataLogger.syncRTC();
    }

    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    //if (getBatteryVoltage() > 3.4) 
    {
        SerialStd.println(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(
            true);  // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(
            true);  // true = wait for internal housekeeping after write
    }

    //dataLogger.setSendOffset=0;
    dataLogger._sendEveryX_cnt=1;
    //dataLogger.setPostMax_num(100);
    //dataLogger.logDataAndPubReliably(0x3);
    // Call the processor sleep
    SerialStd.println(F("Putting processor to sleep\n"));
    delay(100);
    dataLogger.systemSleep();
}
/** End [setup] */


// ==========================================================================
//  Arduino Loop Function
// ==========================================================================
/** Start [loop] */
// Use this short loop for simple data logging and sending
void loop() {
    // Note:  primitive but take a guess and set voltages
    // For hardware always take one reading and reference that  can change each time read
    /* Wio_T doesn't support BatteryV - its a seperate unit.
    float battery_V = 4.123;//nh dbg getBatteryVoltage() ;
    // At very low battery, just go back to sleep

    SerialStd.print(F("BatteryVoltage="));
    SerialStd.print(battery_V);
    if (battery_V < 3.4) 
    {
        SerialStd.println(F(" systemSleep"));
        delay(500);
        dataLogger.systemSleep();
    }
    // At moderate voltage, log data but don't send it over the modem
    else if (battery_V  < 3.55)  {
        SerialStd.println(F(" logData"));
        delay(500);
        dataLogger.logData();
    }
    // If the battery is good, send the data to the world
    else 
    */{
        //SerialStd.println(F("Start LogDataAndPubReliably"));
        //delay(500);
        dataLogger.logDataAndPubReliably();  //TCP / RTL !there
    }
}
/** End [loop] */
