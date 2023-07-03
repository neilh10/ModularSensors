/*****************************************************************************
tu_xx01.cpp
Based on examples/logging_to_MMW.ino
Adapted by Neil Hancock from Matt Barney 

Orginially Written By:  Sara Damiano (sdamiano@stroudcenter.org)
Development Environment: PlatformIO
Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
Software License: BSD-3.
  Copyright (c) 2022, Neil Hancock
  Copyright (c) 2020, Trout Unlimited, Stroud Water Research Center (SWRC)
  and the EnviroDIY Development Team

This implements a reliable interface to a specific set of sensors and some
paramters are configured in a file on the uSD ms_cfg.ini

DISCLAIMER:
THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
*****************************************************************************/

// ==========================================================================
//    Defines for the Arduino IDE
//    In PlatformIO, set these build flags in your platformio.ini
// ==========================================================================
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 240
#endif

// ==========================================================================
//    Include the base required libraries
// ==========================================================================
#include "ms_cfg.h"  //must be before ms_common.h & Arduino.h

// Use  MS_DBG()
#ifdef MS_TU_XX_DEBUG
#undef MS_DEBUGGING_STD
#define MS_DEBUGGING_STD "tu_ctd"
#define MS_DEBUG_THIS_MODULE 1
#endif  // MS_TU_XX_DEBUG

#ifdef MS_TU_XX_DEBUG_DEEP
#undef MS_DEBUGGING_DEEP
#define MS_DEBUGGING_DEEP "tu_ctdD"
#undef MS_DEBUG_THIS_MODULE
#define MS_DEBUG_THIS_MODULE 2
#endif  // MS_TU_XX_DEBUG_DEEP
#include "ModSensorDebugger.h"
#undef MS_DEBUGGING_STD
#undef MS_DEBUGGING_DEEP
#include <Arduino.h>          // The base Arduino library
#include <EnableInterrupt.h>  // for external and pin change interrupts
#include <ModularSensors.h>   // Include the main header for ModularSensors
#if defined USE_PS_EEPROM
#include "EEPROM.h"
#endif  // USE_PS_EEPROM
#include "ms_common.h"

// ==========================================================================
//    Data Logger Settings
// ==========================================================================
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
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID          = LOGGERID_DEF_STR;
const char* configIniID_def   = configIniID_DEF_STR;
const char* configDescription = CONFIGURATION_DESCRIPTION_STR;

// How frequently (in minutes) to log data
const uint8_t loggingIntervaldef = loggingInterval_CDEF_MIN;

// ==========================================================================
//     Local storage - evolving
// ==========================================================================
#ifdef USE_MS_SD_INI
persistent_store_t ps_ram;
#define epc ps_ram
#endif  //#define USE_MS_SD_INI

// ==========================================================================
//    Primary Arduino-Based Board and Processor
// ==========================================================================
#include <BatteryManagement.h>
BatteryManagement bms;

#include <sensors/ProcessorStats.h>

const long serialBaud =
    115200;  // Baud rate for the primary serial port for debugging
const int8_t greenLED  = 8;  // MCU pin for the green LED (-1 if not applicable)
const int8_t redLED    = 9;  // MCU pin for the red LED (-1 if not applicable)
const int8_t buttonPin = 21;  // MCU pin for a button to use to enter debugging
                              // mode  (-1 if not applicable)
const int8_t wakePin = A7;    // MCU interrupt/alarm pin to wake from sleep
// Set the wake pin to -1 if you do not want the main processor to sleep.
// In a SAMD system where you are using the built-in rtc, set wakePin to 1
const int8_t sdCardPwrPin = -1;  // MCU SD card power pin (-1 if not applicable)
const int8_t sdCardSSPin =
    12;  // MCU SD card chip select/slave select pin (must be given!)
const int8_t sensorPowerPin =
    22;  // MCU pin controlling main sensor power (-1 if not applicable)

// Create the main processor chip "sensor" - for general metadata
typedef enum {BT_MAYFLY_0_5, BT_MAYFLY_1_0, BT_last} bt_BoardType_t;
const char*    mcuBoardVersion_1_x = "v1.0";
const char*    mcuBoardVersion_0_5 = "v0.5b";
ProcessorStats mcuBoardPhy(mcuBoardVersion_1_x); //Define board default

// ==========================================================================
//    Settings for Additional Serial Ports
// ==========================================================================

// The modem and a number of sensors communicate over UART/TTL - often called
// "serial". "Hardware" serial ports (automatically controlled by the MCU) are
// generally the most accurate and should be configured and used for as many
// peripherals as possible.  In some cases (ie, modbus communication) many
// sensors can share the same serial port.

#if defined(ARDUINO_ARCH_AVR) || defined(__AVR__)  // For AVR boards
// Unfortunately, most AVR boards have only one or two hardware serial ports,
// so we'll set up three types of extra software serial ports to use

// AltSoftSerial by Paul Stoffregen
// (https://github.com/PaulStoffregen/AltSoftSerial) is the most accurate
// software serial port for AVR boards. AltSoftSerial can only be used on one
// set of pins on each board so only one AltSoftSerial port can be used. Not all
// AVR boards are supported by AltSoftSerial. AltSoftSerial is capable of
// running up to 31250 baud on 16 MHz AVR. Slower baud rates are recommended
// when other code may delay AltSoftSerial's interrupt response.
// Pins In/Rx 6  Out/Tx=5
#include <AltSoftSerial.h>
AltSoftSerial altSoftSerialPhy;

// NeoSWSerial (https://github.com/SRGDamia1/NeoSWSerial) is the best software
// serial that can be used on any pin supporting interrupts.
// You can use as many instances of NeoSWSerial as you want.
// Not all AVR boards are supported by NeoSWSerial.
#if 0
#include <NeoSWSerial.h>  // for the stream communication
const int8_t neoSSerial1Rx = 11;     // data in pin
const int8_t neoSSerial1Tx = -1;     // data out pin
NeoSWSerial neoSSerial1(neoSSerial1Rx, neoSSerial1Tx);
// To use NeoSWSerial in this library, we define a function to receive data
// This is just a short-cut for later
void neoSSerial1ISR()
{
    NeoSWSerial::rxISR(*portInputRegister(digitalPinToPort(neoSSerial1Rx)));
}
#endif
#if 0  // Not used
// The "standard" software serial library uses interrupts that conflict
// with several other libraries used within this program, we must use a
// version of software serial that has been stripped of interrupts.
// NOTE:  Only use if necessary.  This is not a very accurate serial port!
const int8_t softSerialRx = A3;     // data in pin
const int8_t softSerialTx = A4;     // data out pin

#include <SoftwareSerial_ExtInts.h>  // for the stream communication
SoftwareSerial_ExtInts softSerial1(softSerialRx, softSerialTx);
#endif
#endif  // End software serial for avr boards
// ==========================================================================
//    Wifi/Cellular Modem Settings
// ==========================================================================

// Create a reference to the serial port for the modem
// Extra hardware and software serial ports are created in the "Settings for
// Additional Serial Ports" section
HardwareSerial& modemSerial = Serial1;  // Use hardware serial if possible
// AltSoftSerial &modemSerial = altSoftSerialPhy;  // For software serial if
// needed NeoSWSerial &modemSerial = neoSSerial1;  // For software serial if
// needed Use this to create a modem if you want to monitor modem communication
// through a secondary Arduino stream.  Make sure you install the StreamDebugger
// library! https://github.com/vshymanskyy/StreamDebugger
#if defined STREAMDEBUGGER_DBG
#include <StreamDebugger.h>
StreamDebugger modemDebugger(modemSerial, STANDARD_SERIAL_OUTPUT);
#define modemSerHw modemDebugger
#else
#define modemSerHw modemSerial
#endif  // STREAMDEBUGGER_DBG

// Modem Pins - Describe the physical pin connection of your modem to your board
#define MODEM_VCC_CE_PIN 18
// Set the default for startup
const int8_t modemVccPin_mayfly_1_x = MODEM_VCC_CE_PIN;  //Pin18 on Xbee  Mayfly v1.x,
const int8_t modemVccPin_mayfly_0_5 = -2; //No power control rev 0.5b
#define modemVccPin modemVccPin_mayfly_1_x 

const int8_t modemStatusPin =
    19;  // MCU pin used to read modem status (-1 if not applicable)
const int8_t modemResetPin = -1;//20? MCU modem reset pin (-1 if unconnected)
const int8_t modemSleepRqPin =
    23;  // MCU pin used for modem sleep/wake request (-1 if not applicable)
const int8_t modemLEDPin = redLED;  // MCU pin connected an LED to show modem
                                    // status (-1 if unconnected)
const int8_t I2CPower = -1;  // sensorPowerPin; Needs to remain on if any IC powered like STC3100/KNH002
                             // off (-1 if unconnected)

#if defined UseModem_Module
#include <modems/ModemFactory.h>
// Network connection information
const char* apn_def =
    APN_CDEF;  // The APN for the gprs connection, unnecessary for WiFi
const char* wifiId_def =
    WIFIID_CDEF;  // The WiFi access point, unnecessary for gprs
const char* wifiPwd_def =
    WIFIPWD_CDEF;  // The password for connecting to WiFi, unnecessary for gprs
const long modemBaud = 9600;  // All XBee's use 9600 by default
const bool useCTSforStatus =
    false;  // true? Flag to use the XBee CTS pin for status

  loggerModem*     loggerModemPhyInst=NULL ;//was modemPhy 
#define loggerModemPhyDigiWifi ((DigiXBeeWifi *) loggerModemPhyInst)
#define loggerModemPhyDigiCell ((DigiXBeeCellularTransparent *) loggerModemPhyInst)

#if defined DIGI_RSSI_UUID
//The loggerModemPhyInst is created at run time 
Variable*  modemPhyRssi_var = new Modem_RSSI(loggerModemPhyInst);
float  modemPhyRssiGetValue(void) {  // Can't get value till instatiated
    if (NULL==loggerModemPhyInst) return 0;

    return modemPhyRssi_var->getValue(true);
}
// Create the calculated RSSI Variable object 
Variable* modemPhyRssi_calc =
    new Variable(modemPhyRssiGetValue,  // function that does the calculation
                 MODEM_RSSI_RESOLUTION, // resolution
                 MODEM_RSSI_UNIT_NAME, // var name.
                              //from http://vocabulary.odm2.org/variablename/
                 "dBm",  // var unit. 
                          //from http://vocabulary.odm2.org/units/
                 MODEM_RSSI_DEFAULT_CODE,  // var code MODEM_RSSI_DEFAULT_CODE
                 DIGI_RSSI_UUID);
#endif //DIGI_RSSI_UUID

#endif // UseModem_Module

// ==========================================================================
// Create a reference to the serial port for modbus
// Extra hardware and software serial ports are created in the "Settings for
// Additional Serial Ports" section
#if defined(KellerAcculevel_ACT) || defined(KellerNanolevel_ACT) || defined InsituLTrs485_ACT 
#if defined SerialModbus && (defined ARDUINO_ARCH_SAMD || defined ATMEGA2560)
HardwareSerial& modbusSerial = SerialModbus;  // Use hardware serial if possible
#else
AltSoftSerial& modbusSerial =
    altSoftSerialPhy;  // For software serial if needed
// NeoSWSerial &modbusSerial = neoSSerial1;  // For software serial if needed
#endif //SerialModbus
// byte acculevelModbusAddress = KellerAcculevelModbusAddress;  // The modbus
// address of KellerAcculevel
const int8_t rs485AdapterPower =
    rs485AdapterPower_DEF;  // Pin to switch RS485 adapter power on and off (-1
                            // if unconnected)
const int8_t modbusSensorPower =
    modbusSensorPower_DEF;  // Pin to switch sensor power on and off (-1 if
                            // unconnected)
const int8_t max485EnablePin =
    max485EnablePin_DEF;  // Pin connected to the RE/DE on the 485 chip (-1 if
                          // unconnected)

const int8_t RS485PHY_TX_PIN  = CONFIG_HW_RS485PHY_TX_PIN;
const int8_t RS485PHY_RX_PIN  = CONFIG_HW_RS485PHY_RX_PIN;
const int8_t RS485PHY_DIR_PIN = CONFIG_HW_RS485PHY_DIR_PIN;

#endif  // KellerXx Insitu

// ==========================================================================
// Units conversion functions
// ==========================================================================
#define SENSOR_T_DEFAULT_F -0.009999
float convertDegCtoF(float tempInput) {  // Simple deg C to deg F conversion
    if (-9999 == tempInput) return SENSOR_T_DEFAULT_F;
    if (SENSOR_T_DEFAULT_F == tempInput) return SENSOR_T_DEFAULT_F;
    return tempInput * 1.8 + 32;
}

float convertMmtoIn(float mmInput) {  // Simple millimeters to inches conversion
    if (-9999 == mmInput) return SENSOR_T_DEFAULT_F;
    if (SENSOR_T_DEFAULT_F == mmInput) return SENSOR_T_DEFAULT_F;
    return mmInput / 25.4;
}
float convert_mtoFt(float mInput) {  // meters to feet conversion
    if (-9999 == mInput) return SENSOR_T_DEFAULT_F;
    if (SENSOR_T_DEFAULT_F == mInput) return SENSOR_T_DEFAULT_F;

#define meter_to_feet 3.28084
    //   (1000 * mInput) / (25.4 * 12);
    return (meter_to_feet * mInput);
}

#if 0
// ==========================================================================
//    Campbell OBS 3 / OBS 3+ Analog Turbidity Sensor
// ==========================================================================
#include <sensors/CampbellOBS3.h>

const int8_t OBS3Power = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const uint8_t OBS3NumberReadings = 10;
const uint8_t ADSi2c_addr = 0x48;  // The I2C address of the ADS1115 ADC
#endif  // 0
#if defined Decagon_CTD_UUID
// ==========================================================================
//    Decagon CTD Conductivity, Temperature, and Depth Sensor
// ==========================================================================
#include <sensors/DecagonCTD.h>

const char*   CTDSDI12address   = "1";  // The SDI-12 Address of the CTD
const uint8_t CTDNumberReadings = 6;    // The number of readings to average
const int8_t  SDI12Power =
    sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const int8_t SDI12Data = 7;  // The SDI12 data pin

// Create a Decagon CTD sensor object
DecagonCTD ctdPhy(*CTDSDI12address, SDI12Power, SDI12Data, CTDNumberReadings);
#endif  // Decagon_CTD_UUID

// ==========================================================================
//    Insitu Level/Aqua Troll High Accuracy Submersible Level Transmitter
// wip Tested for Level Troll 500
// ==========================================================================
#ifdef InsituLTrs485_ACT  //Insitu_TrollModbus_UUID 
#include <sensors/InsituTrollModbus.h>

const byte ltModbusAddress =
    InsituLTrs485ModbusAddress_DEF;  // The modbus address of InsituLTrs485
// const int8_t rs485AdapterPower = sensorPowerPin;  // Pin to switch RS485
// adapter power on and off (-1 if unconnected) const int8_t modbusSensorPower =
// A3;  // Pin to switch sensor power on and off (-1 if unconnected) const
// int8_t max485EnablePin = -1;  // Pin connected to the RE/DE on the 485 chip
// (-1 if unconnected)
const uint8_t ltNumberReadings =
    3;  // The manufacturer recommends taking and averaging a few readings

// Create a Insitu LT sensor object

InsituLevelTroll InsituLT_snsr(ltModbusAddress, modbusSerial, rs485AdapterPower,
                               modbusSensorPower, max485EnablePin,
                               ltNumberReadings);

// Create pressure, temperature, and height variable pointers for the Nanolevel
// Variable *nanolevPress =  new InsituLTrs485_Pressure(&InsituLT_snsr,
// "12345678-abcd-1234-efgh-1234567890ab"); Variable *nanolevTemp =   new
// InsituLTrs485_Temp(&InsituLT_snsr, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *nanolevHeight = new InsituLTrs485_Height(&InsituLT_snsr,
// "12345678-abcd-1234-efgh-1234567890ab");

#endif  // InsituLTrs485_ACT
#if defined Insitu_TrollSdi12_UUID //|| defined Insitu_TrollModbus_UUID

// ==========================================================================
//    Insitu Aqua/Level Troll Pressure, Temperature, and Depth Sensor
// ==========================================================================
#include <sensors/InsituTrollSdi12a.h>

const char*   ITROLLSDI12address   = "1";  // SDI12 Address ITROLL
const uint8_t ITROLLNumberReadings = 2;    // The number of readings to average
const int8_t  IT_SDI12Power =
    sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const int8_t IT_SDI12Data = 7;  // The SDI12 data pin

// Create a  ITROLL sensor object
InsituTrollSdi12a itrollPhy(*ITROLLSDI12address, IT_SDI12Power, IT_SDI12Data,
                           ITROLLNumberReadings);
#endif  // Insitu_TrollXxx

// ==========================================================================
//   Analog Electrical Conductivity using the processors analog pins
// ==========================================================================
#ifdef AnalogProcEC_ACT
/** Start [AnalogElecConductivity] */
#include <sensors/AnalogElecConductivityM.h>
const int8_t ECpwrPin   = ECpwrPin_DEF;
const int8_t ECdataPin1 = ECdataPin1_DEF;

#define EC_RELATIVE_OHMS 100000
AnalogElecConductivityM analogEC_phy(ECpwrPin, ECdataPin1, EC_RELATIVE_OHMS);
/** End [AnalogElecConductivity] */
#endif  // AnalogProcEC_ACT

// ==========================================================================
//    Keller Acculevel High Accuracy Submersible Level Transmitter
// ==========================================================================
#if defined(KellerAcculevel_ACT) || defined(KellerNanolevel_ACT)
#define KellerXxxLevel_ACT 1
//#include <sensors/KellerAcculevel.h>

#endif  // defined KellerAcculevel_ACT  || defined KellerNanolevel_ACT

#if defined KellerAcculevel_ACT
#include <sensors/KellerAcculevel.h>

byte acculevelModbusAddress =
    KellerAcculevelModbusAddress_DEF;  // The modbus address of KellerAcculevel
const uint8_t acculevelNumberReadings =
    3;  // The manufacturer recommends taking and averaging a few readings

// Create a Keller Acculevel sensor object
KellerAcculevel acculevel_snsr(acculevelModbusAddress, modbusSerial,
                               rs485AdapterPower, modbusSensorPower,
                               max485EnablePin, acculevelNumberReadings);

// Create pressure, temperature, and height variable pointers for the Acculevel
// Variable *acculevPress = new KellerAcculevel_Pressure(&acculevel,
// "12345678-abcd-1234-efgh-1234567890ab"); Variable *acculevTemp = new
// KellerAcculevel_Temp(&acculevel, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *acculevHeight = new KellerAcculevel_Height(&acculevel,
// "12345678-abcd-1234-efgh-1234567890ab");

#if KellerAcculevel_DepthUnits > 1
// Create a depth variable pointer for the KellerAcculevel
Variable* kAcculevelDepth_m = new KellerAcculevel_Height(&acculevel_snsr,
                                                         "NotUsed");

float kAcculevelDepth_worker(void) {  // Convert depth KA
    // Get new reading
    float depth_m  = kAcculevelDepth_m->getValue(false);
    float depth_ft = convert_mtoFt(depth_m);
    MS_DEEP_DBG(F("Acculevel ft"), depth_ft, F("from m"), depth_m);
    return depth_ft;
}

// Setup the object that does the operation
Variable* KAcculevelHeightFt_var =
    new Variable(kAcculevelDepth_worker,  // function that does the calculation
                 3,                       // resolution
                 "waterDepth",            // var name. This must be a value from
                                // http://vocabulary.odm2.org/variablename/
                 "feet",  // var unit. This must be a value from This must be a
                          // value from http://vocabulary.odm2.org/units/
                 "kellerAccuDepth",  // var code
                 KellerXxlevel_Height_UUID);
#endif  // KellerAcculevel_DepthUnits
#endif  // KellerAcculevel_ACT


// ==========================================================================
//    Keller Nanolevel High Accuracy Submersible Level Transmitter
// ==========================================================================
#ifdef KellerNanolevel_ACT
#include <sensors/KellerNanolevel.h>

byte nanolevelModbusAddress =
    KellerNanolevelModbusAddress_DEF;  // The modbus address of KellerNanolevel
// const int8_t rs485AdapterPower = sensorPowerPin;  // Pin to switch RS485
// adapter power on and off (-1 if unconnected) const int8_t modbusSensorPower =
// A3;  // Pin to switch sensor power on and off (-1 if unconnected) const
// int8_t max485EnablePin = -1;  // Pin connected to the RE/DE on the 485 chip
// (-1 if unconnected)
const uint8_t nanolevelNumberReadings =
    3;  // The manufacturer recommends taking and averaging a few readings

// Create a Keller Nanolevel sensor object

KellerNanolevel nanolevel_snsr(nanolevelModbusAddress, modbusSerial,
                               rs485AdapterPower, modbusSensorPower,
                               max485EnablePin, nanolevelNumberReadings);

// Create pressure, temperature, and height variable pointers for the Nanolevel
// Variable *nanolevPress = new KellerNanolevel_Pressure(&nanolevel,
// "12345678-abcd-1234-efgh-1234567890ab"); Variable *nanolevTemp = new
// KellerNanolevel_Temp(&nanolevel, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *nanolevHeight = new KellerNanolevel_Height(&nanolevel,
// "12345678-abcd-1234-efgh-1234567890ab");

#endif  // KellerNanolevel_ACT

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
AOSongAM2315 am23xx(I2CPower);

// Create humidity and temperature variable pointers for the AM2315
// Variable *am2315Humid = new AOSongAM2315_Humidity(&am23xx,
// "12345678-abcd-1234-ef00-1234567890ab"); Variable *am2315Temp = new
// AOSongAM2315_Temp(&am23xx, "12345678-abcd-1234-ef00-1234567890ab");
/** End [ao_song_am2315] */
#endif  // ASONG_AM23XX_UUID

#if defined SENSIRION_SHT4X_UUID
// ==========================================================================
//  Sensirion SHT4X Digital Humidity and Temperature Sensor
// ==========================================================================
/** Start [sensirion_sht4x] */
#include <sensors/SensirionSHT4x.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t SHT4xPower     = sensorPowerPin;  // Power pin
const bool   SHT4xUseHeater = true;

// Create an Sensirion SHT4X sensor object
SensirionSHT4x sht4x(SHT4xPower, SHT4xUseHeater);

// Create humidity and temperature variable pointers for the SHT4X
/*Variable* sht4xHumid =
    new SensirionSHT4x_Humidity(&sht4x, "12345678-abcd-1234-ef00-1234567890ab");
Variable* sht4xTemp =
    new SensirionSHT4x_Temp(&sht4x, "12345678-abcd-1234-ef00-1234567890ab");*/
/** End [sensirion_sht4x] */
#endif //SENSIRION_SHT4X_UUID

// ==========================================================================
//    Maxim DS3231 RTC (Real Time Clock)
// ==========================================================================
#include <sensors/MaximDS3231.h>

// Create a DS3231 sensor object
MaximDS3231 ds3231(1);


// ==========================================================================
//    Bosch BME280 Environmental Sensor (Temperature, Humidity, Pressure)
// ==========================================================================
#if 0
#include <sensors/BoschBME280.h>

const int8_t I2CPower = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
uint8_t BMEi2c_addr = 0x76;
// The BME280 can be addressed either as 0x77 (Adafruit default) or 0x76 (Grove default)
// Either can be physically mofidied for the other address

// Create a Bosch BME280 sensor object
BoschBME280 bme280(I2CPower, BMEi2c_addr);


// ==========================================================================
//    Maxim DS18 One Wire Temperature Sensor
// ==========================================================================
#include <sensors/MaximDS18.h>

// OneWire Address [array of 8 hex characters]
// If only using a single sensor on the OneWire bus, you may omit the address
// DeviceAddress OneWireAddress1 = {0x28, 0xFF, 0xBD, 0xBA, 0x81, 0x16, 0x03, 0x0C};
const int8_t OneWirePower = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const int8_t OneWireBus = 6;  // Pin attached to the OneWire Bus (-1 if unconnected) (D24 = A0)

// Create a Maxim DS18 sensor objects (use this form for a known address)
// MaximDS18 ds18(OneWireAddress1, OneWirePower, OneWireBus);

// Create a Maxim DS18 sensor object (use this form for a single sensor on bus with an unknown address)
MaximDS18 ds18(OneWirePower, OneWireBus);
#endif  // 0

#if defined MAYFLY_BAT_STC3100
#include <sensors/STSTC3100_Sensor.h> 

// The STC3100 only has one address 

STSTC3100_Sensor stc3100_phy(STC3100_NUM_MEASUREMENTS);

//Its on a wingboard and may not be plugged in
bool       batteryFuelGauge_present = false;

//#define PRINT_STC3100_SNSR_VAR 1
#if defined PRINT_STC3100_SNSR_VAR 
bool userPrintStc3100BatV_avlb=false;
#endif  // PRINT_STC3100_SNSR_VAR
// Read the battery Voltage asynchronously, with  getLionBatStc3100_V()
// and have that voltage used on logging event

Variable* kBatteryVoltage_V = new STSTC3100_Volt(&stc3100_phy,"nu");

#if !defined MS_LION_MAX_VOLT 
#define  MS_LION_MAX_VOLT  4.9
#endif
#if !defined MS_LION_ERR_VOLT 
#define  MS_LION_ERR_VOLT  0.1234
#endif

float wLionBatStc3100_worker(void) {  // get the Battery Reading
    // Get reading - Assumes updated before calling
    float flLionBatStc3100_V = stc3100_phy.stc3100_device.v.voltage_V;
    if (MS_LION_MAX_VOLT < flLionBatStc3100_V) {
        Serial.print(F("  wLionBatStc3100 err meas LiIon V"));
        Serial.print(flLionBatStc3100_V, 4);
        Serial.println();
    
        flLionBatStc3100_V = MS_LION_ERR_VOLT;
    }
    // MS_DBG(F("wLionBatStc3100_worker"), flLionBatStc3100_V);
#if defined MS_TU_XX_DEBUG_DEEP
    DEBUGGING_SERIAL_OUTPUT.print(F("  wLionBatStc3100_worker "));
    DEBUGGING_SERIAL_OUTPUT.print(flLionBatStc3100_V, 4);
    DEBUGGING_SERIAL_OUTPUT.println();
#endif  // MS_TU_XX_DEBUG
#if defined PRINT_STC3100_SNSR_VAR
    if (userPrintStc3100BatV_avlb) {
        userPrintStc3100BatV_avlb = false;
        STANDARD_SERIAL_OUTPUT.print(F("  BatteryVoltage(V) "));
        STANDARD_SERIAL_OUTPUT.print(flLionBatStc3100_V, 4);
        STANDARD_SERIAL_OUTPUT.println();       
    }
#endif  // PRINT_STC3100_SNSR_VAR
    return flLionBatStc3100_V;
}
float getLionBatStc3100_V(void) {
    return kBatteryVoltage_V->getValue(false);
}
// Setup the object that does the operation
Variable*  pLionBatStc3100_var =
    new Variable(wLionBatStc3100_worker,  // function that does the calculation
                 4,                    // resolution
                 "batteryVoltage",     // var name. This must be a value from
                                    // http://vocabulary.odm2.org/variablename/
                 "volts",  // var unit. This must be a value from This must be a
                           // value from http://vocabulary.odm2.org/units/
                 "Stc3100_V",  // var code
                 STC3100_Volt_UUID);
#endif //MAYFLY_BAT_STC3100


#if defined ExternalVoltage_Volt0_UUID
// ==========================================================================
//    External Voltage via TI ADS1115
// ==========================================================================
#include <sensors/ExternalVoltage.h>

const int8_t ADSPower = 1;     // sensorPowerPin;  // Pin to switch power on and
                               // off (-1 if unconnected)
const int8_t ADSChannel0 = 0;  // The ADS channel of interest
const int8_t ADSChannel1 = 1;  // The ADS channel of interest
const int8_t ADSChannel2 = 2;  // The ADS channel of interest
const int8_t ADSChannel3 = 3;  // The ADS channel of interest
const float  dividerGain = 11;  // Gain RevR02 1/Gain 1M+100K
// The Mayfly is modified for ECN R04 or divide by 11
// Vbat is expected to be 3.2-4.2, so max V to ads is 0.38V,
// Practically the defaule GAIN_ONE for ADS1115 provide the best performance.
// 2020Nov13 Characterizing the ADS1115 for different gains seems to fall far
// short of the datasheet. Very frustrating.

const uint8_t ADSi2c_addr    = 0x48;  // The I2C address of the ADS1115 ADC
const uint8_t VoltReadsToAvg = 1;     // Only read one sample stable input

// Create an External Voltage sensor object
ExternalVoltage extvolt_AA0(ADSPower, ADSChannel0, dividerGain, ADSi2c_addr,
                         VoltReadsToAvg);
// ExternalVoltage extvolt1(ADSPower, ADSChannel1, dividerGain, ADSi2c_addr,
// VoltReadsToAvg); special Vcc 3.3V

//#define PRINT_EXTADC_BATV_VAR
#if defined PRINT_EXTADC_BATV_VAR
bool userPrintExtBatV_avlb=false;
#endif  // PRINT_EXTADC_BATV_VAR
// Create a capability to read the battery Voltage asynchronously,
// and have that voltage used on logging event
Variable* varExternalVoltage_Volt = new ExternalVoltage_Volt(&extvolt_AA0, "NotUsed");


float wLionBatExt_worker(void) {  // get the Battery Reading
    // Get new reading
   float flLionBatExt_V = varExternalVoltage_Volt->getValue(true);
    // float depth_ft = convert_mtoFt(depth_m);
    // MS_DBG(F("wLionBatExt_worker"), flLionBatExt_V);
#if defined MS_TU_XX_DEBUG
    DEBUGGING_SERIAL_OUTPUT.print(F("  wLionBatExt_worker "));
    DEBUGGING_SERIAL_OUTPUT.print(flLionBatExt_V, 4);
    DEBUGGING_SERIAL_OUTPUT.println();
#endif  // MS_TU_XX_DEBUG
#if defined PRINT_EXTADC_BATV_VAR
    if (userPrintExtBatV_avlb) {
        userPrintExtBatV_avlb = false;
        STANDARD_SERIAL_OUTPUT.print(F("  LiionBatExt(V) "));
        STANDARD_SERIAL_OUTPUT.print(flLionBatExt_V, 4);
        STANDARD_SERIAL_OUTPUT.println();       
    }
#endif  // PRINT_EXTADC_BATV_VAR
    return flLionBatExt_V;
}
float getLionBatExt_V(void) {
    return varExternalVoltage_Volt->getValue(false);
}
// Setup the object that does the operation
Variable* pLionBatExt_var =
    new Variable(wLionBatExt_worker,  // function that does the calculation
                 4,                    // resolution
                 "batteryVoltage",     // var name. This must be a value from
                                    // http://vocabulary.odm2.org/variablename/
                 "volts",  // var unit. This must be a value from This must be a
                           // value from http://vocabulary.odm2.org/units/
                 "extVolt0",  // var code
                 ExternalVoltage_Volt0_UUID);
#endif  // ExternalVoltage_Volt0_UUID

// Measuring the battery voltage can be from a number of sources
// Battery Filtering measurement in V
// Vbat from A6 noise filtering ~ bfv
// Mayfly 0.5-1.1 has a noisy battery measurement that uses
// a sliding window filtering technique to measure the lowest 
// battery voltage over a number of samples. 

float bat_filtered_v;
/// @brief Variable for software noise filtering window size
#define BFV_VBATLOW_WINDOW_SZ 6
float bfv_sliding[BFV_VBATLOW_WINDOW_SZ ];

bool bfv_Init=false; //Has it been initialized
uint8_t bfv_idx=0;

#if defined MAYFLY_BAT_CHOICE
#if MAYFLY_BAT_CHOICE == MAYFLY_BAT_STC3100
#define bms_SetBattery() bms.setBatteryV(wLionBatStc3100_worker());
#elif MAYFLY_BAT_CHOICE == MAYFLY_BAT_AA0 
// Need for internal battery 
#define bms_SetBattery() bms.setBatteryV(wLionBatExt_worker());
#elif  MAYFLY_BAT_CHOICE == MAYFLY_BAT_A6
#warning need to test mcuBoardPhy, interface 
// Read's the battery voltage
// NOTE: This returns the lowest battery level from previous running updates!
float getBatteryVoltageProc() {
    float  bat_now_v, bfv_lowest;
    uint8_t bfv_lp;
    bat_now_v = mcuBoardPhy.readSensorVbat();
    bfv_lowest= bat_now_v;

    if (bfv_Init) {
        //Insert the latest reading into the next slot
        bfv_sliding[bfv_idx]=bat_now_v;
        MS_DBG(F("Vbatlow update:"),bfv_idx, bfv_lowest);
        if (++bfv_idx >= BFV_VBATLOW_WINDOW_SZ) {
            //MS_DEEP_DBG(F("Vbatlow idx rst"),bfv_idx);
            bfv_idx=0;
        } 
        //Check all slots and find the lowest reading
        for (bfv_lp=0;bfv_lp<BFV_VBATLOW_WINDOW_SZ ;bfv_lp++){
            if (bfv_lowest>bfv_sliding[bfv_lp]) {
                bfv_lowest=bfv_sliding[bfv_lp];
                MS_DBG(F("Vbatlow i:"),bfv_lp, bfv_lowest);
            } else {
                //MS_DBG(F("Vbatlow i="),bfv_lp);
            }
        }
    } else {
        //Initialize with the highest voltage read.
        bfv_lowest = bat_now_v = mcuBoardPhy.readSensorVbat();

        //Look for highest voltage and init with that voltage
        // bfv_lowest is a temporary variable with highest found Vbat
        #define BFV_FILTER_INITIALIZE_LOOP 5
        for (bfv_lp=0;bfv_lp<BFV_FILTER_INITIALIZE_LOOP ;bfv_lp++){
            delay(200); // allow some settline between readings.
            bat_now_v = mcuBoardPhy.readSensorVbat();
            if (bat_now_v > bfv_lowest) {
                bfv_lowest =bat_now_v ;
                PRINTOUT(F("Vbat slow rise detect :"), bfv_lowest);
                }
        }

        for (bfv_lp=0;bfv_lp<BFV_VBATLOW_WINDOW_SZ ;bfv_lp++){
            bfv_sliding[bfv_lp]=bfv_lowest;
        }
        bfv_Init=true;
        Serial.print(F("Vbat_low init="));
        Serial.print(bat_now_v,3);
        Serial.print(F(" filter_sz="));
        Serial.println(BFV_VBATLOW_WINDOW_SZ); 
    }
    bat_filtered_v = bfv_lowest;
    if (bat_filtered_v > bat_now_v) {
        Serial.print(F("Vbat_low now/prev "));
        Serial.print(bat_now_v,3);
        Serial.print("/");
        Serial.println(bat_filtered_v,3);
        bat_filtered_v = bat_now_v;
    } else {
        Serial.print(F("Vbat_low prev/new "));
        Serial.print(bat_filtered_v,3);
        Serial.print("/");
        Serial.println(bat_now_v,3);
    }
    return bat_filtered_v;
}
#define bms_SetBattery() bms.setBatteryV(getBatteryVoltageProc());

#if defined REPORT_FILTERED_BAT_A6_V
float BatFilteredGetValue_V(void) {
    return bat_filtered_v;
}
// Create the calculated Battery Filtered V object 
Variable* BatFiltered_calc =
    new Variable(BatFilteredGetValue_V,  // function that does the calculation
                 PROCESSOR_BATTERY_RESOLUTION, // resolution
                 PROCESSOR_BATTERY_VAR_NAME, // var name.
                        //from  http://vocabulary.odm2.org/variablename/
                 PROCESSOR_BATTERY_UNIT_NAME, // var unit.
                        // from http://vocabulary.odm2.org/units/
                 PROCESSOR_BATTERY_DEFAULT_CODE, // var code
                 ProcessorStats_Batt_UUID);

#endif //REPORT_FILTERED_BAT_A6_V

#endif  //MAYFLY_BAT_A6
#else 
#warning MAYFLY_BAT_CHOICE not defined
//Leave battery settings at default or off
#define bms_SetBattery()
#endif  //defined MAYFLY_BAT_CHOICE
#if defined ProcVolt_ACT
// ==========================================================================
//    Internal  ProcessorAdc
// ==========================================================================
#include <sensors/processorAdc.h>
const int8_t  procVoltPower      = -1;
const uint8_t procVoltReadsToAvg = 1;  // Only read one sample

#if defined ARDUINO_AVR_ENVIRODIY_MAYFLY
// Only support Mayfly rev5 10M/2.7M &  10bit ADC (3.3Vcc / 1023)
const int8_t sensor_Vbatt_PIN    = A6;
const float  procVoltDividerGain = 4.7;
#else
#error define other processors ADC pins here
#endif  //
processorAdc sensor_batt_V(procVoltPower, sensor_Vbatt_PIN, procVoltDividerGain,
                           procVoltReadsToAvg);
// processorAdc sensor_V3v6_V(procVoltPower, sensor_V3V6_PIN,
// procVoltDividerGain, procVoltReadsToAvg);

#endif  // ProcVolt_ACT

// ==========================================================================
// Creating Variable objects for those values for which we're reporting in
// converted units, via calculated variables Based on baro_rho_correction.ino
// and VariableBase.h from enviroDIY.
// ==========================================================================
#if defined Decagon_CTD_UUID
// Create a temperature variable pointer for the Decagon CTD
Variable* CTDTempC = new DecagonCTD_Temp(&ctdPhy, "NotUsed");
float     CTDTempFgetValue(void) {  // Convert temp for the CTD
    return convertDegCtoF(CTDTempC->getValue());
}
// Create the calculated water temperature Variable object and return a pointer
// to it
Variable* CTDTempFcalc = new Variable(
    CTDTempFgetValue,     // function that does the calculation
    1,                    // resolution
    "temperatureSensor",  // var name. This must be a value from
                          // http://vocabulary.odm2.org/variablename/
    "degreeFahrenheit",   // var unit. This must be a value from This must be a
                          // value from http://vocabulary.odm2.org/units/
    "TempInF",            // var code
    CTD10_TEMP_UUID);

// Create a depth variable pointer for the Decagon CTD
Variable* CTDDepthMm = new DecagonCTD_Depth(&ctdPhy, "NotUsed");
float     CTDDepthInGetValue(void) {  // Convert depth for the CTD
    // Pass true to getValue() for the Variables for which we're only sending a
    // calculated version of the sensor reading; this forces the sensor to take
    // a reading when getValue is called.
    return convertMmtoIn(CTDDepthMm->getValue(true));
}
// Create the calculated depth Variable object and return a pointer to it
Variable* CTDDepthInCalc =
    new Variable(CTDDepthInGetValue,  // function that does the calculation
                 1,                   // resolution
                 "CTDdepth",          // var name. This must be a value from
                              // http://vocabulary.odm2.org/variablename/
                 "Inch",  // var unit. This must be a value from This must be a
                          // value from http://vocabulary.odm2.org/units/
                 "waterDepth",  // var code
                 CTD10_DEPTH_UUID);
#endif  // Decagon_CTD_UUID

#if defined MaximDS3231_TEMP_UUID || defined MaximDS3231_TEMPF_UUID
// Create a temperature variable pointer for the DS3231
#if defined MaximDS3231_TEMP_UUID
Variable*   ds3231TempC = new MaximDS3231_Temp(&ds3231, MaximDS3231_TEMP_UUID);
#else
Variable* ds3231TempC = new MaximDS3231_Temp(&ds3231);
#endif
float ds3231TempFgetValue(void) {  // Convert temp for the DS3231
    // Pass true to getValue() for the Variables for which we're only sending a
    // calculated version of the sensor reading; this forces the sensor to take
    // a reading when getValue is called.
    return convertDegCtoF(ds3231TempC->getValue(true));
}
#endif  // MaximDS3231_TEMP_UUID
// Create the calculated Mayfly temperature Variable object and return a pointer
// to it
#if defined MaximDS3231_TEMPF_UUID
Variable*   ds3231TempFcalc = new Variable(
    ds3231TempFgetValue,      // function that does the calculation
    1,                        // resolution
    "temperatureDatalogger",  // var name. This must be a value from
                              // http://vocabulary.odm2.org/variablename/
    "degreeFahrenheit",       // var unit. This must be a value from
                              // http://vocabulary.odm2.org/units/
    "TempInF",                // var code
    MaximDS3231_TEMPF_UUID);
#endif  // MaximDS3231_TEMPF_UUID


// ==========================================================================
//    Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================

Variable* variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoardPhy,
                                    ProcessorStats_SampleNumber_UUID),
#if defined STC3100_AVLBL_mAhr_UUID 
    new STC3100_AVLBL_MAH(&stc3100_phy,STC3100_AVLBL_mAhr_UUID),
#endif //STC3100_AVLBL_mAhr_UUID 
#if defined STC3100_USED1_mAhr_UUID 
    new STC3100_USED1_MAH(&stc3100_phy,STC3100_USED1_mAhr_UUID),
#endif //STC3100_USED1_mAhr_UUID 
#if defined STC3100_Volt_UUID 
    pLionBatStc3100_var,
#endif //STC3100_Volt_UUID 

#if defined ExternalVoltage_Volt0_UUID
    pLionBatExt_var,
#endif
#if defined MAYFLY_BAT_A6
    #if defined REPORT_FILTERED_BAT_A6_V
    BatFiltered_calc,
    #else 
    new ProcessorStats_Battery(&mcuBoardPhy, ProcessorStats_Batt_UUID),
    #endif // REPORT_FILTERED_BAT_A6_V
#endif  // MAYFLY_BAT_A6 
#if defined AnalogProcEC_ACT
    // Do Analog processing measurements.
    new AnalogElecConductivityM_EC(&analogEC_phy, EC1_UUID),
#endif  // AnalogProcEC_ACT

#if defined(ExternalVoltage_Volt1_UUID)
    new ExternalVoltage_Volt(&extvolt1, ExternalVoltage_Volt1_UUID),
#endif
#if defined Decagon_CTD_UUID
    // new DecagonCTD_Depth(&ctdPhy,CTD10_DEPTH_UUID),
    CTDDepthInCalc,
    // new DecagonCTD_Temp(&ctdPhy, CTD10_TEMP_UUID),
    CTDTempFcalc,
#endif  // Decagon_CTD_UUID
#if   defined Insitu_TrollModbus_UUID
//#if defined InsituLTrs485_ACT
    //   new insituLevelTroll_Pressure(&InsituLT_snsr, "UUID"),
    new InsituLevelTroll_Temp(&InsituLT_snsr, InsituLTrs485_Temp_UUID),
    new InsituLevelTroll_Height(&InsituLT_snsr, InsituLTrs485_Depth_UUID),
//#endif  // InsituLTrs485_ACT
#elif defined Insitu_TrollSdi12_UUID
    new InsituTrollSdi12a_Depth(&itrollPhy, ITROLLS_DEPTH_UUID),
    new InsituTrollSdi12a_Temp(&itrollPhy, ITROLLS_TEMP_UUID),
#endif  // Insitu_TrollXx
#if defined KellerAcculevel_ACT
// new KellerAcculevel_Pressure(&acculevel, "UUID"),
#if KellerAcculevel_DepthUnits > 1
    KAcculevelHeightFt_var,
#else
    new KellerAcculevel_Height(&acculevel_snsr, KellerXxlevel_Height_UUID),
#endif  // KellerAcculevel_DepthUnits
    new KellerAcculevel_Temp(&acculevel_snsr, KellerXxlevel_Temp_UUID),
#endif  // KellerAcculevel_ACT
#if defined KellerNanolevel_ACT
    //   new KellerNanolevel_Pressure(&nanolevel_snsr, "UUID"),
    new KellerNanolevel_Temp(&nanolevel_snsr, KellerXxlevel_Temp_UUID),
    new KellerNanolevel_Height(&nanolevel_snsr, KellerXxlevel_Height_UUID),
#endif  // SENSOR_CONFIG_KELLER_NANOLEVEL
// new BoschBME280_Temp(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
// new BoschBME280_Humidity(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
// new BoschBME280_Pressure(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
// new BoschBME280_Altitude(&bme280, "12345678-abcd-1234-ef00-1234567890ab"),
// new MaximDS18_Temp(&ds18, "12345678-abcd-1234-ef00-1234567890ab"),
#if defined SENSIRION_SHT4X_UUID
    new SensirionSHT4x_Humidity(&sht4x, SENSIRION_SHT4X_Air_Humidity_UUID),
    new SensirionSHT4x_Temp(&sht4x, SENSIRION_SHT4X_Air_Temperature_UUID),
// ASONG_AM23_Air_TemperatureF_UUID

#elif defined ASONG_AM23XX_UUID
    new AOSongAM2315_Humidity(&am23xx, ASONG_AM23_Air_Humidity_UUID),
    new AOSongAM2315_Temp(&am23xx, ASONG_AM23_Air_Temperature_UUID),
// ASONG_AM23_Air_TemperatureF_UUID
// calcAM2315_TempF
#endif  // ASONG_AM23XX_UUID
#if defined MaximDS3231_TEMP_UUID
    // new MaximDS3231_Temp(&ds3231,      MaximDS3231_Temp_UUID),
    ds3231TempC,
#endif  //  MaximDS3231_TEMP_UUID
#if defined MaximDS3231_TEMPF_UUID
    ds3231TempFcalc,
#endif  // MaximDS3231_TempF_UUID
#if defined DIGI_RSSI_UUID
    //loggerModemPhyInst not setup
    //new Modem_RSSI(&loggerModemPhyInst, DIGI_RSSI_UUID),
    modemPhyRssi_calc,
#endif  // DIGI_RSSI_UUID


};


// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);

// Create the VariableArray object
VariableArray varArray(variableCount, variableList);

// ==========================================================================
//     The Logger Object[s]
// ==========================================================================

// Create a new logger instance
Logger dataLogger(LoggerID, loggingIntervaldef, &varArray);


// ==========================================================================
//    A Publisher to Monitor My Watershed / EnviroDIY Data Sharing Portal
// ==========================================================================
// Device registration and sampling feature information can be obtained after
// registration at https://monitormywatershed.org or https://data.envirodiy.org
const char* registrationToken =
    registrationToken_UUID;  // Device registration token
const char* samplingFeature = samplingFeature_UUID;  // Sampling feature UUID

#if defined UseModem_PushData
#if defined USE_PUB_MMW
// Create a data publisher for the EnviroDIY/WikiWatershed POST endpoint
#include <publishers/EnviroDIYPublisher.h>
// EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modemPhy.gsmClient,
// registrationToken, samplingFeature);
EnviroDIYPublisher EnviroDIYPOST(dataLogger, 15, 0);
#endif //USE_PUB_MMW
#if defined USE_PUB_TSMQTT
// Create a data publisher for ThingSpeak
// Create a channel with fields on ThingSpeak.com 
// The fields will be sent in exactly the order they are in the variable array.
// Any custom name or identifier given to the field on ThingSpeak is irrelevant.
// No more than 8 fields of data can go to any one channel.  Any fields beyond
// the eighth in the array will be ignored.
//example https://www.mathworks.com/help/thingspeak/mqtt-publish-and-subscribe-with-esp8266.html
#include <publishers/ThingSpeakPublisher.h>
ThingSpeakPublisher TsMqttPub;
#endif //USE_PUB_TSMQTT
#if defined USE_PUB_UBIDOTS
#include <publishers/UbidotsPublisher.h>
UbidotsPublisher UbidotsPub;
#endif  // USE_PUB_UBIDOTS
#endif  // UseModem_PushData

// ==========================================================================
//    Working Functions
// ==========================================================================
#define SerialStd Serial
#include "iniHandler.h"

// Flashes the Green/Read LED's LED1 & LED2
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

// Flashes the Green LED
void greenflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        //digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        //digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}
/**
 * @brief Check if battery can provide power for action to be performed.
 *
 * @param reqBatState  On of bm_Lbatt_status_t
 *   LB_PWR_USEABLE_REQ forces a battery voltage reading
 *   all other requests use this reading
 *
 *   Customized per type of sensor configuration
 *
 * @return **bool *** True if power available, else false
 */
bm_Lbatt_status_t Lbatt_status = BM_LBATT_UNUSEABLE_STATUS;

bool isBatteryChargeGoodEnough(lb_pwr_req_t reqBatState) {
    bool retResult = true;

    bms_SetBattery();  // Read battery votlage
    switch (reqBatState) {
        case LB_PWR_USEABLE_REQ: 
        default:
            // Check battery status
            Lbatt_status = bms.isBatteryStatusAbove(true, BM_PWR_USEABLE_REQ);
            if (BM_LBATT_UNUSEABLE_STATUS == Lbatt_status) {
                retResult = false;
                PRINTOUT(F("---LB_PWR_USEABLE_REQ CANCELLED--Lbatt="),Lbatt_status);
            }
            MS_DBG(F(" isBatteryChargeGoodEnoughU "), retResult,
                   bms.getBatteryVm1(), F("V status"), Lbatt_status,
                   reqBatState);
            break;

        case LB_PWR_SENSOR_USE_REQ:

// heavy power sensors ~ use BM_PWR_LOWSTATUS
#if 1
            if (BM_LBATT_LOW_STATUS >= Lbatt_status) {
                retResult = false;
                PRINTOUT(F("---LB_PWR_SENSOR_USE_REQ CANCELLED--Lbatt="),Lbatt_status);
            }
#endif

            MS_DBG(F(" isBatteryChargeGoodEnouSnsr"), retResult);
            break;

        case LB_PWR_MODEM_USE_REQ:
            // WiFi BM_LBATT_MEDIUM_STATUS
            // Cell (BM_LBATT_HEAVY_STATUS
            if (BM_LBATT_HEAVY_STATUS > Lbatt_status) { 
                retResult = false; 
                PRINTOUT(F("---LB_PWR_MODEM_USE_REQ CANCELLED--Lbatt="),Lbatt_status);
            }
            MS_DBG(F(" isBatteryChargeGoodEnoughTx"), retResult);
            // modem sensors BM_PWR_LOW_REQ
            break;
    }
    return retResult;
}

// ==========================================================================
// Manages the Modbus Physical Pins.
// Pins pulled high when powered off will cause a ghost power leakage.
#if defined KellerXxxLevel_ACT || defined InsituLTrs485_ACT 
void        modbusPinPowerMng(bool status) {
    MS_DBG(F("  **** modbusPinPower"), status);
#if 1
    if (status) {
        modbusSerial.setupPhyPins();
    } else {
        modbusSerial.disablePhyPins();
    }
#endif
}
#endif  // KellerXxxLevel_ACT InsituLTrs485_ACT

#define PORT_SAFE(pinNum)   \
    pinMode(pinNum, INPUT); \
    digitalWrite(pinNum, LOW);

#define PORT_HIGH(pinNum)   \
    pinMode(pinNum, INPUT); \
    digitalWrite(pinNum, HIGH);

#define PORT_LOW(pinNum)   \
    pinMode(pinNum, OUTPUT); \
    digitalWrite(pinNum, LOW);

void unusedBitsMakeSafe() {
    // Set all unused Pins to a safe no current mode for sleeping
    // Mayfly variant.h: D0->23  (Analog0-7) or D24-31
    // PORT_SAFE( 0); Rx0  Tty
    // PORT_SAFE( 1); Tx0  TTy
    // PORT_SAFE( 2); Rx1  Xb?
    // PORT_SAFE( 3); Tx1  Xb?
#if !defined KellerXxxLevel_ACT && !defined InsituLTrs485_ACT 
    //PORT_SAFE(04);
    //PORT_SAFE(05);
#endif  // KellerXxxLevel_ACT
    PORT_SAFE(06);
    // PORT_SAFE(07); SDI12
    // PORT_SAFE(08); Grn Led
    // PORT_SAFE(09); Red LED
    PORT_SAFE(10);  // Assumes hw default SJ1 goes to A7  RTC Int Pullup
    PORT_SAFE(11);
    PORT_SAFE(12);
    // mosi LED PORT_SAFE(13);
    // miso PORT_SAFE(14);
    // sck PORT_SAFE(15);
    // scl PORT_SAFE(16);
    // sda PORT_SAFE(17);
    PORT_SAFE(18);
    PORT_SAFE(19);  // Xbee CTS
    PORT_SAFE(20);  // Xbee RTS
    PORT_SAFE(21);
    // All Rs485 and Modbus Insitu_TrollModbus_UUID
    #if defined KellerXxxLevel_ACT || defined InsituLTrs485_ACT 
    PORT_LOW(22);  //Pwr Sw rs485AdapterPower
    #else 
    PORT_SAFE(22);
    #endif
#if defined  UseModem_Module
    PORT_HIGH(23);  // Xbee DTR modemSleepRqPin LOW until Modem takes over
 #else 
    PORT_SAFE(23);
 #endif //UseModem_Module
    // Analog from here on
    // PORT_SAFE(24);//A0 ECData1
    PORT_SAFE(25);  // A1
    PORT_SAFE(26);  // A2
    PORT_SAFE(27);  // A3
    // PORT_SAFE(28); //A4  ECpwrPin
    PORT_SAFE(29);  // A5
    PORT_SAFE(30);  // A6
    // PORT_SAFE(31); //A7 Timer Int
};


#if defined MS_TTY_USER_INPUT
// ==========================================================================
bool userButton1Act = false;
void userButtonISR() {
    //Need setting up to actiavted by appropiate buttonPin
    MS_DBG(F("ISR userButton!"));
    if (digitalRead(buttonPin)) {
        userButton1Act =true;
    } 

} //userButtonISR

// ==========================================================================
 void setupUserButton () {
    if (buttonPin >= 0) {
        pinMode(buttonPin, INPUT_PULLUP);
        enableInterrupt(buttonPin, userButtonISR, CHANGE);
        MS_DBG(F("Button on pin"), buttonPin,
               F("user input."));
    }
} // setupUserButton
#include "tu_serialCmd.h"
#else 
#define setupUserButton()

#if defined MS_TTY_SERIAL_COUNT

long ch_count_tot=0; 
uint8_t serialInputCount() 
{
    //char incoming_ch;
    uint8_t ch_count_now=0;
 
    //Read any input queue
    while (Serial.available()) {
        //incoming_ch = 
        Serial.read();
        if (++ch_count_now > 250) break;
    }
    ch_count_tot+=ch_count_now;

    return ch_count_now;
}
#endif // MS_TTY_SERIAL_COUNT
#endif //MS_TTY_USER_INPUT
// ==========================================================================
// Poll management sensors- eg FuelGauges status  
// 
void  managementSensorsPoll() {
#if defined MAYFLY_BAT_STC3100
    if (batteryFuelGauge_present) {
        stc3100_phy.stc3100_device.readValues();
        Serial.print("BtMonStc31, ");
        //Create a time traceability header 
        String csvString = "";
        csvString.reserve(24);
        dataLogger.dtFromEpochTz(dataLogger.getNowLocalEpoch()).addToString(csvString);
        csvString += ", ";
        Serial.print(csvString);
        //Serial.print(dataLogger.formatDateTime_ISO8601(dataLogger.getNowEpochTz()));

        //Output readings
        Serial.print(F(" V=,"));
        Serial.print(stc3100_phy.stc3100_device.v.voltage_V, 3);
        Serial.print(F(", mA=,"));
        Serial.print(stc3100_phy.stc3100_device.v.current_mA, 1);
        Serial.print(F(", C mAh=,"));
        Serial.print(stc3100_phy.stc3100_device.v.charge_mAhr, 3);
        Serial.print(F(", EU mAh="));
        Serial.print(stc3100_phy.stc3100_device._energyUsed_mAhr, 3);        
        Serial.print(F(", EA mAh="));
        Serial.print(stc3100_phy.stc3100_device.getEnergyAvlbl_mAhr(), 3);
        Serial.print(F(",0x"));
        Serial.print((uint16_t)stc3100_phy.stc3100_device._batCharge1_raw,HEX);
        Serial.print(", CntsAdc=,");
        Serial.println(stc3100_phy.stc3100_device.v.counter);

        // Serial.print(" & IC Temp(C), ");
        // Serial.println(lc.getCellTemperature(), 1);

         //Ensure no matter how many readings are averaged, some values are only read once.
        stc3100_phy.stc3100_device.setHandshake1();
    }
#endif  // MAYFLY_BAT_STC3100
} //managementSensorsPoll


// ==========================================================================
// Checks available power on battery.
// 
bool batteryCheck(bm_pwr_req_t useable_req, bool waitForGoodBattery,uint8_t dbg_src) 
{
    bool LiBattPower_Unseable=false;
    bool UserButtonAct = false;
    uint16_t lp_wait = 1;

    PRINTOUT(F("batteryCheck req/wait/src"),useable_req, waitForGoodBattery,dbg_src);
    bms_SetBattery();
    do {
         #if MAYFLY_BAT_CHOICE == MAYFLY_BAT_STC3100
        //Read the V - FUT make compatible adcRead()
        stc3100_phy.stc3100_device.readValues();
        bms.setBatteryV(stc3100_phy.stc3100_device.v.voltage_V);
        PRINTOUT(F("Bat_V(stc3100)"),bms.getBatteryVm1());
        #elif MAYFLY_BAT_CHOICE == MAYFLY_BAT_AA0 
        PRINTOUT(F("Bat_V(Ext) tbd"));
        #elif  MAYFLY_BAT_CHOICE == MAYFLY_BAT_A6
        PRINTOUT(F("Bat_V(low)"),bat_filtered_v);
        #else //alt Read the V - FUT make compatible adcRead()
        PRINTOUT(F("Bat_V(undef)"));
        #endif //
        LiBattPower_Unseable =
            ((BM_LBATT_UNUSEABLE_STATUS ==
              bms.isBatteryStatusAbove(true, useable_req))
                 ? true
                 : false);
        if (LiBattPower_Unseable && waitForGoodBattery) 
        {
            /* Sleep
            * If can't collect data wait for more power to accumulate.
            * This sleep appears to taking 5mA, where as later sleep takes 3.7mA
            * Under no other load conditions the mega1284 takes about 35mA
            * Another issue is that onstartup currently requires turning on comms device to
            * set it up. On an XbeeS6 WiFi this can take 20seconds for some reason.
            */
            PRINTOUT(lp_wait++,F(": BatV Low ="), bms.getBatteryVm1()),F(" Sleep60sec");
            dataLogger.systemSleep(1); //This is not sleeping
            delay(1000);  // need 
            PRINTOUT(F("---tu_xx01:Wakeup check power. Press user button to bypass"));
        }
        if (buttonPin >= 0) { UserButtonAct = digitalRead(buttonPin); }
        dataLogger.resetWatchdog();
    } while (LiBattPower_Unseable && !UserButtonAct && waitForGoodBattery);
    return !LiBattPower_Unseable;
}

// ==========================================================================
// Activate the logger
// ==========================================================================
inline void dataLogger_do (uint8_t cia_val_override){
    #if defined USE_PUB_MMW
    dataLogger.logDataAndPubReliably(cia_val_override);
    #else
    //FUT use reliable 
    dataLogger.logDataAndPublish(); 
    #endif 
}
#if defined(__AVR__)
#if !defined FREE_RAM_SEED 
#define FREE_RAM_SEED 0
#endif
#if defined MS_DUMP_FREE_RAM
inline void initFreeRam() {
    extern int16_t __heap_start, *__brkval;
    uint8_t * p;
#define START_FREE_RAM ((uint8_t*)(__brkval == 0 ? (int)&__heap_start : (int)__brkval) )
#define END_FREE_RAM   (uint16_t)&p
    for (p = START_FREE_RAM; (uint16_t)p < END_FREE_RAM; p++) {
        *p =FREE_RAM_SEED ;
    }
}
#else 
inline void initFreeRam() {return;}
#endif //MS_DUMP_FREE_RAM
#else 
inline void initFreeRam() {}
#endif // defined(__AVR__)

// ==========================================================================
// Main setup function
// ==========================================================================
void setup() {
    // uint8_t resetCause = REG_RSTC_RCAUSE;        AVR ?//Reads from hw
    // uint8_t resetBackupExit = REG_RSTC_BKUPEXIT; AVR ?//Reads from hw
    uint8_t mcu_status = MCUSR; //is this already cleared by Arduino startup???
    //MCUSR = 0; //reset for unique read
    noInterrupts(); //should be off
    initFreeRam();
// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
#if defined SERIAL_PORT_USBVIRTUAL
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000)) {}
#endif

    // Start the primary serial connection
    Serial.begin(serialBaud);
    Serial.print(F("\n---Boot("));
    Serial.print(mcu_status,HEX);
    Serial.print(F(") Sw Build: "));
    Serial.print(build_ref);
    Serial.print(" ");
    Serial.println(git_usr);
    Serial.print(" ");
    Serial.println(git_branch);

    Serial.print(F("Sw Name: "));
    Serial.println(configDescription);

    Serial.print(F("ModularSensors version "));
    Serial.println(MODULAR_SENSORS_VERSION);

#if defined UseModem_Module
    Serial.print(F("TinyGSM Library version "));
    Serial.println(TINYGSM_VERSION);
#else
    Serial.println(F("TinyGSM - none"));
#endif

    unusedBitsMakeSafe();
    dataLogger.startFixedWatchdog();
    readAvrEeprom();
#if defined USE_PS_HW_BOOT
    //Print sames as .csv header, used in LoggerBaseExtCpp.h 
    Serial.print(F("Board: "));
    Serial.print((char*)epc.hw_boot.board_name);
    Serial.print(F(" rev:'"));
    Serial.print((char*)epc.hw_boot.rev);
    Serial.print(F("' sn:'"));
    Serial.print((char*)epc.hw_boot.serial_num);
    Serial.println(F("'"));
#endif  // USE_PS_HW_BOOT

    bt_BoardType_t boardType=BT_MAYFLY_1_0;
    if (0==strncmp((char*)epc.hw_boot.rev, "0.5",3)){
        boardType=BT_MAYFLY_0_5;
        Serial.println(F(" Board: Found Mayfly 0.5b"));
        mcuBoardPhy.setVersion(mcuBoardVersion_0_5); 
    } else {
        PRINTOUT( F(" Board: Assume Mayfly 1.1A ") );  

    }

    // set up for escape out of battery check if too low.
    // If buttonPress then exit.
    // Button is read inactive as low
    if (buttonPin >= 0) { pinMode(buttonPin, INPUT_PULLUP); }

 #if defined MAYFLY_BAT_STC3100
    //Setsup Sensor for battery read. FUT local V ADC
    // Could be warm boot in which case the STC3100 is alreading running
    if(!stc3100_phy.setup()){
        MS_DBG(F("STC3100 Not detected!"));
    } else {
        uint8_t dm_lp=0;
        batteryFuelGauge_present = true;
        Serial.print("STC3100 detected sn ");
        for (int snlp=1;snlp<(STC3100_ID_LEN-1);snlp++) {
            Serial.print(stc3100_phy.stc3100_device.serial_number[snlp],HEX);
        }
        //managementSensorsPoll(); stc3100_phy.stc3100_device.v.voltage_V
        #define STCDM_POLL 20
        #define STCDM_MIN_V 2.5
        for ( dm_lp=0;dm_lp<STCDM_POLL;dm_lp++) {
            delay(250); //Takes 8192 clock cycles for first V measurement
            stc3100_phy.stc3100_device.dmBegin(); //read registers
            if (STCDM_MIN_V  < stc3100_phy.stc3100_device.v.voltage_V) break;
        }

        PRINTOUT(F("  BatV/lp/cntr"), stc3100_phy.stc3100_device.v.voltage_V,dm_lp,stc3100_phy.stc3100_device.v.counter);
    }
#endif //MAYFLY_BAT_STC3100
    // A vital check on power availability
    batteryCheck(BM_PWR_USEABLE_REQ, true,1);


// Allow interrupts for software serial
#if defined SoftwareSerial_ExtInts_h
    enableInterrupt(softSerialRx, SoftwareSerial_ExtInts::handle_interrupt,
                    CHANGE);
#endif
#if defined NeoSWSerial_h
    enableInterrupt(neoSSerial1Rx, neoSSerial1ISR, CHANGE);
#endif

// Start the serial connection with the modem
#if defined UseModem_Module
    MS_DEEP_DBG("***modemSerial.begin");
    modemSerial.begin(modemBaud);
#endif  // UseModem_Module

#if defined(CONFIG_SENSOR_RS485_PHY)
    // Start the stream for the modbus sensors; all currently supported modbus
    // sensors use 9600 baud
    MS_DEEP_DBG("***modbusSerial.begin");
    delay(10);
    PRINTOUT(F("modbus Baudrate:"),MODBUS_BAUD_RATE,F("config:"),MODBUS_SERIAL_CONFIG);
    modbusSerial.begin(MODBUS_BAUD_RATE, MODBUS_SERIAL_CONFIG);
    modbusPinPowerMng(false);  // Turn off pins
#endif

    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();
    // not in this scope Wire.begin();

    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, -1, greenLED);
    setupUserButton(); //used for serialInput

#ifdef USE_MS_SD_INI
    // Set up SD card access
    PRINTOUT(F("---parseIni Start"));
    dataLogger.setPs_cache(&ps_ram);
    dataLogger.parseIniSd(configIniID_def, inihUnhandledFn);
    epcParser(); //use ps_ram to update classes
    PRINTOUT(F("---parseIni complete\n"));
#endif  // USE_MS_SD_INI

    // set the RTC to be in UTC TZ=0
    Logger::setRTCTimeZone(0);

    bms.printBatteryThresholds();

#ifdef UseModem_Module
    //Instaniate modem  
    LoggerModemFactory  mdmFactory;
    uint8_t mdmType = epc_network; 
    loggerModemPhyInst = mdmFactory.createInstance((modemTypesCurrent_t)mdmType,
        &modemSerHw, modemVccPin, 
        modemStatusPin, useCTSforStatus, 
        modemResetPin, modemSleepRqPin); /**/
    Client* inGsmClient;
    // Further runtime configuration of the Modem
    switch (mdmType) {
    case MODEMT_WIFI_DIGI_S6:
        loggerModemPhyDigiWifi->setWiFiId(epc_WiFiId, false);
        loggerModemPhyDigiWifi->setWiFiPwd(epc_WiFiPwd,false);
        inGsmClient =  &(loggerModemPhyDigiWifi->gsmClient);
        PRINTOUT(F("Modem config set as WIFI_DIGI_S6"));
        break;
    case MODEMT_LTE_DIGI_CATM1:
        loggerModemPhyDigiCell->setApn(epc_apn, false);
        inGsmClient =  &(loggerModemPhyDigiCell->gsmClient);
        PRINTOUT(F("Modem config set as LTE_DIGI_CATM1"));
        break;
    default: break;
        PRINTOUT(F("Modem config ERR** "),mdmType);
    };

    if (BT_MAYFLY_0_5==boardType) {
        loggerModemPhyInst->setPowerPin(modemVccPin_mayfly_0_5 );
    }

#if !defined UseModem_PushData
    const char None_STR[] = "None";
    dataLogger.setSamplingFeatureUUID(None_STR);
#endif  // UseModem_PushData

    // Attach the modem and information pins to the logger
    if ( loggerModemPhyInst->getPowerPin() > -1) {
        //For Mayfly1.0 turn on power 
        pinMode(loggerModemPhyInst->getPowerPin()  , OUTPUT);
        digitalWrite(loggerModemPhyInst->getPowerPin() , LOW); //Off
        PRINTOUT(F("---pwr Xbee OFF on pin "),loggerModemPhyInst->getPowerPin());
    } 

    dataLogger.attachModem(loggerModemPhyInst);
    loggerModemPhyInst->modemHardReset(); //Ensure in known state ~ 5mS

    // loggerModemPhyInst->setModemLED(modemLEDPin); //Used in UI_status subsystem
#endif  // UseModem_Module

    // Begin the logger
    MS_DBG(F("---dataLogger.begin "));
    dataLogger.begin();
#if defined UseModem_PushData
#if defined USE_PUB_MMW
    EnviroDIYPOST.begin(dataLogger, inGsmClient, 
                        ps_ram.app.provider.s.ed.registration_token,
                        ps_ram.app.provider.s.ed.sampling_feature);
    EnviroDIYPOST.setDIYHost(ps_ram.app.provider.s.ed.cloudId);
    EnviroDIYPOST.setQuedState(true);
    EnviroDIYPOST.setTimerPostTimeout_mS(ps_ram.app.provider.s.ed.timerPostTout_ms);
    EnviroDIYPOST.setTimerPostPacing_mS(ps_ram.app.provider.s.ed.timerPostPace_ms);
#endif //USE_PUB_MMW
#if defined USE_PUB_TSMQTT
    TsMqttPub.begin(dataLogger, inGsmClient , 
                ps_ram.app.provider.s.ts.thingSpeakMQTTKey,
                ps_ram.app.provider.s.ts.thingSpeakChannelID, 
                ps_ram.app.provider.s.ts.thingSpeakChannelKey);
    //FUT: njh tbd extensions for Reliable delivery
    //TsMqttPub.setQuedState(true);
    //TsMqttPub.setTimerPostTimeout_mS(ps_ram.app.provider.s.ts.timerPostTout_ms);
    //TsMqttPub.setTimerPostPacing_mS(ps_ram.app.provider.s.ts.timerPostPace_ms);
#endif// USE_PUB_TSMQTT
#if defined USE_PUB_UBIDOTS
    UbidotsPub.begin(dataLogger, inGsmClient ,
                        ps_ram.app.provider.s.ub.authentificationToken,
                        ps_ram.app.provider.s.ub.deviceID);
    //FUT: njh tbd extensions for Reliable delivery
    //UbidotsPub.setQuedState(true);
    //UbidotsPub.setTimerPostTimeout_mS(ps_ram.app.provider.s.ts.timerPostTout_ms);
    //UbidotsPub.setTimerPostPacing_mS(ps_ram.app.provider.s.ts.timerPostPace_ms);
#endif // USE_PUB_UBIDOTS

    dataLogger.setSendQueSz_num(ps_ram.app.msn.s.sendQueSz_num); 
    dataLogger.setSendEveryX(ps_ram.app.msn.s.collectReadings_num);
    dataLogger.setSendOffset(ps_ram.app.msn.s.sendOffset_min);  // delay Minutes
    dataLogger.setPostMax_num(ps_ram.app.msn.s.postMax_num); 

#endif  // UseModem_PushData

// Sync the clock  if we have good battery else assume set
#define LiIon_BAT_REQ BM_PWR_HEAVY_REQ 
#if defined UseModem_Module && !defined NO_FIRST_SYNC_WITH_NIST

    // The comms module  is supported and its expected to be configured.
    // ToDo Test - there may be a runtime use case where it exists but shouldn't be used?
    if (batteryCheck(LiIon_BAT_REQ, false,2)) 
    {
        MS_DBG(F("Sync with NIST "), bms.getBatteryVm1(),
           F("Req"), LiIon_BAT_REQ, F("Got"),
           bms.isBatteryStatusAbove(true, LiIon_BAT_REQ));

        bool syncResult = dataLogger.syncRTC();  // Will also set up the modemPhy
        PRINTOUT(F("Sync="),syncResult ,F("with NIST over "), loggerModemPhyInst->getModemName());
    } else {
        MS_DBG(F("Skipped sync with NIST as not enough power "), bms.getBatteryVm1(),
           F("Req"), LiIon_BAT_REQ );
    }
#endif  // UseModem_Module
    // List start time, if RTC invalid will also be initialized
    PRINTOUT(F("Local Time "),
             dataLogger.formatDateTime_ISO8601(dataLogger.getNowLocalEpoch()));
    PRINTOUT(F("Time local epoch "),dataLogger.getNowLocalEpoch());
    PRINTOUT(F("Time  UTC  epoch "),dataLogger.getNowUTCEpoch());

    //Setup sensors, including reading sensor data sheet that can be recorded on SD card
    PRINTOUT(F("Setting up sensors..."));
    batteryCheck(BM_PWR_SENSOR_CONFIG_BUILD_SPECIFIC, true,3);
    varArray.setupSensors();
// Create the log file, adding the default header to it
// Do this last so we have the best chance of getting the time correct and
// all sensor names correct
// Writing to the SD card can be power intensive, so if we're skipping
// the sensor setup we'll skip this too.

#if defined MAYFLY_BAT_STC3100
    //Setsup - Sensor not initialized yet. Reads unique serial number
    //stc3100_phy.stc3100_device.begin(); assumes done
    if(!stc3100_phy.stc3100_device.start()){
        MS_DBG(F("STC3100 Not detected!"));
    } else {
        batteryFuelGauge_present = true;
    }
    String sn(stc3100_phy.stc3100_device.getSn());
    PRINTOUT(F("STC3100 sn:"),sn);
    //if sn special, change series resistor range
    const char STC3100SN_100mohms_pm[] EDIY_PROGMEM = "13717d611";
    if (sn.equals(STC3100SN_100mohms_pm)) {
        #define STC3100_R_SERIES_100mOhms 100
        PRINTOUT(F("STC3100 diagnostic set R to mOhms "),STC3100_R_SERIES_100mOhms);
        stc3100_phy.stc3100_device.setCurrentResistor(STC3100_R_SERIES_100mOhms);
    } 
    stc3100_phy.stc3100_device.setBatteryCapacity_mAh(epc_battery_mAhr);
    delay(100); //Let STC3100 run a few ADC to collect readings
    stc3100_phy.stc3100_device.dmBegin(); //begin the Device Manager
#endif // MAYFLY_BAT_STC3100

// SDI12?
#if defined KellerNanolevel_ACT
    nanolevel_snsr.registerPinPowerMng(&modbusPinPowerMng);
#endif  // KellerNanolevel_ACT
#if defined KellerAcculevel_ACT
    acculevel_snsr.registerPinPowerMng(&modbusPinPowerMng);
#endif  // KellerAcculevel_ACT
#if defined InsituLTrs485_ACT 
    InsituLT_snsr.registerPinPowerMng(&modbusPinPowerMng);
    #if ! defined SENSORMODBUSMASTER_NO_DBG
    InsituLT_snsr.setDebugStream(&Serial);
    #endif //SENSORMODBUSMASTER_NO_DBG
#endif  // InsituLTrs485_ACT 
    PRINTOUT(F("Setting up file on SD card"));
    dataLogger.turnOnSDcard(
        true);  // true = wait for card to settle after power up
    dataLogger.createLogFile(true);  // true = write a new header
    dataLogger.turnOffSDcard(
        true);  // true = wait for internal housekeeping after write
    dataLogger.setBatHandler(&isBatteryChargeGoodEnough);

#if defined UseModem_Module && !defined NO_FIRST_SYNC_WITH_NIST
    if (batteryCheck(LiIon_BAT_REQ, false,4)) {
        dataLogger_do(LOGGER_RELIABLE_POST);
    } else {
//needs testing        dataLogger_do(LOGGER_NEW_READING); 
    }
#endif // UseModem_Module && !NO_FIRST_SYNC_WITH_NIST
#if defined MS_TTY_USER_INPUT
    tu2setup();
#endif //

    MS_DBG(F("\n\nSetup Complete ****"));
}


// ==========================================================================
// Main loop function
// ==========================================================================

void loop() {
    managementSensorsPoll();
    #if defined MS_TTY_USER_INPUT
    if ((true == userButton1Act ) || Serial.available()){
        userInputCollection =true;
        if (userButton1Act) {
            greenflash();
        }
        tu2SerialInputPoll();
        userButton1Act = false;
    } 
    #else 
    #if defined MS_TTY_INPUT_COUNT
    uint8_t ch_count_now;
    ch_count_now=  serialInputCount();
    if (ch_count_now) {
        PRINTOUT(F("\n\n  Char Cnt ** UNEXPECTED ** Rx"), ch_count_now,F(" Tot since reset"),ch_count_tot);
    } else {
        PRINTOUT(F("Char Cnt tot since reset"),ch_count_tot);
    }
    #endif //MS_TTY_USER_INPUT
    #endif // MS_TTY_INPUT_COUNT
    #if defined PRINT_EXTADC_BATV_VAR    
    // Signal when battery is next read, to give user information
    userPrintExtBatV_avlb=true;
    #endif //PRINT_EXTADC_BATV_VAR
    #if defined MAYFLY_BAT_STC3100 && defined PRINT_STC3100_SNSR_VAR 
    userPrintStc3100BatV_avlb=true;
    #endif //MAYFLY_BAT_STC3100
    dataLogger_do(0);
    #if defined MAYFLY_BAT_STC3100
    stc3100_phy.stc3100_device.periodicTask();
    #endif //MAYFLY_BAT_STC3100
}
