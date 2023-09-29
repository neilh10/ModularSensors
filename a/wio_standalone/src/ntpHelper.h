
#ifndef SRC_NTP_HELPER_H_
#define SRC_NTP_HELPER_H_
#include "Arduino.h"
//#include <AtWiFi.h>
#include <rpcWiFi.h>
#include "DateTime.h"
const unsigned int localPort = 2390;      // local port to listen for UDP packets
#ifdef USELOCALNTP
    char timeServer[] = "n.n.n.n"; // local NTP server 
#else
    const char timeServer[] = "time.nist.gov"; // extenral NTP server e.g. time.nist.gov
#endif
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

class ntpHelper {
public:
    bool connectToWiFi(const char* ssid, const char* pwd);
    bool connectToWiFi();
    void printWifiStatus();
    unsigned long getNTPtime();
    bool sendDataTuple(size_t seq_cnt,String timeNow="none" );
    //WiFiClient& getStream(void);
    void addToken(String mmwToken) { _mmwToken = mmwToken;}
    void addSamplingFeature(String mmwSamplingFeature) { _mmwSamplingFeature = mmwSamplingFeature;}
uint8_t packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
int8_t _loggerTimeZone = -8;

private :
unsigned long sendNTPpacket(const char* address) ;
    String _mmwToken;
    String _mmwSamplingFeature;
    const char* _ssid;
    const char* _pwd;
};


#endif // SRC_NTP_HELPER_H_
