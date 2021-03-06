/**
 * @TIINA219M.cpp
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Written By: Neil Hancock
 *
 * @brief Implements the TIINA219 class.
 */

#include "TIINA219M.h"

// The constructors
TIINA219M::TIINA219M(TwoWire* theI2C, int8_t powerPin, uint8_t i2cAddressHex,
                   uint8_t measurementsToAverage)
    : Sensor("TIINA219", INA219_NUM_VARIABLES, INA219_WARM_UP_TIME_MS,
             INA219_STABILIZATION_TIME_MS, INA219_MEASUREMENT_TIME_MS, powerPin,
             -1, measurementsToAverage) {
    _i2cAddressHex = i2cAddressHex;
    _i2c           = theI2C;
}
TIINA219M::TIINA219M(int8_t powerPin, uint8_t i2cAddressHex,
                   uint8_t measurementsToAverage)
    : Sensor("TIINA219M", INA219_NUM_VARIABLES, INA219_WARM_UP_TIME_MS,
             INA219_STABILIZATION_TIME_MS, INA219_MEASUREMENT_TIME_MS, powerPin,
             -1, measurementsToAverage) {
    _i2cAddressHex = i2cAddressHex;
    _i2c           = &Wire;
    _ina219_pollmask    = INA219_POLLMASK_ALL;
    _ampMult            = 1.0;
    _voltLowThreshold_V = 0;
    _thresholdAlertFxn  = NULL;
}
// Destructor
TIINA219M::~TIINA219M() {}


String TIINA219M::getSensorLocation(void) {
    String address = F("I2C_0x");
    address += String(_i2cAddressHex, HEX);
    return address;
}


bool TIINA219M::setup(void) {
    bool wasOn;
    Sensor::setup();  // this will set pin modes and the setup status bit

    // This sensor needs power for setup!
    wasOn = checkPowerOn();
    if (!wasOn) {
        powerUp();
        waitForWarmUp();
    }

    ina219_phy.begin(_i2c);

    // Turn the power back off it it had been turned on
    if (!wasOn) { powerDown(); }

    return true;
}


bool TIINA219M::wake(void) {
    // Sensor::wake() checks if the power pin is on and sets the wake timestamp
    // and status bits.  If it returns false, there's no reason to go on.
    if (!Sensor::wake()) {
        MS_DBG(F("Sensor Err"));
        return false;
    }

    // Begin/Init needs to be rerun after every power-up to set the calibration
    // coefficient for the INA219 (see p21 of datasheet)
    MS_DBG(F("Wake"));
    ina219_phy.begin(_i2c,INA219_RANGE_32V_1A);
    //ina219_phy.begin(&Wire, INA219_RANGE_32V_1A);

    return true;
}


bool TIINA219M::addSingleMeasurementResult(void) {
    bool success = false;

    // Initialize float variables
    float current_mA = -9999;
    float busV_V     = -9999;
    float power_mW   = -9999;

    // Check a measurement was *successfully* started (status bit 6 set)
    // Only go on to get a result if it was
    if (bitRead(_sensorStatus, 6)) {
        MS_DBG(getSensorNameAndLocation(), F("is reporting:"));

        // Read values - turn on internal ADC
        ina219_phy.powerSave(false);

        // Collect data - starting with BusVolatage as it has a check for
        // Conversion Ready.
        if (INA219_POLLMASK_V & _ina219_pollmask) {
            uint8_t  cnvrStatus;
            uint16_t cnvrCnt = 1;  // Breakout counter
            do {
                busV_V     = ina219_phy.getBusVoltage_V();
                cnvrStatus = ina219_phy.getLastCnvrStatus();
                if (cnvrStatus) break;
                MS_DBG(F("CNVR Status check "), cnvrCnt);
                delay(10);
            } while (500 < ++cnvrCnt);
            if (isnan(busV_V)) busV_V = -9999;
            MS_DBG(F("  V, BusV: "), busV_V);
            verifyAndAddMeasurementResult(INA219_BUS_VOLTAGE_VAR_NUM, busV_V);
        }
        if (INA219_POLLMASK_A & _ina219_pollmask) {
            current_mA = (ina219_phy.getCurrent_mA() * _ampMult);
            // current_mA = (ina219_phy.getCurrent_mA());
            if (isnan(current_mA)) current_mA = -9999;
            MS_DBG(F("  mA, current: "), current_mA);
            verifyAndAddMeasurementResult(INA219_CURRENT_MA_VAR_NUM,
                                          current_mA);
        }
        if (INA219_POLLMASK_W & _ina219_pollmask) {
            power_mW = ina219_phy.getPower_mW();
            if (isnan(power_mW)) power_mW = -9999;
            MS_DBG(F("  mW, Power: "), power_mW);
            verifyAndAddMeasurementResult(INA219_POWER_MW_VAR_NUM, power_mW);
        }
        ina219_phy.powerSave(true);  // Turn off continuous conversion
        success = true;

        // MS_DBG(F("  Current [mA]:"), current_mA);
        // MS_DBG(F("  Bus Voltage [V]:"), busV_V);
        // MS_DBG(F("  Power [mW]:"), power_mW);
    } else
        MS_DBG(getSensorNameAndLocation(), F("is not currently measuring!"));

    // Unset the time stamp for the beginning of this measurement
    _millisMeasurementRequested = 0;
    // Unset the status bits for a measurement request (bits 5 & 6)
    _sensorStatus &= 0b10011111;

    return success;
}


// Az extensions

void TIINA219M::set_active_sensors(uint8_t sensors_mask) {
    _ina219_pollmask = sensors_mask;
}

uint8_t TIINA219M::which_sensors_active() {
    return _ina219_pollmask;
}


void TIINA219M::setCustomAmpMult(float newAmpMult) {
    _ampMult = newAmpMult;
}

float TIINA219M::getCustomAmpMult(void) {
    return _ampMult;
}

void TIINA219M::setCustomVoltThreshold(
    float voltLowThreshold_V,
    void (*thresholdAlertFxn)(bool exceed, float value_V)) {
    _voltLowThreshold_V = voltLowThreshold_V;
    _thresholdAlertFxn  = thresholdAlertFxn;
}

float TIINA219M::getCustomVoltThreshold(void) {
    return _voltLowThreshold_V;
}
// End
