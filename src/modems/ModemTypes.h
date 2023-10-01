

#ifndef SRC_MODEMS_MODEMTYPESH_
#define SRC_MODEMS_MODEMTYPESH_

typedef enum {
    MODEMT_NONE=0,
    MODEMT_LTE_DIGI_CATM1=1,
    MODEMT_WIFI_DIGI_S6=  2,
    MODEMT_LORA_XXX_S6=   3,
    MODEMT_LTE_SIM7080=10,
    MODEMT_WIFI_WIOT_RPC=11,
    MODEMT_END
} modemTypes_01_t;
#define modemTypesCurrent_t modemTypes_01_t 
typedef struct {
    uint8_t modemTypseVersion;
    modemTypesCurrent_t modemType; 
} modemTypeTupl_t;
    
#endif  // SRC_MODEMS_MODEMTYPESH_