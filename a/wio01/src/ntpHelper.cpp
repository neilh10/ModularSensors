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
WiFiUDP udpTime;

//ntpHelper::ntpHelper() {}
//ntpHelper::~ntpHelper() {}

//ntpHelper:: {}
bool ntpHelper::connectToWiFi(const char* ssid, const char* pwd) {
    _ssid=ssid;
    _pwd = pwd;
    return connectToWiFi();
}
bool ntpHelper::connectToWiFi() {
    Serial.println("Connecting to WiFi network: " + String(_ssid)+"/"+String(_pwd));

    // delete old config
    WiFi.disconnect(true);
    Serial.println("Waiting for WIFI connection");
    delay(500);
    //Initiate connection
    WiFi.begin(_ssid, _pwd);
#define CONNECT_RETRYS 20
    uint16_t connect_try=0;
    do {
        if (++connect_try > CONNECT_RETRYS) {
            connect_try =0;
             Serial.print("\n\rRetry disconnect");
            WiFi.disconnect(true);
            delay(500);
            Serial.print(" begin ");
            WiFi.begin(_ssid, _pwd);
            //Serial.print("\n\rRetry");
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
        //initializes the udpTime state
        //This initializes the transfer buffer
        udpTime.begin(WiFi.localIP(), localPort);

        sendNTPpacket(timeServer); // send an NTP packet to a time server
        // wait to see if a reply is available
        delay(1000);
        if (udpTime.parsePacket()) {
            Serial.println("udp packet received");
            Serial.println("");
            // We've received a packet, read the data from it
            udpTime.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

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
            // were not able to parse the udpTime packet successfully
            // clear down the udp connection
            udpTime.stop();
            return 0; // zero indicates a failure
        }
        // not calling ntp time frequently, stop releases resources
        udpTime.stop();
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
    udpTime.beginPacket(address, 123); //NTP requests are to port 123
    udpTime.write(packetBuffer, NTP_PACKET_SIZE);
    return udpTime.endPacket();
}

#include "HTTPClientMmw.h"
#define USE_SERIAL Serial
bool ntpHelper::sendDataTuple(size_t seq_cnt,String timeNow) {
    bool retStatus=false;

    if((WiFi.status() != WL_CONNECTED)) {
        connectToWiFi(); //how to abort if fail
    }
    {

        HTTPClientMmw  http;
        int httpCode;
        String mmwPayload;


        char intStr[10];
        itoa(seq_cnt,intStr,10);
        String seq_num_str = String(intStr);

        mmwPayload = "{\"sampling_feature\":\""+_mmwSamplingFeature+"\",\"timestamp\":\""+timeNow+"\",\"8c57835f-a32f-4d62-82dc-0ba09f04cf52\":"+seq_num_str+",\"3bebd4a3-8b54-4f92-ba55-5fd2fd021358\":3.987,\"03e7b375-97a7-4423-a3f0-1d822d8b19b9\":17.37,\"43bcda9b-2973-4639-af2c-f0b6bb3fa44b\":0.2358,\"08646cc3-c5de-414c-af65-c795b2dcac24\":50.04,\"8849814d-1603-4a2f-861f-f31ae68cccf3\":19.88,\"7182846e-46e0-4a10-b110-9bc32de4aca9\":-25}";


#define TCP_CONNECT_TIMEOUT_MS   5000
#define TCP_RESPONSE_TIMEOUT_MS 10000
        USE_SERIAL.print("Timeouts connect/response ");
        USE_SERIAL.print(TCP_CONNECT_TIMEOUT_MS);
        USE_SERIAL.print(" / ");
        USE_SERIAL.println(TCP_RESPONSE_TIMEOUT_MS);
        http.setConnectTimeout(TCP_CONNECT_TIMEOUT_MS); //default 2000mSIn seconds tv.tv_usec = timeout * 1000;
        http.setTimeout(TCP_RESPONSE_TIMEOUT_MS);      //mS _client->setTimeout((_tcpTimeout + 500) / 1000);

        // configure traged server and url
        String dest_http;
        dest_http = "monitormywatershed.org"; //Connects to server with no http://
        http.begin(dest_http,80,"/api/data-stream/"); //HTTP

        http.addtoken(_mmwToken);
        httpCode = http.POSTmmw(mmwPayload);
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] POST rsp: Code=%d\n", httpCode);
            USE_SERIAL.println(mmwPayload);
            // file found at server
            if(httpCode == HTTP_CODE_CREATED) {
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

