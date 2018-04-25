/*
 *KellerAcculevel.h
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Initial library developement done by Anthony Aufdenkampe <aaufdenkampe@limno.com>
 *
 *This file is for Modbus communication to  Keller Series 30, Class 5, Group 20 sensors,
 *that are Software version 5.20-12.28 and later (i.e. made after the 2012 in the 28th week)
 *Only tested the Acculevel
 *
 *Documentation for the Yosemitech Protocol commands and responses, along with
 *information about the various variables, can be found
 *in the EnviroDIY KellerModbus library at:
 * https://github.com/EnviroDIY/KellerModbus
*/

#ifndef KellerAcculevel_h
#define KellerAcculevel_h

#include "KellerParent.h"
#include "VariableBase.h"

#define KellerAcculevel_NUM_VARIABLES 3
#define KellerAcculevel_WARM_UP_TIME_MS 500
#define KellerAcculevel_STABILIZATION_TIME_MS 5000
#define KellerAcculevel_MEASUREMENT_TIME_MS 1500

#define KellerAcculevel_PRESSURE_RESOLUTION 5
#define KellerAcculevel_PRESSURE_VAR_NUM 0

#define KellerAcculevel_TEMP_RESOLUTION 2
#define KellerAcculevel_TEMP_VAR_NUM 1

#define KellerAcculevel_HEIGHT_RESOLUTION 4
#define KellerAcculevel_HEIGHT_VAR_NUM 2


// The main class for the Keller Sensors
class KellerAcculevel : public KellerParent
{
public:
    // Constructors with overloads
    KellerAcculevel(byte modbusAddress, Stream* stream, int8_t powerPin,
                   int8_t enablePin = -1, uint8_t measurementsToAverage = 1)
     : KellerParent(modbusAddress, stream, powerPin, enablePin, measurementsToAverage,
                    Acculevel, F("KellerAcculevel"), KellerAcculevel_NUM_VARIABLES,
                    KellerAcculevel_WARM_UP_TIME_MS, KellerAcculevel_STABILIZATION_TIME_MS, KellerAcculevel_MEASUREMENT_TIME_MS)
    {}
    KellerAcculevel(byte modbusAddress, Stream& stream, int8_t powerPin,
                   int8_t enablePin = -1, uint8_t measurementsToAverage = 1)
     : KellerParent(modbusAddress, stream, powerPin, enablePin, measurementsToAverage,
                    Acculevel, F("KellerAcculevel"), KellerAcculevel_NUM_VARIABLES,
                    KellerAcculevel_WARM_UP_TIME_MS, KellerAcculevel_STABILIZATION_TIME_MS, KellerAcculevel_MEASUREMENT_TIME_MS)
    {}
};


// Defines the PressureGauge (vented & barometricPressure corrected) variable
class KellerAcculevel_Pressure : public Variable
{
public:
    KellerAcculevel_Pressure(Sensor *parentSense, String UUID = "", String customVarCode = "")
     : Variable(parentSense, KellerAcculevel_PRESSURE_VAR_NUM,
                F("pressureGauge"), F("millibar"),
                KellerAcculevel_PRESSURE_RESOLUTION,
                F("kellerPress"), UUID, customVarCode)
    {}
};


// Defines the Temperature Variable
class KellerAcculevel_Temp : public Variable
{
public:
    KellerAcculevel_Temp(Sensor *parentSense, String UUID = "", String customVarCode = "")
     : Variable(parentSense, KellerAcculevel_TEMP_VAR_NUM,
                F("temperature"), F("degreeCelsius"),
                KellerAcculevel_TEMP_RESOLUTION,
                F("kellerTemp"), UUID, customVarCode)
    {}
};

// Defines the gageHeight (Water level with regard to an arbitrary gage datum) Variable
class KellerAcculevel_Height : public Variable
{
public:
    KellerAcculevel_Height(Sensor *parentSense, String UUID = "", String customVarCode = "")
     : Variable(parentSense, KellerAcculevel_HEIGHT_VAR_NUM,
                F("gageHeight"), F("meter"),
                KellerAcculevel_HEIGHT_RESOLUTION,
                F("kellerHeight"), UUID, customVarCode)
    {}
};

#endif
