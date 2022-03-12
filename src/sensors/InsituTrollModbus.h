/**
 * @file InsituLevelTroll.h - wip. Not working yet.
 * @copyright 2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Written By: Anthony Aufdenkampe <aaufdenkampe@limno.com> and Neil
 * Hancock Edited by Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the Insitu Level Troll sensor subclass and the
 * InsituLevelTroll_Pressure, InsituLevelTroll_Temp, and InsituLevelTroll_Height
 * variable subclasses.
 *
 * These are for InsituLevelTroll level sensors.
 *
 *This file is for Modbus communication to  Insitu Level Troll System Spec 1 and
 *Spec 3, from InSitu Modbus Communication Protocol Version 5.10  ar Target
 *testing on the Insitu LT400
 *
 * It uses the KellerParent as the base
 * Testing - LT500+cable+pigtail to RS485board
 * STATUS: no response has been received from an LT500.
 * It may be that the CRC has to be switched around as Keller was non-standard
 * modbus. Winsitu is used to program LT500 and defaults to 19200 and even parity.
 */
/* clang-format off */
/**
 * @defgroup sensor_leveltroll Insitu Leveltroll
 * Classes for the Insitu Leveltroll.
 *
 * @ingroup insitu_group
 *
 * @tableofcontents
 * @m_footernavigation
 *
 * ??These are for Keller Series 30, Class 5, Group 20 sensors using Modbus
 * communication, that are software version 5.20-12.28 and later (i.e. made
 * after the 2012 in the 28th week).
 *
 * Only tested on the LT500.
 *
 * @section sensor_leveltroll_datasheet Sensor Datasheet
 * 
 * @section sensor_leveltroll_ctor Sensor Constructor
 * {{ @ref InsituLevelTrollXx::InsituLevelTrollXx }}
 *
 * ___
 * @section sensor_leveltroll_examples Example Code
 * The Insitu Leveltroll is used in the @menulink{leveltroll} example.
 *
 * @menusnip{leveltroll}
 */
/* clang-format on */

// Header Guards
#ifndef SRC_SENSORS_INSITULEVELTROLL_H
#define SRC_SENSORS_INSITULEVELTROLL_H

// Included Dependencies
#include "sensors/InsituParent.h"

// Sensor Specific Defines
/** @ingroup sensor_leveltrollxx */
/**@{*/

/**
 * @anchor sensor_leveltrollxx_timing
 * @name Sensor Timing
 * The sensor timing for a Insitu LevelTrollXx
 */
/**@{*/
/// @brief Sensor::_warmUpTime_ms; the LevelTrollXx takes about 500 ms to respond.
#define LEVELTROLL_WARM_UP_TIME_MS 500
/// @brief Sensor::_stabilizationTime_ms; the LevelTrollXx is stable after about
/// 5s (5000ms).
#define LEVELTROLL_STABILIZATION_TIME_MS 5000
///@brief Sensor::_measurementTime_ms; the LevelTrollXx takes 1500ms to complete a
/// measurement.
#define LEVELTROLL_MEASUREMENT_TIME_MS 1500
/**@}*/

/**
 * @anchor sensor_leveltrollxx_pressure
 * @name Pressure
 * The pressure variable from a Insitu LevelTrollXx
 * - Range is 0 to 11 bar
 * - Accuracy is Standard ±0.1% FS, Optional ±0.05% FS
 *
 * {{ @ref InsituLtxx_Pressure::InsituLtxx_Pressure }}
 */
/**@{*/
/// @brief Decimals places in string representation; pressure should have 5 -
/// resolution is 0.002%.
#define LEVELTROLL_PRESSURE_RESOLUTION 5
/// @brief Default variable short code; "insituLtxxPress"
#define ACCULEVEL_PRESSURE_DEFAULT_CODE "insituLtxxPress"
/**@}*/

/**
 * @anchor sensor_leveltrollxx_temp
 * @name Temperature
 * The temperature variable from a  Insitu LevelTrollXx
 * - Range is -10°C to 60°C
 * - Accuracy is not specified in the sensor datasheet
 *
 * {{ @ref InsituLtxx_Temp::InsituLtxx_Temp }}
 */
/**@{*/
/// @brief Decimals places in string representation; temperature should have 2 -
/// resolution is 0.01°C.
#define LEVELTROLL_TEMP_RESOLUTION 2
/// @brief Default variable short code; "insituLtxxTemp"
#define ACCULEVEL_TEMP_DEFAULT_CODE "insituLtxxTemp"
/**@}*/

/**
 * @anchor sensor_leveltrollxx_height
 * @name Height
 * The height variable from a Insitu LevelTrollXx
 * - Range is 0 to 900 feet
 * - Accuracy is Standard ±0.1% FS, Optional ±0.05% FS
 *
 * {{ @ref InsituLtxx_Height::InsituLtxx_Height }}
 */
/**@{*/
/// @brief Decimals places in string representation; height should have 4 -
/// resolution is 0.002%.
#define LEVELTROLL_HEIGHT_RESOLUTION 4
/// @brief Default variable short code; "insituLtxxHeight"
#define ACCULEVEL_HEIGHT_DEFAULT_CODE "insituLtxxHeight"
/**@}*/


/* clang-format off */
/**
 * @brief The Sensor sub-class for the
 * [Insitu leveltroll sensor](@ref sensor_itrolllevel).
 *
 * @ingroup sensor_itrolllevel
 */
/* clang-format on */
class InsituLevelTroll : public InsituParent {
 public:
    // Constructors with overloads
    /**
     * @brief Construct a new Insitu LevelTroll
     *
     * @param modbusAddress The modbus address of the LevelTrollXx.
     * @param stream An Arduino data stream for modbus communication.  See
     * [notes](@ref page_arduino_streams) for more information on what streams
     * can be used.
     * @param powerPin The pin on the mcu controlling power to the LevelTrollXx.
     * Use -1 if it is continuously powered.
     * - The LevelTrollXx requires a 9-28 VDC power supply.
     * @param powerPin2 The pin on the mcu controlling power to the RS485
     * adapter, if it is different from that used to power the sensor.  Use -1
     * or omit if not applicable.
     * @param enablePin The pin on the mcu controlling the direction enable on
     * the RS485 adapter, if necessary; use -1 or omit if not applicable.
     * @note An RS485 adapter with integrated flow control is strongly
     * recommended.
     * @param measurementsToAverage The number of measurements to take and
     * average before giving a "final" result from the sensor; optional with a
     * default value of 1.
     */
    InsituLevelTroll(byte modbusAddress, Stream* stream, int8_t powerPin,
                     int8_t powerPin2 = -1, int8_t enablePin = -1,
                     uint8_t measurementsToAverage = 1)
        : InsituParent(
              modbusAddress, stream, powerPin, powerPin2, enablePin,
              measurementsToAverage, Leveltroll_InsituModel,"InsituLevelTroll", 
              INSITU_NUM_VARIABLES, LEVELTROLL_WARM_UP_TIME_MS,
              LEVELTROLL_STABILIZATION_TIME_MS,LEVELTROLL_MEASUREMENT_TIME_MS) {}
    /**
     * @copydoc InsituLevelTrollXx::InsituLevelTrollXx
     */
    InsituLevelTroll(byte modbusAddress, Stream& stream, int8_t powerPin,
                    int8_t powerPin2 = -1, int8_t enablePin = -1,
                    uint8_t measurementsToAverage = 1)
        : InsituParent(
              modbusAddress, stream, powerPin, powerPin2, enablePin,
              measurementsToAverage, Leveltroll_InsituModel,"InsituLevelTroll", 
              INSITU_NUM_VARIABLES, LEVELTROLL_WARM_UP_TIME_MS,
              LEVELTROLL_STABILIZATION_TIME_MS, LEVELTROLL_MEASUREMENT_TIME_MS) {}
    // Destructor
    ~InsituLevelTroll() {}
};


/* clang-format off */
/**
 * @brief The Variable sub-class used for the
 * [gauge pressure (vented and barometric pressure corrected) output](@ref sensor_leveltrollxx_pressure)
 * from a [Insitu LevelTrollXx](@ref sensor_leveltrollxx).
// Defines the PressureGauge (vented & barometricPressure corrected) variable
 *
 * @ingroup sensor_leveltrollxx
 */
/* clang-format on */
class InsituLevelTroll_Pressure : public Variable {
 public:
    /**
     * @brief Construct a new InsituLtxx_Pressure object.
     *
     * @param parentSense The parent InsituLtxx providing the result
     * values.
     * @param uuid A universally unique identifier (UUID or GUID) for the
     * variable; optional with the default value of an empty string.
     * @param varCode A short code to help identify the variable in files;
     * optional with a default value of "insituLtxxPress".
     */
    InsituLevelTroll_Pressure(
        Sensor* parentSense, const char* uuid = "",
        const char* varCode = "Insitu LTxPress")
        : Variable(parentSense, (const uint8_t)INSITU_PRESSURE_VAR_NUM,
                   (uint8_t)LEVELTROLL_PRESSURE_RESOLUTION,
                   "pressureGauge","millibar", varCode,
                   uuid) {}
    /**
     * @brief Construct a new InsituLtxx_Pressure object.
     *
     * @note This must be tied with a parent InsituLtxx before it can be
     * used.
     */
    InsituLevelTroll_Pressure()
        : Variable((const uint8_t)INSITU_PRESSURE_VAR_NUM,
                   (uint8_t)LEVELTROLL_PRESSURE_RESOLUTION,
                   "pressureGauge","millibar", 
                    "Insitu LTxPress") {}
    /**
     * @brief Destroy the InsituLtxx_Pressure object - no action needed.
     */
    ~InsituLevelTroll_Pressure() {}
};


/* clang-format off */
/**
 * @brief The Variable sub-class used for the
 * [temperature output](@ref sensor_leveltrollxx_temp) from a
 * [Insitu LevelTrollXx](@ref sensor_leveltrollxx).
 *
 * @ingroup sensor_leveltrollxx
 */
/* clang-format on */
class InsituLevelTroll_Temp : public Variable {
 public:
    /**
     * @brief Construct a new InsituLtxx_Temp object.
     *
     * @param parentSense The parent InsituLtxx providing the result
     * values.
     * @param uuid A universally unique identifier (UUID or GUID) for the
     * variable; optional with the default value of an empty string.
     * @param varCode A short code to help identify the variable in files;
     * optional with a default value of "insituLtxxTemp".
     */
    InsituLevelTroll_Temp(
        Sensor* parentSense, const char* uuid = "",
        const char* varCode = "Insitu LTxTemp")
        : Variable(parentSense, (const uint8_t)INSITU_TEMP_VAR_NUM,
                   (uint8_t)LEVELTROLL_TEMP_RESOLUTION, "temperature",
                   "degreeCelsius", varCode, uuid) {}
    /**
     * @brief Construct a new InsituLtxx_Temp object.
     *
     * @note This must be tied with a parent InsituLtxx before it can be
     * used.
     */
    InsituLevelTroll_Temp()
        : Variable((const uint8_t)INSITU_TEMP_VAR_NUM,
                   (uint8_t)LEVELTROLL_TEMP_RESOLUTION, "temperature",
                   "degreeCelsius", "Insitu LTxTemp") {}
    /**
     * @brief Destroy the InsituLtxx_Temp object - no action needed.
     */
    ~InsituLevelTroll_Temp() {}
};


/* clang-format off */
/**
 * @brief The Variable sub-class used for the
 * [gauge height (water level with regard to an arbitrary gage datum) output](@ref sensor_leveltrollxx_height)
 * from a [Insitu LevelTrollXx](@ref sensor_leveltrollxx).
 *
 * @ingroup sensor_leveltrollxx
 */
/* clang-format on */
class InsituLevelTroll_Height : public Variable {
 public:
    /**
     * @brief Construct a new InsituLtxx_Height object.
     *
     * @param parentSense The parent InsituLtxx providing the result
     * values.
     * @param uuid A universally unique identifier (UUID or GUID) for the
     * variable; optional with the default value of an empty string.
     * @param varCode A short code to help identify the variable in files;
     * optional with a default value of "insituLtxxHeight".
     */
    InsituLevelTroll_Height(
        Sensor* parentSense, const char* uuid = "",
        const char* varCode = "InsituLTxHeight")
        : Variable(parentSense, (const uint8_t)INSITU_HEIGHT_VAR_NUM,
                   (uint8_t)LEVELTROLL_HEIGHT_RESOLUTION, "gaugeHeight",
                   "meter", varCode, uuid) {}
    /**
     * @brief Construct a new InsituLtxx_Height object.
     *
     * @note This must be tied with a parent InsituLtxx before it can be
     * used.
     */
    InsituLevelTroll_Height()
        : Variable((const uint8_t)INSITU_HEIGHT_VAR_NUM,
                   (uint8_t)LEVELTROLL_HEIGHT_RESOLUTION, "gaugeHeight",
                   "meter", "InsituLTxHeight") {}
    /**
     * @brief Destroy the InsituLtxx_Height object - no action needed.
     */
    ~InsituLevelTroll_Height() {}
};
/**@}*/
#endif  // SRC_SENSORS_INSITULEVELTROLL_H
