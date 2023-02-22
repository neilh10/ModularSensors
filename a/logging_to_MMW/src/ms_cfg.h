/*****************************************************************************
ms_cfg.h_wio_wifi - ModularSensors Config - MMW _Wio/Mayfly WiFi
Status 220617: 0.33.1.abaa
Written By:  Neil Hancock www.envirodiy.org/members/neilh20/
Development Environment: PlatformIO
Hardware Platform(s): EnviroDIY Mayfly Arduino Datalogger+RS485 Wingboard

Software License: BSD-3.
  Copyright (c) 2022, Neil Hancock - all rights assigned to Stroud Water
Research Center (SWRC) and they may change this title to Stroud Water Research
Center as required and the EnviroDIY Development Team


DISCLAIMER:
THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
*****************************************************************************/
#ifndef ms_cfg_h
#define ms_cfg_h
#include <Arduino.h>  // The base Arduino library
// Local default defitions here

//**************************************************************************
// This configuration is for a standard Mayfly1.1 or Wio Terminal
// Sensors Used - two std to begin then
//#define AnalogProcEC_ACT 1


//#define MAYFLY_BAT_A6 4

//#define ENVIRODIY_MAYFLY_TEMPERATURE 1
//#define Decagon_CTD_UUID 1
//For Insitu_Troll specify one or none 
//#define Insitu_TrollSdi12_UUID 1
#define Insitu_TrollModbus_UUID 1


//Select one of following MAYFLY_BAT_xx as the source for BatterManagement Analysis
//#define MAYFLY_BAT_CHOICE MAYFLY_BAT_A6

//Only define 1 below . SENSIRION_SHT4X is on Mayfly 1.x
//#define SENSIRION_SHT4X_UUID
//#define ASONG_AM23XX_UUID 1

//Two heavy sensors with power useage
#define BM_PWR_SENSOR_CONFIG_BUILD_SPECIFIC BM_PWR_LOW_REQ

//Board specific 
#if defined(ARDUINO_AVR_ENVIRODIY_MAYFLY)
// Mayfly configuration
// Carrier board for Digi XBEE LTE CAT-M1 and jumper from battery
// Digi WiFi S6 plugged in directly
// For debug: C4 removed, strap for AA2/Vbat AA3/SolarV,
//#define MFVersion_DEF "v0.5b"
#define MFName_DEF "Mayfly"
//#define HwVersion_DEF MFVersion_DEF
#define HwName_DEF MFName_DEF
#define CONFIGURATION_DESCRIPTION_STR "Maylfy Digi LTE XB3-C-A2 MMW"

#define USE_MS_SD_INI 1
//#define USE_PS_EEPROM 1
//#define USE_PS_HW_BOOT 1

//#define USE_PS_modularSensorsCommon 1
#define serialBaudDebugDef 115200 
#define USE_LEDS
#define greenLEDPinDef 8  // MCU pin for the green LED (-1 if not applicable)
#define redLEDPinDef 9    // MCU pin for the red LED (-1 if not applicable)
#define buttonPinDef 21
#define wakePinDef 31

#define sdCardPwrPinDef   -1  // MCU SD card power pin
#define sdCardSSPinDef    12  // SD card chip select/slave select pin


#define sensorPowerPin_DEF 22
#define OneWireBus_DEF 6

#define modemVccPin_DEF -2  // MCU pin controlling modem power
#define modemSleepRqPin_DEF 23
#define modemStatusPin_DEF  19  // MCU pin used to read modem status (-1 if not applicable)
#define modemResetPin_DEF   20  // MCU pin connected to modem reset pin (-1 if unconnected)
#define modemSerial_Upstream_DEF Serial1
#define modemBaud_Upstream_DEF 9600

#elif defined(WIO_TERMINAL) 
// Wio configuration
// Carrier board for Digi XBEE LTE CAT-M1 and jumper from battery
// On board WiFi 


#define HwName_DEF "WioTerminal"
#define CONFIGURATION_DESCRIPTION_STR "WioTerm WiFi Basic"

#define USE_MS_SD_INI 1
//#define USE_PS_EEPROM 1
//#define USE_PS_HW_BOOT 1

//#define USE_PS_modularSensorsCommon 1
#define serialBaudDebugDef 115200 
#define greenLEDPinDef -1  // wioT no LED MCU pin for the green LED (-1 if not applicable)
#define redLEDPinDef -1   //wioT no LED M MCU pin for the red LED (-1 if not applicable)
#define buttonPinDef WIO_KEY_A //wioT 
#define wakePinDef  WIO_KEY_B  //wioT
// ALso WIO_KEY_C and 5 way switch

#define sdCardPwrPinDef   -1  //  MCU SD card power pin
#define sdCardSSPinDef PIN_SPI2_SS //wioManual SD card chip select/slave select pin

#if defined WIO_TERMINAL 
#define sensorPowerPin_DEF -1 //WioT always on 
#define OneWireBus_DEF 1  //WioT J4 Pin2 = D1
#else 
#define sensorPowerPin_DEF 22 //mayfly 
#define OneWireBus_DEF 6  //mayfly 
#endif //WIO_TERMINAL 

#define modemVccPin_DEF -2  // wioT MCU pin controlling modem power
//#define modemSleepRqPin_DEF 23 //mayfly 
//#define modemStatusPin_DEF  19  //mayfly  MCU pin used to read modem status (-1 if not applicable)
//#define modemResetPin_DEF   20  //mayfly  MCU pin connected to modem reset pin (-1 if unconnected)
#define modemSerial_Upstream_DEF Serial1 //mayfly 
#define modemBaud_Upstream_DEF 9600 //mayfly 

#endif //Board

#define configIniID_DEF_STR "ms_cfg.ini"
#define CONFIG_TIME_ZONE_DEF -8

// ** How frequently (in minutes) to log data **
// For two Loggers defined logger2Mult with the faster loggers timeout and the
// multiplier to the slower loggger
// #define  loggingInterval_Fast_MIN (1)
// #define logger2Mult 5 ~Not working for mayfly

// How frequently (in minutes) to log data
#if defined logger2Mult
#define loggingInterval_CDEF_MIN (loggingInterval_Fast_MIN * logger2Mult)
#else
#define loggingInterval_CDEF_MIN 15
#endif  // logger2Mult
// Maximum logging setting allowed
#define loggingInterval_MAX_CDEF_MIN 6 * 60

// Maximum logging setting allowed
#define loggingInterval_MAX_CDEF_MIN 6 * 60


// Supports DigiXBeeCellularTransparent & DigiXBeeWifi
#define UseModem_Module 1
#if UseModem_Module 
// The Modem is used to push data and also sync Time
// In standalong logger, no internet, Modem can be required at factor to do a
// sync Time Normally enable both of the following. In standalone, disable
// UseModem_PushData.
#define UseModem_PushData 1
//Select buildtime Publishers  supported. 
// The persisten resources (EEPROM) are allocated as a baselevel no matter what options 
#define USE_PUB_MMW      1
//#define USE_PUB_TSMQTT   1
//#define  USE_PUB_UBIDOTS 1

// Required for TinyGsmClient.h
#define TINY_GSM_MODEM_XBEE

// The APN for the gprs connection, unnecessary for WiFi
#define APN_CDEF "VZWINTERNET"

// The WiFi access point  never set to real, as should be set by config.
#define WIFIID_CDEF "ArthurGuestSsid"
// NULL for none, or  password for connecting to WiFi,
#define WIFIPWD_CDEF "Arthur8166"
#define MMW_TIMER_POST_TIMEOUT_MS_DEF 5000L
//POST PACING ms 0-15000
#define MMW_TIMER_POST_PACING_MS_DEF 100L
//Post MAX Num - is num of MAX num at one go. 0 no limit
//#define MMWGI_POST_MAX_RECS_MUM_DEF 100 //ms_common.h
//Manage Internet - common for all providers
#define MNGI_COLLECT_READINGS_DEF 1
#define MNGI_SEND_OFFSET_MIN_DEF 0
#endif  // UseModem_Module 

// This might need revisiting
#define ARD_ANLAOG_MULTIPLEX_PIN A6

//#define SENSOR_CONFIG_GENERAL 1
//#define KellerAcculevel_ACT 1
// Defaults for data.envirodiy.org
//Test08 https://monitormywatershed.org/sites/tu_rc_test08/
#define LOGGERID_DEF_STR "test08"
#define NEW_LOGGERID_MAX_SIZE 40
#define registrationToken_UUID "0cf7c40a-232e-457d-87d6-cea5c0757fec"
#define samplingFeature_UUID   "236c674b-69b9-43af-b0d6-33d67b870ecc"
#define SEQUENCE_NUMBER_UUID   "8c57835f-a32f-4d62-82dc-0ba09f04cf52"
#define BAT_VOLTAGE_UUID       "3bebd4a3-8b54-4f92-ba55-5fd2fd021358"
#define TEMPERATURE_A_UUID     "03e7b375-97a7-4423-a3f0-1d822d8b19b9"
#endif  // ms_cfg_h