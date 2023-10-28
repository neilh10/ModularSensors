/**
 * @file battery_types.h
 * @copyright 2023 Neil Hancock Assigned Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library 
 * @author Neil Hancock
 *
 * @brief Contains the Battery Types that can be provisioned
 *
 */

typedef enum {
    //Maps to rows in BM_LBATT_TBL
    BMBR_ALL = 0,  // ALL works
    BMBR_0500mA,   // 500mA or less
    BMBR_2000mA,   // 2000mA 

    BMBR_LiSi18,  // LiSiOCL2 19Ahr/larger Pulse 150mA "D" cell - Nomonal 3.6
                  // discharged at 3.2V
    BMBR_3D,      // 3D * 1.6V MnO2 18AHR Pulse ?100mA "D" cell - Nomonal 4.8
                  // discharged at 2.4V
    // 3 MnO2 "C" cell - higher impedance than "D" cell
    BMBR_NUM,  /// Number of Battery types supported
    BMBR_UNDEF,
} bm_battery_type_rating_t;
#define BMBR_LIION BMBR_0500mA
// Default should be all to allow it to power up until set by user
#define BMBR_BAT_TYPE_DEF BMBR_ALL