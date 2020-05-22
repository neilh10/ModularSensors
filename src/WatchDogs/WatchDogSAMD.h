/**
 * @file WatchDogSAMD.h
 * @copyright 2020 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the extendedWatchDogSAMD class.
 *
 * Code for this is taken from the Adafruit SleepyDog library:
 * https://github.com/adafruit/Adafruit_SleepyDog/ and this library:
 * https://github.com/javos65/WDTZero
 *
 * @copydetails extendedWatchDogSAMD
 */

// Header Guards
#ifndef SRC_WATCHDOGS_WATCHDOGSAMD_H_
#define SRC_WATCHDOGS_WATCHDOGSAMD_H_

// Debugging Statement
// #define MS_WATCHDOGSAMD_DEBUG

#ifdef MS_WATCHDOGSAMD_DEBUG
#define MS_DEBUGGING_STD "WatchDogSAMD"
#endif

// Included Dependencies
#include "ModSensorDebugger.h"
#undef MS_DEBUGGING_STD


void WDT_Handler(void);  // ISR HANDLER FOR WDT EW INTERRUPT

class extendedWatchDogSAMD {
 public:
    // Constructor
    extendedWatchDogSAMD();
    ~extendedWatchDogSAMD();

    // One-time initialization of watchdog timer.
    void setupWatchDog(uint32_t resetTime_s);
    void enableWatchDog();
    void disableWatchDog();

    void resetWatchDog();

    static volatile uint32_t _barksUntilReset;

 private:
    void inline waitForWDTBitSync();
    uint32_t _resetTime_s;
};

#endif  // SRC_WATCHDOGS_WATCHDOGSAMD_H_
