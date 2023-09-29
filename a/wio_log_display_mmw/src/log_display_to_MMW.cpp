/** =========================================================================
 * @file log_display_to_MMW.cpp
 * @brief WioT logging 6 sensors, local display of readings and publishing to Monitor My Watershed 
 *
 * @author Neil Hancock port to Wio Terminal
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * @copyright (c) 2017-2023 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This code is published under the BSD-3 license.
 *
 * Build Environment: Visual Studios Code with PlatformIO
 * Hardware Platform set in platformio.ini
 * default_envs =seeed_wio_terminal (320*240 pixles)
 * Future:
 * adafruit_pygamer
 * adafruit_expM4_eink
 * adafruit_metroM4 (+ arduinio std) 
 * 
 * For wio_t 320*240 pixles see
 * https://github.com/Seeed-Studio/Seeed_Arduino_LCD/tree/master/examples/320x240
 * pygamer ~  1.8" 160x128 color 
 * eInk - some have 3 buttons + reset
 * eink_2.9" 4 gray scales 268x128  80x38mm
 * eink_2.9" tri-color  296x128 80x38mm
 * eink_2.9" monochrome 296x128 
 * 
 * Tasks: 
 * * MS soak test  ie reliablity - WioT periodically resets on POST/WiFI
 * * use ms_cfg.ini
 * * DS18 Temperature logger  into J5/D0 d1 3V3 - Seeed SKU 101990578
 *    Right hand J5 D1=PB09
 *   AM2320 Humiidty Temperature sensor 
 *     Left hand Seeed socket
 * 
 * * WiFi subsystem, post to MMW - complete
 * * WiFi subystem, get accurate wall time, ntp/udp - complete
 *   Easy upgrade over USB UF2 bootloader from "USB Key"
 * * Easy programming/monitoring over USB. Option Serial1 UART for low power debug 
 * * Lower power when using Serial1 UART
 * 
 * Future
 * * Noise Level  internal micrcophone 
 *
 * 
 * 2023 Feb 21 WioT Power Measured USB Stick on USB-C
 *  USB active with WiFi 54mA, startup is 100mA
 * with lowpower WiFi/RTL87280 is unreliable
 *
 * DISCLAIMER:
 * THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
 * ======================================================================= */


/** Start [defines] */
/*#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif */
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

// Serial Debug Output routing
// For Mayfly its always a Serial though this connects with a USB chip
// For WioT it can be built in USB Serial or UART Serial1 that requires an FTDI or similar debug port.
// This is routed through the platformio.ino 
// Generally
// STANDARD_SERIAL_OUTPUT defined in  ModSensorDebugger.h
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

/** Start [WIO_TERMINAL_COMMS] */
// For WIO_TERMINAL that has WiFi and BT
// WioT uses an  API Message/SPI (Mayfly has AT over UART)
#include <modems/WioTerminal_rpcwifi.h>

// Create a reference to the serial port for the modem

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
//const int8_t modemVccPin    = RTL8720D_CHIP_PU; //future 
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
WioTerminal_rpcwifi modemWIOT( modemVccPin, 
                        modemStatusPin, modemResetPin, modemSleepRqPin,  
                        wifi_ssid, wifi_pwd
                        //,espSleepRqPin, espStatusPin
                        );
WioTerminal_rpcwifi modemPhy = modemWIOT;
/** End [WIO_TERMINAL_COMMS] */


// ==========================================================================
//  Using the Processor as a Sensor
// ==========================================================================
/** Start [processor_sensor] */
#include <sensors/ProcessorStats.h>

// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "WioT1.0";
ProcessorStats mcuBoard(mcuBoardVersion);
/** End [processor_sensor] */



// ==========================================================================
//  Bosch BME280 Environmental Sensor
// ==========================================================================
#if USE_BME280
/** Start [bme280] */
#include <sensors/BoschBME280.h>

const int8_t I2CPower    = sensorPowerPin;  // Power pin (-1 if unconnected)
uint8_t      BMEi2c_addr = 0x76;
// The BME280 can be addressed either as 0x77 (Adafruit default) or 0x76 (Grove
// default) Either can be physically mofidied for the other address

// Create a Bosch BME280 sensor object
BoschBME280 bme280(I2CPower, BMEi2c_addr);
/** End [bme280] */
#endif //USE_BME280

// ==========================================================================
//  Maxim DS18 One Wire Temperature Sensor
// ==========================================================================

#if defined TEMPERATURE_ALL_DS18
/** Start [ds18] */

#include <sensors/MaximDS18.h>

// OneWire Address [array of 8 hex characters]
// If only using a single sensor on the OneWire bus, you may omit the address
// DeviceAddress OneWireAddress1 = {0x28, 0xFF, 0xBD, 0xBA, 0x81, 0x16, 0x03,
// 0x0C};
const int8_t OneWirePower = -1;//sensorPowerPin;  Power pin (-1 if unconnected)
const int8_t OneWireBus   = OneWireBus_DEF;  // OneWire Bus Pin (-1 if unconnected)

// Create a Maxim DS18 sensor objects (use this form for a known address)
// MaximDS18 ds18(OneWireAddress1, OneWirePower, OneWireBus);

// Create a Maxim DS18 sensor object (use this form for a single sensor on bus
// with an unknown address)
// tbd how to do this for a number of same sensors.
// Could configure in .ini ~ which means 1) determining number of sensors 2) each sensors address
//4 Address found through using a OneWireSearch:
uint8_t Dev1_Ds18Addr_a[8]= { 0x28, 0x8A, 0xAB, 0xD9, 0x06, 0x00, 0x00, 0x3B };
uint8_t Dev1_Ds18Addr_b[8]= { 0x28, 0x8A, 0x92, 0x9C, 0x0E, 0x00, 0x00, 0x08 };
uint8_t Dev1_Ds18Addr_c[8]= { 0x28, 0xCB, 0x12, 0x9D, 0x0E, 0x00, 0x00, 0x0E };
uint8_t Dev1_Ds18Addr_d[8]= { 0x28, 0xE4, 0x78, 0x9C, 0x0E, 0x00, 0x00, 0xBF };
//4 Instances of the sensor
MaximDS18 ds18phy_a(Dev1_Ds18Addr_a,OneWirePower, OneWireBus);
MaximDS18 ds18phy_b(Dev1_Ds18Addr_b,OneWirePower, OneWireBus);
MaximDS18 ds18phy_c(Dev1_Ds18Addr_c,OneWirePower, OneWireBus);
MaximDS18 ds18phy_d(Dev1_Ds18Addr_d,OneWirePower, OneWireBus);
#endif // TEMPERATURE_ALL_DS18

/** End [ds18] */

#if defined(ASONG_AM23XX_UUID)
// ==========================================================================
//    AOSong AM2315 Digital Humidity and Temperature Sensor
// ==========================================================================
//use updated solving  https://github.com/neilh10/ModularSensors/issues/102
/** Start [ao_song_am2315] */
#include <sensors/AOSongAM2315.h>

// const int8_t I2CPower = 1;//sensorPowerPin;  // Pin to switch power on and
// off (-1 if unconnected)

// Create an AOSong AM2315 sensor object
// Data sheets says AM2315 and AM2320 have same address 0xB8 (8bit addr) of 1011
// 1000 or 7bit 0x5c=0101 1100 AM2320 AM2315 address 0x5C
#define I2CPower -1
AOSongAM2315 am23xx(I2CPower);

/** End [ao_song_am2315] */
#endif  // ASONG_AM23XX_UUID

#if defined(BAT_VOLTAGE_UUID )
#include "SparkFunBQ27441.h"
const unsigned int BATTERY_CAPACITY = 650; // Set Wio Terminal Battery's Capacity 

#define CHASSIS_BATTERY_NOT_PRESENT -0.123
float chassisBattery_volt = CHASSIS_BATTERY_NOT_PRESENT;

bool battery_present=false;

float getChassisBattery_volt(void) {
    if (battery_present) {
        // Read battery stats from the BQ27441-G1A
        uint16_t volts_mv = lipo.voltage(); // Read battery voltage (mV)
        chassisBattery_volt = ((float)volts_mv)/1000; //Convert to Volts
    }
    return chassisBattery_volt;
 } //getChassisBattery_volt

Variable*     chassisBattery_variable = new Variable(
    getChassisBattery_volt, // function that does the calculation
    3,                      // resolution
    "batteryVoltage",     // var name. This must be a value from
                    // http://vocabulary.odm2.org/variablename/
    "volts",  // var unit. This must be a value from This must be a
            // value from http://vocabulary.odm2.org/units/
    "Volt1",  // var code
    BAT_VOLTAGE_UUID);

#endif //BAT_VOLTAGE_UUID 

// ==========================================================================
//  Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
/** Start [variable_arrays] */
Variable* variableList[] = {
    //Order of the variable need to be consistent with reporting
    new ProcessorStats_SampleNumber(&mcuBoard, SEQUENCE_NUMBER_UUID),
#if defined ASONG_AM23XX_UUID
    new AOSongAM2315_Humidity(&am23xx, ASONG_AM23_Air_Humidity_UUID),
    new AOSongAM2315_Temp(&am23xx, ASONG_AM23_Air_Temperature_UUID),
// ASONG_AM23_Air_TemperatureF_UUID
#if defined TEMPERATURE_ALL_DS18
    new MaximDS18_Temp(&ds18phy_a, TEMPERATURE_A_UUID,"Ds18Ta"),
    new MaximDS18_Temp(&ds18phy_b, TEMPERATURE_B_UUID,"Ds18Tb"),
    new MaximDS18_Temp(&ds18phy_c, TEMPERATURE_C_UUID,"Ds18Tc"),
    new MaximDS18_Temp(&ds18phy_d, TEMPERATURE_D_UUID,"Ds18Td"),
#endif //TEMPERATURE_ALL_DS18
#endif  // ASONG_AM23XX_UUID    

#if defined(BAT_VOLTAGE_UUID) 
    chassisBattery_variable,
#endif // BAT_VOLTAGE_UUID 
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
//An Arduino client instance to use to print data to.
//     * Allows the use of any type of client and multiple clients tied to a
//     * single modem instance 
#if  defined WIO_TERMINAL
EnviroDIYPublisher EnviroDIYPOST(dataLogger, 15, 0);
#else
EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modemPhy.gsmClient,
                                 registrationToken, samplingFeature);
#endif // WIO_TERMINAL
/** End [publishers] */


// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */

// Reads the battery voltage
// NOTE: This will actually return the battery level from the previous update!
/*float getBatteryVoltage() {
    if (mcuBoard.sensorValues[0] == -9999) mcuBoard.update();
    return mcuBoard.sensorValues[0];
}*/
#define USE_DISPLAY
#if defined USE_DISPLAY
#include "uiHelperWioT.h"
uiHelperWioT ui_display;
#endif //USE_DISPLAY
/** End [working_functions] */

#if defined(BAT_VOLTAGE_UUID )

void setupBQ27441(void)
{
  // Use lipo.begin() to initialize the BQ27441-G1A and confirm that it's
  // connected and communicating.
  if (!lipo.begin()) // begin() will return true if communication is successful
  {
  // If communication fails, print an error message and loop forever.
    Serial.println("Error: Unable to communicate with BQ27441.");
    Serial.println("  Check battery unit plugged in.");

    battery_present=false;
    return;
  }
  battery_present=true;
  Serial.println("Connected to BQ27441!");
  
  // Uset lipo.setCapacity(BATTERY_CAPACITY) to set the design capacity
  // of your battery.
  lipo.setCapacity(BATTERY_CAPACITY);

}  // setupBQ27441

void printBatteryStats()
{
    if (battery_present) {
        // Read battery stats from the BQ27441-G1A
        unsigned int soc = lipo.soc();  // Read state-of-charge (%)
        unsigned int volts_mv = lipo.voltage(); // Read battery voltage (mV)
        int current = lipo.current(AVG); // Read average current (mA)
        //unsigned int fullCapacity = lipo.capacity(FULL); // Read full capacity (mAh)
        unsigned int capacity = lipo.capacity(REMAIN); // Read remaining capacity (mAh)
        int power = lipo.power(); // Read average power draw (mW)
        int health = lipo.soh(); // Read state-of-health (%)
        // Now print out those values:
        String toPrint = "BatteryStats, ";
        toPrint += String(soc) + ",%, ";
        toPrint += String(volts_mv) + " ,mV, ";
        toPrint += String(current) + ",mA,";
        toPrint += String(capacity) + ",mAh, ";
        //toPrint += String(fullCapacity) + " mAh | ";
        toPrint += String(power) + ",mW,";
        toPrint += String(health) + ",%";
        
        Serial.println(toPrint);
    }
} // printBatteryStats()

#else 
void setupBQ27441(void) {}
void printBatteryStats() {Serial.println("Battery : No battery present")}
#endif // BAT_VOLTAGE_UUID 

// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
/** Start [setup] */
void setup() {
// Serial debug could be Serial1 or USB connection established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
    bool statusUsb=false;
#if defined WIO_TERMINAL 
#if !defined USE_SERIAL1 & defined MS_LOGGING_TO_MMW_DEBUG
#pragma message "WIO TERM Output debug to USB "
    //Be nice to detect if USB is plugged in, but how?
    delay(10);
    statusUsb = USBDevice.ready();
    uint32_t start_ms=millis();
    uint32_t startupUsbDelay_ms;
    // Wait for up to 10seconds for a USB COM to be detected
    #define USB_WAIT_FOR_TERM_MS 10000
    while (!SerialStd && (millis() < USB_WAIT_FOR_TERM_MS )) {}
    startupUsbDelay_ms = millis()-start_ms;
#else  
#pragma message ("WIO TERRM Output to UART ") 
    statusUsb = USBDevice.ready();
    USBDevice.detach();
    //Serial.end();
    //statusUsb &= USBDevice.end(); !not uspported 
#endif // USE_SERIAL1
#endif // WIO_TERMINAL

    // Start the primary serial connection
    SerialStd.begin(serialBaud);

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

   
#if defined USE_DISPLAY
    ui_display.begin();

    SerialStd.print(" Setup Display wake. Backlight=");
    SerialStd.println(ui_display.tft.backlight());
    ui_display.display_on();
    ui_display.fillscreen("Modular Sensors: request time");
#endif //USE_DISPLAY

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // Always set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);

    setupBQ27441();
    printBatteryStats();
    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modemPhy);
    //modemPhy.setModemLED(modemLEDPin); //not mapped/tested WioTerminal
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, wakePin,
                             greenLED);
    dataLogger.setLoggerID("logdef");
    dataLogger.setLoggingInterval(2);
    delay(500);
    // Begin the logger
    dataLogger.begin();

    SerialStd.println(F("Setting up modemPhy as RTL8270 WiFiClient..."));
    EnviroDIYPOST.setClient(&modemPhy.endClient);
    EnviroDIYPOST.begin(dataLogger, &modemPhy.endClient, registrationToken, samplingFeature);
    //EnviroDIYPOST.setDIYHost("data.envirodiy.org"); //use default & port
    EnviroDIYPOST.setQuedState(true);
    EnviroDIYPOST.setTimerPostTimeout_mS(15432); //15.4Sec
    EnviroDIYPOST.setTimerPostPacing_mS(500);
    dataLogger.setLoggingInterval(2); //Set every minute, default 5min

    //dataLogger.setSendQueSz_num(100*60); //60days 
    dataLogger.setSendEveryX(1); //Default 2
    //dataLogger.setSendOffset(1);  // delay Minutes
    //dataLogger.setPostMax_num(100); 

    // Note: Basic Wio Terminal doesn't support reading the voltage
    SerialStd.println(F("Setting up sensors..."));
    delay(1000);
    varArray.setupSensors();

    // Customize setups as using same OneWire bus
    const char *ds18Name_a = "DS18a";
    const char *ds18Name_b = "DS18b";
    const char *ds18Name_c = "DS18c";
    const char *ds18Name_d = "DS18d";    
    ds18phy_a.set_sensorName(ds18Name_a);
    ds18phy_b.set_sensorName(ds18Name_b);    
    ds18phy_c.set_sensorName(ds18Name_c);
    ds18phy_d.set_sensorName(ds18Name_d);

    ds18phy_a.set_warmUpTime_ms(  50); //default 2mS
    ds18phy_b.set_warmUpTime_ms(1000);
    ds18phy_c.set_warmUpTime_ms(2000);
    ds18phy_d.set_warmUpTime_ms(3000); 

    ds18phy_a.set_stabilizationTime_ms(100);
    ds18phy_b.set_stabilizationTime_ms(100);
    ds18phy_c.set_stabilizationTime_ms(100);
    ds18phy_d.set_stabilizationTime_ms(100); //default 0mS

    // Sync the clock if it isn't valid or we have battery to spare
#if !defined NO_FIRST_SYNC_WITH_NIST
    // Synchronize the RTC with NIST
    // This will also set up the modem
    SerialStd.println(F("Synchronize the RTC with NIST"));
    dataLogger.syncRTC();

#endif //NO_FIRST_SYNC_WITH_NIST
    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.

    SerialStd.println(F("Setting up file on SD card"));
    dataLogger.turnOnSDcard(
        true);  // true = wait for card to settle after power up
    dataLogger.createLogFile(true);  // true = write a new header
    dataLogger.turnOffSDcard(
        true);  // true = wait for internal housekeeping after write


    //dataLogger.setSendOffset=0;
    dataLogger._sendEveryX_cnt=1;
    dataLogger.setPostMax_num(100);


    #if defined USE_DISPLAY
    DateTime now_dt= dataLogger.zero_sleep_rtc.now(); 
    //DateTime bootTime_dt ; // time object
    ui_display.fillscreen(now_dt.timestamp(DateTime::TIMESTAMP_FULL).c_str());
    #endif

    dataLogger.logDataAndPubReliably((0x03|0x08)); //CIA_POST_READINGS CIA_NO_SLEEP

    // Call the processor sleep
    SerialStd.println(F("Starting periodic logging\n"));
    delay(100);
    // do reading & then sleep- dataLogger.systemSleep();
}
/** End [setup] */


uint16_t displayOn_timer=0;
#define DISPLAY_ON_MASK 0x3
// ==========================================================================
//  Arduino Loop Function
// ==========================================================================
/** Start [loop] */
void loop() {


    #if defined USE_DISPLAY
    if ((displayOn_timer++)&DISPLAY_ON_MASK) {
        DateTime now_dt(dataLogger.markedLocalEpochTime);
        String ui_status("Stn#3 ");
        ui_display.display_on();

        ui_status += now_dt.timestamp(DateTime::TIMESTAMP_FULL).c_str();
        uiParm6_t parm6 = {&ui_status,
            //Use variable list starting from 2nd sensor or offset [1] 
            variableList[1]->getValue(),variableList[2]->getValue(), //AM23xx Humidity and Temperature
            variableList[3]->getValue(),variableList[4]->getValue(), //One wire temperature sensors
            variableList[5]->getValue(),variableList[6]->getValue()  //
        };
        ui_display.update6(&parm6 );
    } else {
        ui_display.display_off();
        //Check switch 

    }
    #endif // USE_DISPLAY

    printBatteryStats();
    dataLogger.logDataAndPubReliably();  //TCP / RTL !there


}
/** End [loop] */
