/**
 * @file ntpHelper.cpp
 * @copyright 2022 Neil Hancock
 * Used for Wio Terminal testing,  mostly mashup 
 * @author neil hancock <neilh20@wllw.net>
 *
 * @brief NTP interface
 */

// Included Dependencies
#include "ntpHelper.h"

// define WiFI client
WiFiClient client;

//The udp library class
WiFiUDP udp;

//ntpHelper::ntpHelper() {}
//ntpHelper::~ntpHelper() {}

//ntpHelper:: {}
bool ntpHelper::connectToWiFi(const char* ssid, const char* pwd) {
    Serial.println("Connecting to WiFi network: " + String(ssid)+"/"+String(pwd));

    // delete old config
    WiFi.disconnect(true);
    Serial.println("Waiting for WIFI connection...");
    delay(500);
    //Initiate connection
    WiFi.begin(ssid, pwd);
#define CONNECT_RETRYS 20
    uint16_t connect_try=CONNECT_RETRYS;
    do {
        if (++connect_try > 20) {
            connect_try =0;
            WiFi.disconnect(true);
            delay(500);
            WiFi.begin(ssid, pwd);
            Serial.println("Retry");
        } else {
            Serial.print(".");
        }
        delay(200);
    } while (WiFi.status() != WL_CONNECTED) ;

    Serial.println("Connected.");
    printWifiStatus();

    return true;

}

unsigned long ntpHelper::getNTPtime() {

    // module returns a unsigned long time valus as secs since Jan 1, 1970 
    // unix time or 0 if a problem encounted

    //only send data when connected
    if (WiFi.status() == WL_CONNECTED) {
        //initializes the UDP state
        //This initializes the transfer buffer
        udp.begin(WiFi.localIP(), localPort);

        sendNTPpacket(timeServer); // send an NTP packet to a time server
        // wait to see if a reply is available
        delay(1000);
        if (udp.parsePacket()) {
            Serial.println("udp packet received");
            Serial.println("");
            // We've received a packet, read the data from it
            udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

            //the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, extract the two words:

            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            unsigned long epoch = secsSince1900 - seventyYears;
#if defined ADJUST_TIME
            // adjust time for timezone offset in secs +/- from UTC
            // WA time offset from UTC is +8 hours (28,800 secs)
            // + East of GMT
            // - West of GMT
            long tzOffset = 28800UL;

            // WA local time 
            unsigned long adjustedTime;
            return adjustedTime = epoch + tzOffset;
#else
            return epoch;
#endif
        }
        else {
            // were not able to parse the udp packet successfully
            // clear down the udp connection
            udp.stop();
            return 0; // zero indicates a failure
        }
        // not calling ntp time frequently, stop releases resources
        udp.stop();
    }
    else {
        // network not connected
        return 0;
    }

}
// send an NTP request to the time server at the given address
unsigned long ntpHelper::sendNTPpacket(const char* address) {
    // set all bytes in the buffer to 0
    for (int i = 0; i < NTP_PACKET_SIZE; ++i) {
        packetBuffer[i] = 0;
    }
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    return udp.endPacket();
}

#include "HTTPClientMmw.h"
#define USE_SERIAL Serial
bool ntpHelper::sendDataTuple() {
    bool retStatus=false;

    if((WiFi.status() == WL_CONNECTED)) {

        HTTPClientMmw  http;
        int httpCode;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        http.begin("monitormywatershed.org",0,"/api/data-stream/"); //HTTP
        http.begin("monitormywatershed.org"); //HTTP



        String mmwTest;
        mmwTest = "{\"sampling_feature\":\"12a82902-e312-445a-b607-328a6d4aaa87\",\"timestamp\":\"2022-06-19T03:04:00-08:00\",\"f9f90ef7-745a-44e8-9525-a373b59c28e0\":516,\"c2c6407b-03db-45c4-a736-2cfd0b212b22\":4.063,\"8267249c-614d-4bdf-b161-257ef69b2ee9\":10.54,\"84ce98bc-8a6d-48f0-9d8c-e53c00874dae\":0.0504,\"78a6da23-53d1-48d3-b286-f038fcf94572\":56.39,\"f964780d-87f0-443e-abbc-6089b6deafaf\":10.80,\"c467201d-6abe-4b5a-bde7-9551e0b34bd1\":-69}";
        //USE_SERIAL.print("[HTTP] POST=");



        httpCode = http.POSTmmw(mmwTest);
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] POST rsp: Code=%d\n", httpCode);
            USE_SERIAL.println(mmwTest);
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);
                retStatus=true;
            }
        } else {
            USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return retStatus;

}
void  ntpHelper::printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.println("");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println("");
}
