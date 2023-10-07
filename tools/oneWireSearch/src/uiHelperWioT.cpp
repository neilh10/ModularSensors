/**
 * @file uiHelper.cpp
 * @copyright 2020 Neil Hancock
 * Used for Wio Terminal testing,  mostly mashup 
 * @author neil hancock <neilh20@wllw.net>
 *
 * @brief UI Help
 */

// Included Dependencies
#include "uiHelperWioT.h"


//uiHelperWioT::uiHelperWioT() {}
//uiHelperWioT::~uiHelperWioT() {}

//uiHelperWioT:: {}
//#define SerialStd STANDARD_SERIAL_OUTPUT
#define SerialStd Serial

void uiHelperWioT::begin() {
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(FMB12); 
}


void uiHelperWioT::fillscreen(const char *msg) {
    tft.fillScreen(TFT_BLACK);
    int16_t msg_width = tft.textWidth(msg);
    //Serial.print("uiHelperWioT fillscreen txt width ");
    //Serial.println(msg_width);

    #define WIO_T_SCREEN_X 320
    if (WIO_T_SCREEN_X < msg_width) {
        msg_width = WIO_T_SCREEN_X;
    }

    //tft.setCursor((10, 120);
    tft.setCursor((WIO_T_SCREEN_X-msg_width)/2, 120);    
    // tft.setCursor((320 - tft.textWidth(msg))/2, 120);
    tft.print(msg); 
}
void uiHelperWioT::update6(uiParm6_t *pms) {
// -----------------LCD---------------------
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(FF17);
    tft.setTextColor(tft.color565(224,225,232));
    tft.drawString(*pms->status,20,10);
 	
 // resolution 320x240
 //   status line
 // 6 boxes of Two ROws, with 3 lines each Row
 //    <<<<< status line >>>>>> 
 //    Line1 Row1 , Line1 Row2
 //    Line2 Row1 , Line2 Row2
 //    Line3 Row1 , Line3 Row2
 // Each Box has label font FMB9 
 //   and reading font FMB12
 // Screen Pixel cartisian Reference  (x,y)  from top left 
 // https://wiki.seeedstudio.com/Wio-Terminal-LCD-Basic/
 //  x-->  0....319 ?
 //  y--> (down) 0-239 ?
 // Between each box is 4pixels of no color
 // LMBR1(was 10) Left Margin Begin Row1/Row2
#define LMBR1 2  
#define LMBR2 162
 // LMER1 (was 300) Left Margin End Row1/Ro2
#define LMER1 158
#define LMER2 328

 // ROW_HEIGHT 60  differnt than Line Height
#define LINE_HEIGHT 60
// LINE_HEIGHT_COLORED 55
#define LHC 55
// Corner Radius
#define CR 5
// Background Color
#define CELL_BACKGROUND_COL tft.color565(40,40,86)
#define CELL_TEXT_VALUE_COL tft.color565(224,225,232)

 // Box Row1 Left1 TopLeft Y 45  Row1 Line1 Top Left 
#define L1TLY_BOX 45
 // Row L2TL 105 - Line2 Top Left
#define L2TLY_BOX (L1TLY_BOX+LINE_HEIGHT)
 // L3TL 165
#define L3TLY_BOX (L2TLY_BOX+LINE_HEIGHT)
//LINE TOP LEFT X LABEL 10
#define LINE_R1_TLX_LABEL 10
#define LINE_R2_TLX_LABEL 170
//Line TOP LEFT Y LABEL 50
#define L1TLY_LABEL 50
#define L2TLY_LABEL (L1TLY_LABEL+LINE_HEIGHT)
#define L3TLY_LABEL (L2TLY_LABEL+LINE_HEIGHT)

    tft.fillRoundRect(LMBR1, L1TLY_BOX, LMER1, LHC, CR, CELL_BACKGROUND_COL);
    tft.fillRoundRect(LMBR1, L2TLY_BOX, LMER1, LHC, CR, CELL_BACKGROUND_COL);
    tft.fillRoundRect(LMBR1, L3TLY_BOX, LMER1, LHC, CR, CELL_BACKGROUND_COL);
 
    tft.fillRoundRect(LMBR2, L1TLY_BOX, LMER2, LHC, CR, CELL_BACKGROUND_COL);
    tft.fillRoundRect(LMBR2, L2TLY_BOX, LMER2, LHC, CR, CELL_BACKGROUND_COL);
    tft.fillRoundRect(LMBR2, L3TLY_BOX, LMER2, LHC, CR, CELL_BACKGROUND_COL);

    tft.setFreeFont(FM9);
    tft.drawString("humidity",      LINE_R1_TLX_LABEL, L1TLY_LABEL );
    tft.drawString("temperature2",  LINE_R1_TLX_LABEL, L2TLY_LABEL );
    tft.drawString("temperature4",  LINE_R1_TLX_LABEL, L3TLY_LABEL );
 
    tft.drawString("temperature1",  LINE_R2_TLX_LABEL, L1TLY_LABEL );
    tft.drawString("temperature3",  LINE_R2_TLX_LABEL, L2TLY_LABEL );
    tft.drawString("temperature5",  LINE_R2_TLX_LABEL, L3TLY_LABEL );

    tft.setFreeFont(FMB12);
    //LINE TOP LEFT X VALUE was 140 
    // Max 8 digits for draw float <5>.<2>
    #define ROW1_TLX_VALUE  25
    #define ROW2_TLX_VALUE (ROW1_TLX_VALUE+160)
    // LINE Top Left Y for Value
    #define L1TLY_VALUE 75
    #define L2TLY_VALUE L1TLY_VALUE+LINE_HEIGHT
    #define L3TLY_VALUE L2TLY_VALUE+LINE_HEIGHT
    tft.setTextColor(TFT_GREEN); //TFT_RED
    tft.drawFloat(pms->param1,2 , ROW1_TLX_VALUE, L1TLY_VALUE);
    
    tft.setTextColor(CELL_TEXT_VALUE_COL);
    tft.drawFloat(pms->param3,2 , ROW1_TLX_VALUE, L2TLY_VALUE);

    tft.setTextColor(TFT_GREEN);
    tft.drawFloat(pms->param5,2, ROW1_TLX_VALUE, L3TLY_VALUE);
    //tft.drawNumber(pms->param3, ROW1_TLX_VALUE, L3TLY_VALUE);

    tft.setTextColor(TFT_GREEN);
    tft.drawFloat(pms->param2,2 , ROW2_TLX_VALUE, L1TLY_VALUE);
    
    tft.setTextColor(CELL_TEXT_VALUE_COL);
    tft.drawFloat(pms->param4,2 , ROW2_TLX_VALUE, L2TLY_VALUE);

    tft.setTextColor(TFT_GREEN);
    tft.drawFloat(pms->param6,2, ROW2_TLX_VALUE, L3TLY_VALUE);


    //Line Top Left X Units now 150 (assuming 8 pxiels wide) to edge of box. 1was 210
#define LINE_R1_TLX_UNITS 140
    tft.drawString("%", LINE_R1_TLX_UNITS, L1TLY_VALUE);
    tft.drawString("C", LINE_R1_TLX_UNITS, L2TLY_VALUE);
    tft.drawString("C", LINE_R1_TLX_UNITS, L3TLY_VALUE);

#define LINE_R2_TLX_UNITS 300
    tft.drawString("C", LINE_R2_TLX_UNITS, L1TLY_VALUE);
    tft.drawString("C", LINE_R2_TLX_UNITS, L2TLY_VALUE);
    tft.drawString("C", LINE_R2_TLX_UNITS, L3TLY_VALUE);

    //tft.drawString("%",210, 195);
}

void uiHelperWioT::update3(String status, float param1,float param2, float param3) {
}

// https://github.com/Bodmer/TFT_eSPI/issues/671
void uiHelperWioT::display_off(bool force) {
    if (!_display_active_state || force) {
        SerialStd.print(" UiHelper::display_off. Backlight=");
        SerialStd.println(tft.backlight());
        tft.writecommand(0x10); // Sleep
        delay(5); // Delay for shutdown time before another command can be sent
        _display_active_state = false;
        tft.setBacklight(0);
    } else {
        SerialStd.println(" UiHelper::display_off already");
    }
}
void uiHelperWioT::display_on(bool force) {
    if (!_display_active_state || force) {
        tft.writecommand(0x11); // Wake display
        delay(120); // Delay for pwer supplies to stabilise
        _display_active_state = false;
        tft.setBacklight(0xffff);
        //SerialStd.println(" UiHelper::display_on turnedOn");
    } else {
        //SerialStd.println(" UiHelper::display_on already");
    }
}
bool uiHelperWioT::display_state() {
    return _display_active_state;
};







