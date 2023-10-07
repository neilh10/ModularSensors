
#ifndef SRC_UI_HELPER_WIOT_H_
#define SRC_UI_HELPER_WIOT_H_
#include "Arduino.h"
#include"Free_Fonts.h"
#include"TFT_eSPI.h"

typedef struct uiParm6_t {
    String *status;
    float param1;
    float param2;
    float param3;
    float param4;
    float param5;
    float param6;
} uiParm6_t;

class uiHelperWioT {
public:
void begin();
void fillscreen(const char *msg);
void update3(String status, float param1=0.0,float param2=0.0, float param3=0.0);
void update6(uiParm6_t *parms);

void display_off(bool force=false);
void display_on(bool force=false);
bool display_state();
// false display off
// true display on
bool _display_active_state=false;
TFT_eSPI tft;
private :


};


#endif // SRC_UI_HELPER_WIOT_H_
