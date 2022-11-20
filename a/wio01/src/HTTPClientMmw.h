#include <HTTPClient.h>

#ifndef HTTPClientMmw_H_
#define HTTPClientMmw_H_
class HTTPClientMmw : public HTTPClient 
{
public:
    HTTPClientMmw();
    ~HTTPClientMmw();

    int POSTmmw(uint8_t * payload, size_t size);
    int POSTmmw(String payload);
    void addtoken(String token_mmw) {_token_mmw = token_mmw;}
    int sendRequestMmw(const char * type, String payload);
    int sendRequestMmw(const char * type, uint8_t * payload = NULL, size_t size = 0);
    int sendRequestMmw(const char * type, Stream * stream, size_t size = 0);

    //void addHeaderMmw(const String& name, const String& value, bool first = false, bool replace = true);
    bool sendHeaderMmw(const char * type);
    String _token_mmw;
    String header1;
};

#endif // HTTPClientMmw_H_