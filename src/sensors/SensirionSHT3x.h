/**
 * @file SensirionSHT3x.h
 * @copyright 2023 Neil Hancock, modified from SensironSHT4x
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the SensirionSHT3x sensor subclass and the variable
 * subclasses SensirionSHT3x_Humidity and SensirionSHT3x_Temp.
 *
 * These are used for the Sensirion SHT30, SHT31, and SHT35 capacitive humidity
 * and temperature sensor.
 *
 * This depends on the [Adafruit SHT31
 * library](https://github.com/adafruit/Adafruit_SHT31).
 */
/* clang-format off */
/**
 * @defgroup sensor_sht3x Sensirion SHT30, SHT31, and SHT35
 * Classes for the Sensirion SHT30, SHT31, and SHT35 I2C humidity and temperature sensors.
 *
 * @ingroup the_sensors
 *
 * @tableofcontents
 * @m_footernavigation
 *
 * @section sensor_sht3x_intro Introduction
 *
 * > SHT3x is a digital sensor platform for measuring relative humidity and
 * > temperature at different accuracy classes. The I2C interface  provides
 * > several preconfigured I2C addresses and maintains an ultra-low power
 * > budget. The power-trimmed internal heater can be used at three heating
 * > levels thus enabling sensor operation in demanding environments.
 *

 * - The code for this library should work for 0x44 addressed SHT3x sensors - the SHT30, SHT31, and SHT35.
 * - Depends on the [Adafruit SHT31 Library](https://github.com/adafruit/Adafruit_SHT31).
 * - Communicates via I2C
 *   - The SHT3x,  can be set to I2C address 0x44 or 0x45
 * 
 * **This library only supports the 0x44 addressed sensor!**
 *
 * - **Only 1 can be connected to a single I2C bus at a time**
 * - Requires a 3.3 power source
 *
 * @note Software I2C is *not* supported for the SHT3x.
 * A secondary hardware I2C on a SAMD board is supported.
 *
 * @section sensor_sht3x_datasheet Sensor Datasheet
 * [Datasheet](https://cdn-shop.adafruit.com/product-files/5064/5064_Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf)
 *
 * @section sensor_sht3x_ctor Sensor Constructors
 * {{ @ref SensirionSHT3x::SensirionSHT3x(int8_t, bool, uint8_t) }}
 * {{ @ref SensirionSHT3x::SensirionSHT3x(TwoWire*, int8_t, bool, uint8_t) }}
 *
 * @section sensor_sht3x_examples Example Code
 *
 * The SHT30 is used in the @menulink{sensirion_sht3x} example
 *
 * @menusnip{sensirion_sht3x}
 */
/* clang-format on */

// Header Guards
#ifndef SRC_SENSORS_SENSIRIONSHT3X_H_
#define SRC_SENSORS_SENSIRIONSHT3X_H_

// Debugging Statement
// #define MS_SENSIRION_SHT3X_DEBUG

#ifdef MS_SENSIRION_SHT3X_DEBUG
#define MS_DEBUGGING_STD "SensirionSHT3x"
#endif

// Included Dependencies
#include "ModSensorDebugger.h"
#undef MS_DEBUGGING_STD
#include "VariableBase.h"
#include "SensorBase.h"
#include <Adafruit_SHT31.h>

/** @ingroup sensor_sht3x */
/**@{*/

// Sensor Specific Defines
/// @brief Sensor::_numReturnedValues; the SHT3x can report 2 values.
#define SHT3X_NUM_VARIABLES 2
/// @brief Sensor::_incCalcValues; we don't calculate any additional values.
#define SHT3X_INC_CALC_VARIABLES 0

/**
 * @anchor sensor_sht3x_timing
 * @name Sensor Timing
 * The sensor timing for an Sensirion SHT3x
 */
/**@{*/
/// @brief Sensor::_warmUpTime_ms; SHT3x warms up in 0.3ms (typical) and
/// soft-resets in 1ms (max).
#define SHT3X_WARM_UP_TIME_MS 1
/// @brief Sensor::_stabilizationTime_ms; SHT3x is assumed to be immediately
/// stable.
#define SHT3X_STABILIZATION_TIME_MS 0
/// @brief Sensor::_measurementTime_ms; SHT3x takes 8.2ms (max) to complete a
/// measurement at the highest precision.  At medium precision measurement time
/// is 4.5ms (max) and it is 1.7ms (max) at low precision.
#define SHT3X_MEASUREMENT_TIME_MS 9
/**@}*/

/**
 * @anchor sensor_sht3x_humidity
 * @name Humidity
 * The humidity variable from an Sensirion SHT3x
 * - Range is 0 to 100% RH
 * - Accuracy is ± 1.8 % RH (typical)
 *
 * {{ @ref SensirionSHT3x_Humidity::SensirionSHT3x_Humidity }}
 */
/**@{*/
/**
 * @brief Decimals places in string representation; humidity should have 2 (0.01
 * % RH).
 *
 * @note This resolution is some-what silly in light of the ± 1.8 % RH accuracy.
 */
#define SHT3X_HUMIDITY_RESOLUTION 2
/// @brief Sensor variable number; humidity is stored in sensorValues[0].
#define SHT3X_HUMIDITY_VAR_NUM 0
/// @brief Variable name in
/// [ODM2 controlled vocabulary](http://vocabulary.odm2.org/variablename/);
/// "relativeHumidity"
#define SHT3X_HUMIDITY_VAR_NAME "relativeHumidity"
/// @brief Variable unit name in
/// [ODM2 controlled vocabulary](http://vocabulary.odm2.org/units/); "percent"
/// (percent relative humidity)
#define SHT3X_HUMIDITY_UNIT_NAME "percent"
/// @brief Default variable short code; "SHT3xHumidity"
#define SHT3X_HUMIDITY_DEFAULT_CODE "SHT3xHumidity"
/**@}*/

/**
 * @anchor sensor_sht3x_temperature
 * @name Temperature
 * The temperature variable from an Sensirion SHT3x
 * - Range is -40°C to +125°C
 * - Accuracy is ±0.2°C
 *
 * {{ @ref SensirionSHT3x_Temp::SensirionSHT3x_Temp }}
 */
/**@{*/
/**
 * @brief Decimals places in string representation; humidity should have 2 (0.01
 * °C).
 *
 * @note This resolution is some-what silly in light of the ± 0.2°C accuracy.
 */
#define SHT3X_TEMP_RESOLUTION 2
/// @brief Sensor variable number; temperature is stored in sensorValues[1].
#define SHT3X_TEMP_VAR_NUM 1
/// @brief Variable name in
/// [ODM2 controlled vocabulary](http://vocabulary.odm2.org/variablename/);
/// "temperature"
#define SHT3X_TEMP_VAR_NAME "temperature"
/// @brief Variable unit name in
/// [ODM2 controlled vocabulary](http://vocabulary.odm2.org/units/);
/// "degreeCelsius" (°C)
#define SHT3X_TEMP_UNIT_NAME "degreeCelsius"
/// @brief Default variable short code; "SHT3xTemp"
#define SHT3X_TEMP_DEFAULT_CODE "SHT3xTemp"
/**@}*/


/* clang-format off */
/**
 * @brief The Sensor sub-class for the [Sensirion SHT3x](@ref sensor_sht3x).
 */
/* clang-format on */
class SensirionSHT3x : public Sensor {
 public:
    /**
     * @brief Construct a new SensirionSHT3x object using a secondary *hardware*
     * I2C instance.
     *
     * This is only applicable to SAMD boards that are able to have multiple
     * hardware I2C ports in use via SERCOMs.
     *
     * @note It is only possible to connect *one* SHT3x or SHT4x at a time on a single
     * I2C bus.  Only the 0x44 addressed version is supported.
     *
     * @param theI2C A TwoWire instance for I2C communication.  Due to the
     * limitations of the Arduino core, only a hardware I2C instance can be
     * used.  For an AVR board, there is only one I2C instance possible and this
     * form of the constructor should not be used.  For a SAMD board, this can
     * be used if a secondary I2C port is created on one of the extra SERCOMs.
     * @param powerPin The pin on the mcu controlling power to the Sensirion
     * SHT3x.  Use -1 if it is continuously powered.
     * - The SHT3x requires a 3.3V power source
     * @param useHeater Whether or not to run the internal heater of the SHT3x
     * when shutting down the sensor; optional with a default value of true. The
     * internal heater is designed to remove condensed water from the sensor -
     * which will make the sensor stop responding to air humidity changes - and
     * to allow creep-free operation in high humidity environments.  The longest
     * the internal heater can run at a time is 1s and the maximum duty load is
     * 5%.  Running only 1s per measurment cycle probably isn't enough to help
     * with more than very minimal condensation, but it's probably the best we
     * can easily do.
     * @param measurementsToAverage The number of measurements to take and
     * average before giving a "final" result from the sensor; optional with a
     * default value of 1.
     */
    SensirionSHT3x(TwoWire* theI2C, int8_t powerPin, bool useHeater = true,
                   uint8_t measurementsToAverage = 1);
    /**
     * @brief Construct a new SensirionSHT3x object using the primary hardware
     * I2C instance.
     *
     * Because this is I2C and has only 1 possible address (0x44), we only need
     * the power pin.
     *
     * @note It is only possible to connect *one* SHT3x at a time on a single
     * I2C bus.  Only the 0x44 addressed version is supported.
     *
     * @param powerPin The pin on the mcu controlling power to the Sensirion
     * SHT3x.  Use -1 if it is continuously powered.
     * - The SHT3x requires a 3.3V power source
     * @param useHeater Whether or not to run the internal heater of the SHT3x
     * when shutting down the sensor; optional with a default value of true. The
     * internal heater is designed to remove condensed water from the sensor -
     * which will make the sensor stop responding to air humidity changes - and
     * to allow creep-free operation in high humidity environments.  The longest
     * the internal heater can run at a time is 1s and the maximum duty load is
     * 5%.  Running only 1s per measurment cycle probably isn't enough to help
     * with more than very minimal condensation, but it's probably the best we
     * can easily do.
     * @param measurementsToAverage The number of measurements to take and
     * average before giving a "final" result from the sensor; optional with a
     * default value of 1.
     */
    explicit SensirionSHT3x(int8_t powerPin, bool useHeater = true,
                            uint8_t measurementsToAverage = 1);
    /**
     * @brief Destroy the SensirionSHT3x object - no action needed.
     */
    ~SensirionSHT3x();

    /**
     * @brief Report the I2C address of the SHT3x - which is always 0x44.
     *
     * @return **String** Text describing how the sensor is attached to the mcu.
     */
    String getSensorLocation(void) override;

    /**
     * @brief Do any one-time preparations needed before the sensor will be able
     * to take readings.
     *
     * This sets the #_powerPin mode, begins the Wire library (sets pin levels
     * and modes for I2C), and updates the #_sensorStatus.  No sensor power is
     * required.
     *
     * @return **bool** True if the setup was successful.
     */
    bool setup(void) override;

    /**
     * @copydoc Sensor::addSingleMeasurementResult()
     */
    bool addSingleMeasurementResult(void) override;

    /**
     * @copydoc Sensor::sleep()
     *
     * If opted for, we run the SHT3x's internal heater for 1s before going to
     * sleep.
     */
    bool sleep(void) override;

 private:
    /**
     * @brief Internal variable for the heating setting
     */
    bool _useHeater;
    /**
     * @brief Internal reference the the Adafruit BME object
     */
    Adafruit_SHT31 sht3x_internal;
    /**
     * @brief An internal reference to the hardware Wire instance.
     */
    TwoWire* _i2c;
};


/* clang-format off */
/**
 * @brief The Variable sub-class used for the
 * [relative humidity output](@ref sensor_sht3x_humidity) from an
 * [Sensirion SHT3x](@ref sensor_sht3x).
 */
/* clang-format on */
class SensirionSHT3x_Humidity : public Variable {
 public:
    /**
     * @brief Construct a new SensirionSHT3x_Humidity object.
     *
     * @param parentSense The parent SensirionSHT3x providing the result
     * values.
     * @param uuid A universally unique identifier (UUID or GUID) for the
     * variable; optional with the default value of an empty string.
     * @param varCode A short code to help identify the variable in files;
     * optional with a default value of "SHT3xHumidity".
     */
    explicit SensirionSHT3x_Humidity(
        SensirionSHT3x* parentSense, const char* uuid = "",
        const char* varCode = SHT3X_HUMIDITY_DEFAULT_CODE)
        : Variable(parentSense, (const uint8_t)SHT3X_HUMIDITY_VAR_NUM,
                   (uint8_t)SHT3X_HUMIDITY_RESOLUTION, SHT3X_HUMIDITY_VAR_NAME,
                   SHT3X_HUMIDITY_UNIT_NAME, varCode, uuid) {}
    /**
     * @brief Construct a new SensirionSHT3x_Humidity object.
     *
     * @note This must be tied with a parent SensirionSHT3x before it can be
     * used.
     */
    SensirionSHT3x_Humidity()
        : Variable((const uint8_t)SHT3X_HUMIDITY_VAR_NUM,
                   (uint8_t)SHT3X_HUMIDITY_RESOLUTION, SHT3X_HUMIDITY_VAR_NAME,
                   SHT3X_HUMIDITY_UNIT_NAME, SHT3X_HUMIDITY_DEFAULT_CODE) {}
    /**
     * @brief Destroy the SensirionSHT3x_Humidity object - no action needed.
     */
    ~SensirionSHT3x_Humidity() {}
};


/* clang-format off */
/**
 * @brief The Variable sub-class used for the
 * [temperature output](@ref sensor_sht3x_temperature) from an
 * [Sensirion SHTx0](@ref sensor_sht3x).
 */
/* clang-format on */
class SensirionSHT3x_Temp : public Variable {
 public:
    /**
     * @brief Construct a new SensirionSHT3x_Temp object.
     *
     * @param parentSense The parent SensirionSHT3x providing the result
     * values.
     * @param uuid A universally unique identifier (UUID or GUID) for the
     * variable; optional with the default value of an empty string.
     * @param varCode A short code to help identify the variable in files;
     * optional with a default value of "SHT3xTemp".
     */
    explicit SensirionSHT3x_Temp(SensirionSHT3x* parentSense,
                                 const char*     uuid = "",
                                 const char* varCode  = SHT3X_TEMP_DEFAULT_CODE)
        : Variable(parentSense, (const uint8_t)SHT3X_TEMP_VAR_NUM,
                   (uint8_t)SHT3X_TEMP_RESOLUTION, SHT3X_TEMP_VAR_NAME,
                   SHT3X_TEMP_UNIT_NAME, varCode, uuid) {}
    /**
     * @brief Construct a new SensirionSHT3x_Temp object.
     *
     * @note This must be tied with a parent SensirionSHT3x before it can be
     * used.
     */
    SensirionSHT3x_Temp()
        : Variable((const uint8_t)SHT3X_TEMP_VAR_NUM,
                   (uint8_t)SHT3X_TEMP_RESOLUTION, SHT3X_TEMP_VAR_NAME,
                   SHT3X_TEMP_UNIT_NAME, SHT3X_TEMP_DEFAULT_CODE) {}
    /**
     * @brief Destroy the SensirionSHT3x_Temp object - no action needed.
     */
    ~SensirionSHT3x_Temp() {}
};
/**@}*/
#endif  // SRC_SENSORS_SENSIRIONSHT3X_H_
