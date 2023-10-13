/**
 * @file SensirionSHT3x.cpp
 * @copyright 2023 Neil Hancock, modified from SensironSHT4x
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the SensirionSHT3x class.
 */

#include "SensirionSHT3x.h"


// The constructors
SensirionSHT3x::SensirionSHT3x(TwoWire* theI2C, int8_t powerPin, bool useHeater,
                               uint8_t measurementsToAverage)
    : Sensor("SensirionSHT3x", SHT3X_NUM_VARIABLES, SHT3X_WARM_UP_TIME_MS,
             SHT3X_STABILIZATION_TIME_MS, SHT3X_MEASUREMENT_TIME_MS, powerPin,
             -1, measurementsToAverage),
      _useHeater(useHeater),
      _i2c(theI2C) {}
SensirionSHT3x::SensirionSHT3x(int8_t powerPin, bool useHeater,
                               uint8_t measurementsToAverage)
    : Sensor("SensirionSHT3x", SHT3X_NUM_VARIABLES, SHT3X_WARM_UP_TIME_MS,
             SHT3X_STABILIZATION_TIME_MS, SHT3X_MEASUREMENT_TIME_MS, powerPin,
             -1, measurementsToAverage, SHT3X_INC_CALC_VARIABLES),
      _useHeater(useHeater),
      _i2c(&Wire) {}
// Destructor
SensirionSHT3x::~SensirionSHT3x() {}


String SensirionSHT3x::getSensorLocation(void) {
    return F("I2C_0x44"); //Hardwired with SHT31_DEFAULT_ADDR
}


bool SensirionSHT3x::setup(void) {
    _i2c->begin();  // Start the wire library (sensor power not required)

    // Timeouts allow for any error on the wire to be handled
    _i2c->setTimeout(20);

    bool retVal =
        Sensor::setup();  // this will set pin modes and the setup status bit
    
    MS_DBG(getSensorNameAndLocation(), F(" setup:"),retVal);

    // This sensor needs power for setup!
    // The SHT3x's begin() does a soft reset to check for sensor communication.
    bool wasOn = checkPowerOn();
    if (!wasOn) { powerUp(); }
    waitForWarmUp();

    // Run begin fxn because it returns true or false for success in contact
    uint8_t ntries  = 0;
    bool    success = false;
    while (!success && ntries < 5) {
        success = sht3x_internal.begin();
        ntries++;

    }

    sht3x_internal.heater(false);    

    // Set status bits
    if (!success) {
        // Set the status error bit (bit 7)
        _sensorStatus |= 0b10000000;
        // UN-set the set-up bit (bit 0) since setup failed!
        _sensorStatus &= 0b11111110;
         MS_DBG(getSensorNameAndLocation(), F(" begin failed after tries "),ntries);
    }
    retVal &= success;

    // Turn the power back off it it had been turned on
    if (!wasOn) { powerDown(); }

    return retVal;
}


bool SensirionSHT3x::addSingleMeasurementResult(void) {
    // Initialize float variables
    float temp_val  = -9999;
    float humid_val = -9999;
    bool  ret_val   = false;

    // For *successfully*measurement started (status bit 6 set)
    if (bitRead(_sensorStatus, 6)) {
        MS_DBG(getSensorNameAndLocation(), F("is reporting:"));

        #ifdef MS_SENSIRION_SHT3X_DEBUG 
        // heater should be OFF - check if ON but only in debug when looking 
        if (sht3x_internal.isHeaterEnabled()) {
            sht3x_internal.heater(false);
            MS_DBG(getSensorNameAndLocation(), F(" heater ERROR, was on "));
        }
        #endif //MS_SENSIRION_SHT3X_DEBUG 

        ret_val  = sht3x_internal.readBoth(&temp_val,&humid_val);


        if (!ret_val || isnan(temp_val)) temp_val = -9999;
        if (!ret_val || isnan(humid_val)) humid_val = -9999;

        MS_DBG(F("  Temp:"), temp_val, F("°C"));
        MS_DBG(F("  Humidity:"), humid_val, '%');
    } else {
        MS_DBG(getSensorNameAndLocation(), F("is not currently measuring!"));
    }

    verifyAndAddMeasurementResult(SHT3X_TEMP_VAR_NUM, temp_val);
    verifyAndAddMeasurementResult(SHT3X_HUMIDITY_VAR_NUM, humid_val);

    // Unset the time stamp for the beginning of this measurement
    _millisMeasurementRequested = 0;
    // Unset the status bits for a measurement request (bits 5 & 6)
    _sensorStatus &= 0b10011111;

    return ret_val;
}


// The function to run the internal heater before going to sleep
// Turning on heater **may** evaporate any moisture before next read
bool SensirionSHT3x::sleep(void) {
    if (!_useHeater) { return Sensor::sleep(); }

    if (!checkPowerOn()) { return true; }
    if (_millisSensorActivated == 0) {
        MS_DBG(getSensorNameAndLocation(), F("was not measuring!"));
        return true;
    }

    bool success = true;
    MS_DBG(F("Running heater on"), getSensorNameAndLocation(),
           F("at maximum for 1s to remove condensation."));


    // Generate a short heat burst to possible evaporate any moisture
    sht3x_internal.heater(true); 

    #define HEATER_ON_TIME_MS 1000
    unsigned long startMillis = millis();
    do {
        yield(); // running TinyUSB task
    } while(millis() - startMillis < HEATER_ON_TIME_MS);

    // Save power !!
    sht3x_internal.heater(false);

    if (success) {
        // Unset the activation time
        _millisSensorActivated = 0;
        // Unset the measurement request time
        _millisMeasurementRequested = 0;
        // Unset the status bits for sensor activation (bits 3 & 4) and
        // measurement request (bits 5 & 6)
        _sensorStatus &= 0b10000111;
        MS_DBG(F("Done"));
    } else {
        MS_DBG(getSensorNameAndLocation(),
               F("did not successfully run the heater."));
    }

    return success;
}
