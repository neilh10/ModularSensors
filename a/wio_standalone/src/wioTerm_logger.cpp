
/* Status: Basic compile in ModularSensors directory/context - not calling MS

 Name:		wioTerm_logger.cpp  from wioTerm_ntp.ino
 Sensors:
 Version:   2.0.1-nh
 Created:	9/7/2020 04:30:00 PM
 Author:	Jim Hamilton modified Neil Hancock Sept/7/2020
 Details:   Example of setting a rtc via ntp using the Wio Terminal

******* Updates *******
Need to check for low power capability - turning off RTL and turning back on
UM0401_RTL872xD_Datasheet_v3.4_watermark.pdf
https://files.seeedstudio.com/products/102110419/Basic%20documents/UM0401_RTL872xD_Datasheet_v3.4_watermark.pdf

Date:
        2023-09-29 0.34.1-aca  builds OK. Has SIM7080G intergration with my fork. 
        2023-02-20 Reliable ntp and POST/MMW (though MMW seems touchy)
        2023-02-19 Failing when turning off RTL. RTL stops responding when turned on
        2023-02-18 POST but not reliably or powering off RTL
        2022-06-16 WioT Compiled
        2020-07-09
        + initial code 

Notes:
        Uses Seeed Arduino RTC SamD51 for DateTime functions and rtc control
        https://github.com/neilh10/Seeed_Arduino_RTC

        Uses millisDelay for non blocking timers
        https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html
        add to lib/
        Example based on the Arduino WifiUdpNtpClient example

        NTP servers can be called via name or ip address, use only servers that can
        repsond to IPv4 requests.

*/

// switch between local and remote time servers
// comment out to use remote server
//#define USELOCALNTP


//https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html
#include <Arduino.h>
#include <millisDelay.h>
#include <Wire.h>
#include "DateTime.h"
//using namespace seeedArduinoRtc_nm ;
#include "RTC_SAMD51.h"

#define RADIO_WIFI
#if defined RADIO_WIFI
#include "ntpHelper.h"
ntpHelper ntph;


#else 
#define ntph
#endif //
#include "uiHelper.h"

extern const String build_ref = "a\\" __FILE__ " " __DATE__ " " __TIME__ " ";

uiHelper ui_display;


#if defined RADIO_WIFI
const char ssid[] = "ArthurGuestSsid"; // add your required ssid
const char password[] = "Arthur8166";//"your-passowrd"; // add your own netywork password
#endif // RADIO_WIFI

RTC_SAMD51 rtcPhy; // Wio Terminal 
DateTime now_dt, bootTime_dt ; // time object

millisDelay updateDelay; //ntp periodic update.




// localtime
unsigned long devicetime;
uint32_t readings_cnt =0;
uint32_t light_reading_raw=0;
float temperature_reading=18.9;
float humidity_reading=23.4;
void readData() {
   light_reading_raw = analogRead(WIO_LIGHT);
   
   //Future 
       //Temperature1 1W

    //Temperature2 Analog
    //Temperature3 Analog

    //AM2302 https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
}


///***************************************************************
#if 0
#include "FreeRam.h"

void printFree() {

    Serial.print("[");
    Serial.print(FreeRam());
    Serial.print("]");
}
uint32_t heap_start;
#else 
#define printFree()
#endif //0
void setup() {

    Serial.begin(115200);

    while (!Serial); // wait for serial port to connect. Needed for native USB
    printFree();
    ui_display.begin();
    ui_display.fillscreen("Modular Sensors");

    Serial.print(F("\n\n\r---Boot("));
    Serial.print(F(") Sw Build: "));
    Serial.println(build_ref);

    if (!rtcPhy.begin()) {
        Serial.println("Couldn't find RTC");
        //while (1) delay(10); // stop operating
    }
    // get and print the current rtc time
    bootTime_dt = rtcPhy.now();
    Serial.print("RTC boot time (UTC): ");
    String bootTime(bootTime_dt.timestamp(DateTime::TIMESTAMP_FULL) );
    Serial.println(bootTime);

    //Serial.printf("RTL8720 Firmware Version: %s\n\r", rpc_system_version());
    Serial.print("RTL8720 Firmware Version:");
    Serial.println( rpc_system_version());

#if defined RADIO_WIFI
    // setup network before rtc check 
    ntph.connectToWiFi(ssid, password);

    // get the time via NTP (udp) call to time server
    // getNTPtime returns epoch UTC time adjusted for timezone but not daylight savings
    // time
    #define LOCAL_TZ  (-1*(8*60*60))
    devicetime = ntph.getNTPtime()+ LOCAL_TZ; //Make PST for simplicity
    rtcPhy.adjust(devicetime);
    now_dt= rtcPhy.now();
#endif // RADIO_WIFI


    // check if rtc has lost power i.e. battery not present or flat or new device
    now_dt = rtcPhy.now();
    //now_dt= bootTime_dt; //zero_sleep_rtc.getEpoch();

    if (! now_dt.isValid() ) {
        Serial.print("RTC lost power, set the time to ");
        // When time needs to be set on a new device, or after a power loss, 
        //DateTime ntp_dt(devicetime);
        DateTime ccTimeTZ(__DATE__, __TIME__);
        rtcPhy.adjust(ccTimeTZ);
        //zero_sleep_rtc.setTime(ccTimeTZ.hour(), ccTimeTZ.minute(), ccTimeTZ.second());
        //zero_sleep_rtc.setDate(ccTimeTZ.date(), ccTimeTZ.month(), ccTimeTZ.year() - 2000);
        //now_dt = zero_sleep_rtc.getEpoch();
        //Serial.println(now_dt.timestamp(DateTime::TIMESTAMP_FULL));        
    }
    // get and print the current rtc time
    //now_dt = rtcPhy.now();
    bootTime_dt = now_dt;
    Serial.print("RTC time is: ");

    Serial.println(now_dt.timestamp(DateTime::TIMESTAMP_FULL));

    // adjust time using ntp time
    //rtcPhy.adjust(DateTime(devicetime));

    // print boot update details
    //Serial.println("RTC (boot) time updated.");
    // get and print the adjusted rtc time
    //now_dt = rtcPhy.now();
    //Serial.print("Adjusted RTC (boot) time is: ");
    //Serial.println(now_dt.timestamp(DateTime::TIMESTAMP_FULL));
    ui_display.fillscreen(now_dt.timestamp(DateTime::TIMESTAMP_FULL).c_str());

    // start millisdelays timers as required, adjust to suit requirements
    //updateDelay.start(12 * 60 * 60 * 1000); // update time via ntp every 12 hrs
    #define UPDATE_MINUTES 1.0
    Serial.print("Update every mins: ");
    Serial.println(UPDATE_MINUTES);
    updateDelay.start(UPDATE_MINUTES*60* 1000); // Firstupdate time via ntp

    Serial.print(" https://monitormywatershed.org/sites/tu_rc_test08/ begin...\n");
    ntph.addToken("0cf7c40a-232e-457d-87d6-cea5c0757fec"); //Test08
    ntph.addSamplingFeature("236c674b-69b9-43af-b0d6-33d67b870ecc");//Test08
         
    //ntph.addToken("8a297ae4-995e-47e5-af03-3faa6a89d79e",false,false); //Test03
    //ntph.addSamplingFeature("12a82902-e312-445a-b607-328a6d4aaa87"); //test03
}

bool firstPass=true;
void loop() {
    //#define TMPBUF_SZ 37
    //char tmpBuf[TMPBUF_SZ];
    String timeNow;

    if (updateDelay.justFinished() || firstPass) { // delay loop
        //rpc_wifi_on();
        //digitalWrite(RTL8720D_CHIP_PU, HIGH); //CHIP_EN high to enable
        //Serial.print(" RTL8720 On ");
        now_dt = rtcPhy.now();
        timeNow = now_dt.timestamp(DateTime::TIMESTAMP_FULL);
        Serial.println(timeNow);

        Serial.print(++readings_cnt);
        Serial.print(":");
        delay(250); //Tboot 200mS
        Serial.print("RTL8720 Check :");
        Serial.print( rpc_system_version());
        //ntph.connectToWiFi(ssid, password);
        Serial.print(":");
        printFree();
        //Serial.print("]");
#define GET_NPT 1
#if defined GET_NPT
        // update rtc time
        unsigned long timeNptTz_sec = ntph.getNTPtime()+LOCAL_TZ ;
        now_dt = rtcPhy.now();
        devicetime =now_dt.unixtime();
        if (timeNptTz_sec   == 0) {
            Serial.println(" Failed to get time from network time server.");
    
        } else {
            if (devicetime !=  timeNptTz_sec) {
                Serial.print(" TimeUpdate difference=");
                Serial.println(devicetime - timeNptTz_sec);
            } else {
                Serial.print(" TimeUpdate sucess.");
            }
        }
        //else 
#endif // GET_NPT
        {
            //rtcPhy.adjust(DateTime(devicetime));
            //Serial.println("");

            //Serial.print("rtc time updated.");
            // get and print the adjusted rtc time
#if 1 //defined RADIO_WIFI
            now_dt = rtcPhy.now();
#else
            now_dt = zero_sleep_rtc.getEpoch();
#endif //RADIO_WIFI
            Serial.print(" time is: ");
            timeNow = now_dt.timestamp(DateTime::TIMESTAMP_FULL) + "-08:00";
            Serial.println(timeNow);
            readData();

            ui_display.update3(now_dt.timestamp(DateTime::TIMESTAMP_FULL).c_str(),temperature_reading,humidity_reading,light_reading_raw );
        }
        if(!ntph.sendDataTuple(readings_cnt,timeNow)) {
            Serial.println(" POST failed");
        }
        if (firstPass) {
            firstPass = false;
        } else {
            //Serial.print(" RTL8720 Off ");
            now_dt = rtcPhy.now();
            timeNow = now_dt.timestamp(DateTime::TIMESTAMP_FULL);
            Serial.println(timeNow);
            //digitalWrite(RTL8720D_CHIP_PU, LOW); //CHIP_EN low to shutdiwn
            updateDelay.repeat(); // timer
        }
    }
}








