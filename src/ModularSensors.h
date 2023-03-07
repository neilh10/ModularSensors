/**
 * @file ModularSensors.h
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief A simple include file for the Arduino command line interface (CLI).s
 */

// Header Guards
#ifndef SRC_MODULARSENSORS_H_
#define SRC_MODULARSENSORS_H_

/**
 * @brief The current library version number
 * 
 * https://semver.org/ 
 * This fork uses the pre-release version and will always be slightly ahead 
 * of the envirodiy master branch that it is based on. 
 * An hypen '-' and alpha number for tracking this fork's release 
 */
#define MODULAR_SENSORS_VERSION "0.34.1-abb"

// To get all of the base classes for ModularSensors, include LoggerBase.
// NOTE:  Individual sensor definitions must be included separately.
#include "LoggerBase.h"

#endif  // SRC_MODULARSENSORS_H_
