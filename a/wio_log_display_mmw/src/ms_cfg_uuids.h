// Defaults for data.envirodiy.org
//Test08 https://monitormywatershed.org/sites/tu_rc_test08/
#define LOGGERID_DEF_STR "tu_rc_test08"
#if defined UUID_HARDCODED
#define registrationToken_UUID "0cf7c40a-232e-457d-87d6-cea5c0757fec"
#define samplingFeature_UUID   "236c674b-69b9-43af-b0d6-33d67b870ecc"

#define SEQUENCE_NUMBER_UUID   "8c57835f-a32f-4d62-82dc-0ba09f04cf52"

#define TEMPERATURE_ALL_DS18 1
#define TEMPERATURE_A_UUID     "03e7b375-97a7-4423-a3f0-1d822d8b19b9"
#define TEMPERATURE_B_UUID     "c62fcd8a-406e-4fe1-87d9-ff3dca8e1b90"
#define TEMPERATURE_C_UUID     "43bcda9b-2973-4639-af2c-f0b6bb3fa44b"
#define TEMPERATURE_D_UUID     "ff4d732d-88d8-4a1b-b499-16417603edfe"
#define BAT_VOLTAGE_UUID       "3bebd4a3-8b54-4f92-ba55-5fd2fd021358"


#if defined ASONG_AM23XX_UUID 
#define ASONG_AM23_Air_Temperature_UUID "8849814d-1603-4a2f-861f-f31ae68cccf3"
#define ASONG_AM23_Air_Humidity_UUID    "08646cc3-c5de-414c-af65-c795b2dcac24"
#endif  // ASONG_AM23XX_UUID

#else
#define registrationToken_UUID "registrationToken_UUID"
#define samplingFeature_UUID   "samplingFeature_UUID"

#define SEQUENCE_NUMBER_UUID   "SampleNumber_UUID"
#define BAT_VOLTAGE_UUID       "Batt_V_UUID"
#define BAT_Ahr_UUID       "Batt_Ahr_UUID"

#if defined ASONG_AM23XX_UUID 
#define ASONG_AM23_Air_Temperature_UUID "Air_Temperature_UUID"
#define ASONG_AM23_Air_Humidity_UUID "Air_Humidity_UUID"
#endif //ASONG_AM23XX_UUID 

#define TEMPERATURE_ALL_DS18 1
#define TEMPERATURE_A_UUID     "DS18A_UUID"
#define TEMPERATURE_B_UUID     "DS18B_UUID"
#define TEMPERATURE_C_UUID     "DS18C_UUID"
#define TEMPERATURE_D_UUID     "DS18D_UUID"

#endif // UUID_HARDCODED