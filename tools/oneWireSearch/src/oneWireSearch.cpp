// oneWireSearch.cpp - with 3wire powered 3V3
// Copyright Neil Hancock 2023
// With insiporation from many places


// see http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html

// Use with WioTerminal, (samd51), PyGamer(samd51), Mayfly (avr1284)
// WioTerminal ST7789_DRIVER)
// PYGAMER -  160x128 Color TFT Display ~  Adafruit_ST7789 *tft 
// Other similar
// https://github.com/ascillato/Tasmota_KNX/blob/7708df7d252e65bd4b2953f364d8d7af2ff26633/tasmota/xsns_05_ds18x20.ino#L196
// https://github.com/NickB1/OpenSpa/blob/4216d5bc16d53f31f43bb6d0ba0ba4c6117950dd/firmware/_openspa/onewire.ino#L10

#include <Arduino.h>
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include "OneWieSeearch_version.h" 

#if defined WIO_TERMINAL
#define ONE_WIRE_BUS 1     // For Wio_T Right side plug, white wire, PB8 works
#elif defined ADAFRUIT_PYGAMER_M4_EXPRESS
//Test - what port to use
#define ONE_WIRE_BUS 1     // For Wio_T Right side plug, white wire, PB8 works
//#elif defined ADAFRUIT_PYGAMER_ADVANCE_M4_EXPRESS
//#define ONE_WIRE_BUS 1     // For Wio_T Right side plug, white wire, PB8 works
#else 
#error need to define ONE WIRE PORT
#endif


#if defined(ARDUINO_ARCH_AVR)
    #define debug  Serial

#elif defined(ARDUINO_ARCH_SAMD) ||  defined(ARDUINO_ARCH_SAM)
    #define debug  SerialUSB
#else
    #define debug  Serial
#endif
#define SerialStd debug

#define USE_DISPLAY
#if defined USE_DISPLAY
#include "uiHelperWioT.h"
uiHelperWioT ui_display;
#endif //USE_DISPLAY

// The defintion of this build
extern const String build_ref = "a\\" __FILE__ " " __DATE__ " " __TIME__ " ";

int count; 
const int32_t serialBaud = 115200;   // Baud rate for debugging

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
 

uint8_t findDevices(int pin)
{
  OneWire ow(pin);

  uint8_t address[8];
  uint8_t count = 0;
  bool ui_displayed= false;
  #define SCREEN_SIZE 100
  String userDisp( SCREEN_SIZE);
  userDisp = "{\"Addr\":[\n"; 

  if (ow.search(address))
  {
    do {
      count++;
      userDisp += "{";
      for (uint8_t i = 0; i < 8; i++)
      {
        userDisp += "0x";
        if (address[i] < 0x10) {
          userDisp += "0";
        };
        userDisp += String(address[i], HEX);
        if (i < 7) {
          userDisp += ",";
        }
        if (3==i){
            userDisp +="\n ";
        }
      }
      userDisp += "},";
      // CHECK CRC
      #if defined CHECK_CRC
      if (ow.crc8(address, 7) == address[7])
      {
        debug.println(" // CRC OK");
      }
      else
      {
        debug.println("// CRC FAILED");
      }
      #endif // CHECK_CRC
      if (userDisp.length() >SCREEN_SIZE-2) {
        ui_display.fillscreen(userDisp.c_str());
        ui_displayed = true;
      }
    } while (ow.search(address));

    userDisp += "\n]};//"+String(count)+" found";
    if (!ui_displayed) {
        ui_display.fillscreen(userDisp.c_str());
    }
    debug.println(userDisp.c_str());

  }

  return count;
}

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{

  uint32_t start_ms=millis();
  debug.begin(serialBaud); // start serial port

  #if defined USE_DISPLAY
  //Do local display first in case not connected on USB
  ui_display.begin();
  ui_display.display_on();
  String uiDisp(build_ref+" vers:"+String(ONEWIRESEARCH_VERSION)+"\nDallas OneWire IC\n search on Ard Pin "+String(ONE_WIRE_BUS));
  ui_display.fillscreen(uiDisp.c_str());
  #endif // USE_DISPLAY

  //Wait for USB connection.
  #define USB_WAIT_FOR_TERM_MS 10000
  while (!debug && ( (millis()-start_ms) < USB_WAIT_FOR_TERM_MS )) {}


  debug.println(F("\n\n---Boot Sw Build: "));
  debug.println(build_ref);
  debug.print("Version:");
  debug.println(ONEWIRESEARCH_VERSION);
  debug.print("  ***** Low Power RTC SAMD51 ");
  debug.print(F_CPU);
  debug.print("MHz ***** ");
  debug.println("//\n// Start oneWireSearch \n//");
  
  debug.print("Dallas OneWire IC search on Ard Pin ");
  debug.println(ONE_WIRE_BUS);


#if defined USE_DISPLAY
  // Wait at least a period for display to be visible
  while ( ((millis()-start_ms) < USB_WAIT_FOR_TERM_MS )) {}
#endif //USE_DISPLAY

  findDevices(ONE_WIRE_BUS);

  // Start up the library
  sensors.begin();

}
 uint32_t sampleNum=0;
/*
 * Main function, get and show the temperature
 */
void loop(void)
{ 
  //Put up header to track how long been running
  debug.print(++sampleNum);
  debug.print("] ");
  // request to all devices on the bus
  sensors.requestTemperatures(); 

  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);
 
  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    debug.print("Temperature (0): ");
    debug.println(tempC);
  } 
  else
  {
    debug.println("Error: Could not read temperature data");
  }
  delay(2000); //Human readable interval
}
