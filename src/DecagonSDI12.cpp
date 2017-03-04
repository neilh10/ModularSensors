/*
 *DecagonSDI12.cpp
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Work in progress by Sara Damiano taken from code written
 *by Shannon Hicks and templates from USU.

 *
 *This file is for the Decagon Devices 5TM Soil Moisture probe
 *It is dependent on the EnviroDIY SDI-12 library.
*/

#include "DecagonSDI12.h"

// The constructor - need the SDI-12 address, the power pin, and the data pin
DecagonSDI12::DecagonSDI12(char SDI12address, int powerPin, int dataPin, int numReadings)
    : SensorBase(dataPin, powerPin)
{
    _SDI12address = SDI12address;
    _numReadings = numReadings;
    setup();
}

// A helper functeion to run the "sensor info" SDI12 command
void DecagonSDI12::getSensorInfo(void)
{
    SDI12 mySDI12(_dataPin);
    mySDI12.begin();
    delay(500); // allow things to settle

    // Check if the power is on, turn it on if not
    bool wasOn = checkPowerOn();
    if(!wasOn){powerUp();}

   // Serial.println(F("Getting sensor info"));  // For debugging
    myCommand = "";
    myCommand += (char) _SDI12address;
    myCommand += "I!"; // sends 'info' command [address][I][!]
    mySDI12.sendCommand(myCommand);
   // Serial.println(myCommand);  // For debugging
    delay(30);

    // wait for acknowlegement with format:
    // [address][SDI12 support (2 char)][vendor (8 char)][model (6 char)][version (3 char)][serial number (<14 char)]
    sdiResponse = "";
    while (mySDI12.available())  // build response string
    {
        char c = mySDI12.read();
        if ((c != '\n') && (c != '\r'))
        {
            sdiResponse += c;
            delay(5);
        }
    }
   // if (sdiResponse.length() > 1) Serial.println(sdiResponse);  // For debugging
    _sensorName = sdiResponse.substring(3,17);
    _sensorName.trim();
    _sensorVendor = sdiResponse.substring(3,11);
    _sensorVendor.trim();
    _sensorModel = sdiResponse.substring(11,17);
    _sensorModel.trim();
    _sensorVersion = sdiResponse.substring(17,20);
    _sensorVersion.trim();
    _sensorSerialNumber = sdiResponse.substring(20,17);
    _sensorSerialNumber.trim();
    mySDI12.flush();

    // Turn the power back off it it had been turned on
    if(!wasOn){powerDown();}

}

// The sensor name
// String DecagonSDI12::getSensorName(void)
// {return _sensorName;}

// The sensor name
String DecagonSDI12::getSensorVendor(void)
{return _sensorVendor;}

// The sensor name
String DecagonSDI12::getSensorModel(void)
{return _sensorModel;}

// The sensor name
String DecagonSDI12::getSensorVersion(void)
{return _sensorVersion;}

// The sensor name
String DecagonSDI12::getSensorSerialNumber(void)
{return _sensorSerialNumber;}


// The sensor installation location on the Mayfly
String DecagonSDI12::getSensorLocation(void)
{
    sensorLocation = String(_SDI12address) + "_" + String(_dataPin);
    return sensorLocation;
}

int DecagonSDI12::numMeasurements = 0;
float DecagonSDI12::sensorValues[9] = {0};  // Know that all Decagon SDI12 sensors will return 9 or fewer measurements

// Uses SDI-12 to communicate with a Decagon Devices 5TM
bool DecagonSDI12::update()
{
    SDI12 mySDI12(_dataPin);
    mySDI12.begin();
    delay(500); // allow things to settle

    // Check if the power is on, turn it on if not
    bool wasOn = checkPowerOn();
    if(!wasOn){powerUp();}

    // Clear values before starting loop
    for (int i = 0; i <9; i++)
    { DecagonSDI12::sensorValues[i] =  0; }

    // averages x readings in this one loop
    for (int j = 0; j < _numReadings; j++)
    {
       // Serial.print(F("Taking reading #"));  // For debugging
       // Serial.println(j);  // For debugging
        myCommand = "";
        myCommand += _SDI12address;
        myCommand += "M!"; // SDI-12 measurement myCommand format  [address]['M'][!]
        mySDI12.sendCommand(myCommand);
       // Serial.println(myCommand);  // For debugging
        delay(30);

        // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of measurments available, 0-9]
        sdiResponse = "";
        while (mySDI12.available())  // build response string
        {
            char c = mySDI12.read();
            if ((c != '\n') && (c != '\r'))
            {
                sdiResponse += c;
                delay(5);
            }
        }
       // if (sdiResponse.length() > 1) Serial.println(sdiResponse);  // For debugging
        mySDI12.flush();

        // find out how long we have to wait (in seconds).
        unsigned int wait = 0;
        wait = sdiResponse.substring(1,4).toInt();
       // Serial.print(F("Waiting "));  // For debugging
       // Serial.print(wait);  // For debugging
       // Serial.println(F(" seconds for measurement"));  // For debugging

        // Set up the number of results to expect
        DecagonSDI12::numMeasurements =  sdiResponse.substring(4,5).toInt();
       // Serial.print(DecagonSDI12::numMeasurements);  // For debugging
       // Serial.println(F(" results expected"));  // For debugging

        unsigned long timerStart = millis();
        while((millis() - timerStart) < (1000 * wait))
        {
            if(mySDI12.available())  // sensor can interrupt us to let us know it is done early
            {
               // Serial.println("Wait interrupted!");  // For debugging
                mySDI12.flush();
                break;
            }
        }
        delay(30);
        mySDI12.flush();

       // Serial.println(F("Requesting data"));  // For debugging
        myCommand = "";
        myCommand += _SDI12address;
        myCommand += "D0!";  // SDI-12 command to get data [address][D][dataOption][!]
        mySDI12.sendCommand(myCommand);
       // Serial.println(myCommand);  // For debugging
        delay(30);

       // Serial.println(F("Receiving data"));  // For debugging
        mySDI12.read();  // ignore the repeated SDI12 address
        for (int i = 0; i < DecagonSDI12::numMeasurements; i++)
        {
            float result = mySDI12.parseFloat();
            DecagonSDI12::sensorValues[i] += result;
           // Serial.print(F("Result #"));  // For debugging
           // Serial.print(i);  // For debugging
           // Serial.print(F(": "));  // For debugging
           // Serial.println(result);  // For debugging
        }
        // Wait for and discard anything else
        mySDI12.flush();
    }

    // Average over the number of readings
   // Serial.println(F("Averaging over readings"));  // For debugging
    for (int i = 0; i < DecagonSDI12::numMeasurements; i++)
    {
        DecagonSDI12::sensorValues[i] /=  _numReadings;
       // Serial.print(F("Result #"));  // For debugging
       // Serial.print(i);  // For debugging
       // Serial.print(F(": "));  // For debugging
       // Serial.println(sensorValues[i]);  // For debugging
    }

    // Turn the power back off it it had been turned on
    if(!wasOn){powerDown();}

    // Return true when finished
    return true;
}