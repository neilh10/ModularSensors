/*
 *analogElecConductivity.cpp
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 * see .h
*/

#include "analogElecConductivity.h"

// EnviroDIY boards
#if defined(ARDUINO_AVR_ENVIRODIY_MAYFLY)
    #define BOARD "EnviroDIY Mayfly"

// Sodaq boards
#elif defined(ARDUINO_SODAQ_EXPLORER)
    #define BOARD "SODAQ ExpLoRer"
#elif defined(ARDUINO_SODAQ_AUTONOMO)
    #define BOARD "SODAQ Autonomo"
#elif defined(ARDUINO_SODAQ_ONE_BETA)
    #define BOARD "SODAQ ONE Beta"
#elif defined(ARDUINO_SODAQ_ONE)
    #define BOARD "SODAQ ONE"
#elif defined(ARDUINO_AVR_SODAQ_MBILI)
    #define BOARD "SODAQ Mbili"
#elif defined(ARDUINO_AVR_SODAQ_NDOGO)
    #define BOARD "SODAQ Ndogo"
#elif defined(ARDUINO_AVR_SODAQ_TATU)
    #define BOARD "SODAQ Tatu"
#elif defined(ARDUINO_AVR_SODAQ_MOJA)
    #define BOARD "SODAQ Moja"

// Adafruit boards
#elif defined(ARDUINO_AVR_FEATHER32U4)
    #define BOARD "Feather 32u4"
#elif defined(ARDUINO_SAMD_FEATHER_M0)
    #define BOARD "Feather M0"
#elif defined(ARDUINO_SAMD_FEATHER_M0_EXPRESS)
    #define BOARD "Feather M0 Express"
#elif defined(ADAFRUIT_FEATHER_M4_EXPRESS)
    #define BOARD "Feather M4 Express"
// Arduino boards
#elif defined(ARDUINO_AVR_ADK)
    #define BOARD "Mega Adk"
#elif defined(ARDUINO_AVR_BT)    // Bluetooth
    #define BOARD "Bt"
#elif defined(ARDUINO_AVR_DUEMILANOVE)
    #define BOARD "Duemilanove"
#elif defined(ARDUINO_AVR_ESPLORA)
    #define BOARD "Esplora"
#elif defined(ARDUINO_AVR_ETHERNET)
    #define BOARD "Ethernet"
#elif defined(ARDUINO_AVR_FIO)
    #define BOARD "Fio"
#elif defined(ARDUINO_AVR_GEMMA)
    #define BOARD "Gemma"
#elif defined(ARDUINO_AVR_LEONARDO)
    #define BOARD "Leonardo"
#elif defined(ARDUINO_AVR_LILYPAD)
    #define BOARD "Lilypad"
#elif defined(ARDUINO_AVR_LILYPAD_USB)
    #define BOARD "Lilypad Usb"
#elif defined(ARDUINO_AVR_MEGA)
    #define BOARD "Mega"
#elif defined(ARDUINO_AVR_MEGA2560)
    #define BOARD "Mega 2560"
#elif defined(ARDUINO_AVR_MICRO)
    #define BOARD "Micro"
#elif defined(ARDUINO_AVR_MINI)
    #define BOARD "Mini"
#elif defined(ARDUINO_AVR_NANO)
    #define BOARD "Nano"
#elif defined(ARDUINO_AVR_NG)
    #define BOARD "NG"
#elif defined(ARDUINO_AVR_PRO)
    #define BOARD "Pro"
#elif defined(ARDUINO_AVR_ROBOT_CONTROL)
    #define BOARD "Robot Ctrl"
#elif defined(ARDUINO_AVR_ROBOT_MOTOR)
    #define BOARD "Robot Motor"
#elif defined(ARDUINO_AVR_UNO)
    #define BOARD "Uno"
#elif defined(ARDUINO_AVR_YUN)
    #define BOARD "Yun"
#elif defined(ARDUINO_SAMD_ZERO)
    #define BOARD "Zero"

#else
   #define BOARD "Unknown"
   #error BOARD Unknown
#endif


// For Mayfly version because the battery resistor depends on it
analogElecConductivity::analogElecConductivity(int8_t powerPin, int8_t dataPin, uint8_t measurementsToAverage)
    : Sensor(BOARD, ANALOGELECCONDUCTIVITY_NUM_VARIABLES,
             ANALOGELECCONDUCTIVITY_WARM_UP_TIME_MS, ANALOGELECCONDUCTIVITY_STABILIZATION_TIME_MS, ANALOGELECCONDUCTIVITY_MEASUREMENT_TIME_MS,
             powerPin, dataPin, measurementsToAverage)
{
    //_version = version;
    _EcPowerPin = -1;
    _EcAdcPin = -1;
    _ptrWaterTemperature_C = NULL;
   /* #if defined(ARDUINO_AVR_ENVIRODIY_MAYFLY) || defined(ARDUINO_AVR_SODAQ_MBILI)
        _EcAdcPin = A6;
    #elif defined(ARDUINO_AVR_FEATHER32U4) || defined(ARDUINO_SAMD_FEATHER_M0) || defined(ARDUINO_SAMD_FEATHER_M0_EXPRESS)
        _EcAdcPin = 9;
    #elif defined(ADAFRUIT_FEATHER_M4_EXPRESS) 
        _EcAdcPin = A6;//20;  //Dedicated PB01 V_DIV
    #elif defined(ARDUINO_SODAQ_ONE) || defined(ARDUINO_SODAQ_ONE_BETA) || defined(ARDUINO_AVR_SODAQ_NDOGO)
        _EcAdcPin = 10;
    #elif defined(ARDUINO_SODAQ_AUTONOMO)
        if (strcmp(_version, "v0.1") == 0) _EcAdcPin = 48;
        else _EcAdcPin = 33;
    #else
    #error No board defined 
        _EcAdcPin = -1;
    #endif */
}
// Destructor
analogElecConductivity::~analogElecConductivity(){}

String analogElecConductivity::getSensorLocation(void) {return BOARD;}

/************ ***********************
 * SensorsR   
 * 
 * SensorV-- adcPin/Ra --- R1 ---- Sensorconnector&Wire  -- Rwater --- Groond
 * R1 series resistance ~ 500ohms
 * Rwater - Resistenace of Water
 * Ra - Resistance of applied Source - possibly uP Digital Pin
 * 
 * Returns microSiemens the inverse of resistance
 *
 * https://hackaday.io/project/7008-fly-wars-a-hackers-solution-to-world-hunger/log/24646-three-dollar-ec-ppm-meter-arduino
 * http://www.reagecon.com/pdf/technicalpapers/Effect_of_Temperature_TSP-07_Issue3.pdf
*/
float analogElecConductivity::readEC(uint8_t analogPinNum) { 
    uint32_t sensorEC_adc;
    //float sensorEC_V= 0;
    float Rwater_ohms;   //literal value of water
    float EC_uScm,EC25_uScm;  // units are uS per cm

 
    #if defined(ARDUINO_SAMD_FEATHER_M0_EXPRESS) || defined(ADAFRUIT_FEATHER_M4_EXPRESS)
    analogReadResolution(analogElecConductivityDef_Resolution);
    #endif //ARDUINO_SAMD_FEATHER_Mx_
 
    //************Estimates Resistance of Liquid ****************//
    //digitalWrite(_EcPowerPin,HIGH); assume done by class Sensor
    analogRead(analogPinNum);
    sensorEC_adc= analogRead(_EcAdcPin);// This is not a mistake, First reading will be low beause if charged a capacitor
    //digitalWrite(_EcPowerPin,LOW);
 

    //***************** Converts to EC **************************//
 
    //sensorEC_V= (SensorV* sensorEC_adc)/EC_SENSOR_ADC_RANGE;
    //Rwater_ohms=(sensorEC_V*Rseries_ohms)/(SensorV-sensorEC_V);

    /*Assuming sensorEC_adc is ratio metric - adc Reference is same as applied to EC sensor.
    * The Vcc ~ 3.3V can vary, as battery level gets low, so would be nice to elliminate it in the calcs
    * 
    * The current is same through both Rseries and Rwater I =Vrseries/Rseries=Vwater/Rwater
    * [1] Rwater = Rseries * Vwater/Vseries
    * From 
    * Vwater = I*RWater  
    * Vrseries=I*Rseries  or I=Vrseries/Rseries
    * Vwater = RWater  *Vrseries/Rseries
    * [1] Rwater = Rseries *Vwater/Vseries
    * 
    * [2] Vwater = Vref*sensorAdc/ADC_RANGE
    * [3] Vrseries = Vref*(ADC_RANGE-sensorADC)/ADC_RANGE= Vref*(1-sensorADC/ADC_RANGE)
    *  combining [2] & [3] in [1]
    * [4] Rwater = Rseries *Vref*(sensorADC/ADC_RANGE) /(Vref*(1-sensorADC/ADC_RANGE))   
    *  cancel Vref
    *     Rwater = Rseries *sensorADC/(ADC_RANGE(1-sensorADC/ADC_RANGE))
    *     Rwater = Rseries *sensorADC/(ADC_RANGE-sesnosrADC)
    */
    Rwater_ohms = Rseries_ohms*sensorEC_adc/(EC_SENSOR_ADC_RANGE-sensorEC_adc);

    /* The Rwater is an absolute value, but is based on a defined size of the plates of the sensor.
    *  Translating a magic "sensorsEC_konst" is used, derived from others experiments
    */
    EC_uScm = 1000000/(Rwater_ohms*sensorEC_Konst);
 
 
    //*************Compensating For Temperaure********************//
    if (NULL != _ptrWaterTemperature_C) {

        EC25_uScm =  EC_uScm/ (1+ TemperatureCoef*(*_ptrWaterTemperature_C-25.0));
    }else {
        EC25_uScm = EC_uScm;
    }

    return EC25_uScm; 
 
}

bool analogElecConductivity::addSingleMeasurementResult(void)
{


    MS_DBG(F("Getting EC of Water"));

    float sensorEC_uScm = readEC(_EcAdcPin);
    MS_DBG(F("Water EC (uSm/cm"),sensorEC_uScm);
    verifyAndAddMeasurementResult(ANALOGELECCONDUCTIVITY_EC_VAR_NUM, sensorEC_uScm);

    // Unset the time stamp for the beginning of this measurement
    _millisMeasurementRequested = 0;
    // Unset the status bits for a measurement request (bits 5 & 6)
    _sensorStatus &= 0b10011111;

    // Return true when finished
    return true;
}

//void analogElecConductivity::setWaterTemperature(float  WaterTemperature_C) {
//    _WaterTemperature_C = WaterTemperature_C;
//}
void analogElecConductivity::setWaterTemperature(float  *ptrWaterTemperature_C) {
    _ptrWaterTemperature_C = ptrWaterTemperature_C;
}



