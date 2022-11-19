
#include "HTTPClientMmw.h"

/**
 * constructor
 */
HTTPClientMmw::HTTPClientMmw()
{
}

/**
 * destructor
 */
HTTPClientMmw::~HTTPClientMmw()
{
    if(_client) {
        _client->stop();
    }
    if(_currentHeaders) {
        delete[] _currentHeaders;
    }
}

/**
 * sends a post request to the server
 * @param payload uint8_t *
 * @param size size_t
 * @return http code
 */
int HTTPClientMmw::POSTmmw(uint8_t * payload, size_t size)
{
    return sendRequestMmw("POST", payload, size);
}

int HTTPClientMmw::POSTmmw(String payload)
{
    return POSTmmw((uint8_t *) payload.c_str(), payload.length());
}

/*
On POST MonitorMyWatershed the following format is required 
NOTE line TOKEN with the specific channel.

POST /api/data-stream/ HTTP/1.1
Host: monitormywatershed.org
TOKEN: 8a297ae4-995e-47e5-af03-3faa6a89d79e
Content-Length: nnn
Content-Type: application/json
<content nnn bytes> 

*/

/**
 * sendRequest
 * @param type const char *     "GET", "POST", ....
 * @param payload String        data for the message body
 * @return
 */
int HTTPClientMmw::sendRequestMmw(const char * type, String payload)
{
    return sendRequestMmw(type, (uint8_t *) payload.c_str(), payload.length());
}

/**
 * sendRequest
 * @param type const char *     "GET", "POST", ....
 * @param payload uint8_t *     data for the message body if null not send
 * @param size size_t           size for the message body if 0 not send
 * @return -1 if no info or > 0 when Content-Length is set by server
 */
int HTTPClientMmw::sendRequestMmw(const char * type, uint8_t * payload, size_t size_payload)
{
    int code;
    bool redirect = false;
    uint16_t redirectCount = 0;
    do {
        // wipe out any existing headers from previous request
        for(size_t i = 0; i < _headerKeysCount; i++) {
            if (_currentHeaders[i].value.length() > 0) {
                // _currentHeaders[i].value.clear();
                _currentHeaders[i].value.remove(0, sizeof(_currentHeaders[i].value));
            }
        }

        log_d("request type: '%s' redirCount: %d\n", type, redirectCount);
        
        // connect to server
        if(!connect()) {
            Serial.println("POST FAIL>>");
            Serial.print(header1);
            //Serial.println(_headers);
            Serial.println("<<<");
            return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
        }


        if(payload && size_payload > 0) {
            //addHeader(F("\n\rTOKEN"), "8a297ae4-995e-47e5-af03-3faa6a89d79e",false,false); //Test03
            addHeader(F("\n\rTOKEN"), "0cf7c40a-232e-457d-87d6-cea5c0757fec",false,false); //Test08
            addHeader(F("Content-Length"), String(size_payload),false,false);
            addHeader(F("Content-Type"), "application/json",false,false);
            log_d("created header\n");
        } else {
            Serial.println("payload invalid");
        }

        // send Header
        if(!sendHeaderMmw(type)) {
            return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
        }

        // send Payload if needed
        if(payload && size_payload > 0) {
            // Send in chunks of HTTP_TCP_BUFFER_SIZE bytes            
            for (size_t pos = 0; pos < size_payload; pos += HTTP_TCP_BUFFER_SIZE) {
                size_t to_write = min(HTTP_TCP_BUFFER_SIZE, size_payload - pos);
                if(_client->write(&payload[pos], to_write) != to_write) {
                    return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
                }
            }
            log_d(">>\n\r%s", _headers.c_str() );
            if ( (size_payload<499)) {
                //log_d limitations
                log_d("\n\r%s\n\r<<",payload);
            }/**/
        }

        code = handleHeaderResponse();
        log_d("sendRequest code=%d\n", code);

        // Handle redirections as stated in RFC document:
        // https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
        //
        // Implementing HTTP_CODE_FOUND as redirection with GET method,
        // to follow most of existing user agent implementations.
        //
        redirect = false;
        if (
            _followRedirects != HTTPC_DISABLE_FOLLOW_REDIRECTS && 
            redirectCount < _redirectLimit &&
            _location.length() > 0
        ) {
            switch (code) {
                // redirecting using the same method
                case HTTP_CODE_MOVED_PERMANENTLY:
                case HTTP_CODE_TEMPORARY_REDIRECT: {
                    if (
                        // allow to force redirections on other methods
                        // (the RFC require user to accept the redirection)
                        _followRedirects == HTTPC_FORCE_FOLLOW_REDIRECTS ||
                        // allow GET and HEAD methods without force
                        !strcmp(type, "GET") || 
                        !strcmp(type, "HEAD")
                    ) {
                        redirectCount += 1;
                        log_d("following redirect (the same method): '%s' redirCount: %d\n", _location.c_str(), redirectCount);
                        if (!setURL(_location)) {
                            log_d("failed setting URL for redirection\n");
                            // no redirection
                            break;
                        }
                        // redirect using the same request method and payload, diffrent URL
                        redirect = true;
                    }
                    break;
                }
                // redirecting with method dropped to GET or HEAD
                // note: it does not need `HTTPC_FORCE_FOLLOW_REDIRECTS` for any method
                case HTTP_CODE_FOUND:
                case HTTP_CODE_SEE_OTHER: {
                    redirectCount += 1;
                    log_d("following redirect (dropped to GET/HEAD): '%s' redirCount: %d\n", _location.c_str(), redirectCount);
                    if (!setURL(_location)) {
                        log_d("failed setting URL for redirection\n");
                        // no redirection
                        break;
                    }
                    // redirect after changing method to GET/HEAD and dropping payload
                    type = "GET";
                    payload = nullptr;
                    size_payload = 0;
                    redirect = true;
                    break;
                }

                default:
                    break;
            }
        }

    } while (redirect);
    // handle Server Response (Header)
    return returnError(code);
}

/**
 * sendRequest
 * @param type const char *     "GET", "POST", ....
 * @param stream Stream *       data stream for the message body
 * @param size size_t           size for the message body if 0 not Content-Length is send
 * @return -1 if no info or > 0 when Content-Length is set by server
 */
int HTTPClientMmw::sendRequestMmw(const char * type, Stream * stream, size_t size)
{

    if(!stream) {
        return returnError(HTTPC_ERROR_NO_STREAM);
    }

    // connect to server
    if(!connect()) {
        return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
    }

    if(size > 0) {
        addHeader("Content-Length", String(size));
    }

    // send Header
    if(!sendHeader(type)) {
        return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
    }

    int buff_size = HTTP_TCP_BUFFER_SIZE;

    int len = size;
    int bytesWritten = 0;

    if(len == 0) {
        len = -1;
    }

    // if possible create smaller buffer then HTTP_TCP_BUFFER_SIZE
    if((len > 0) && (len < HTTP_TCP_BUFFER_SIZE)) {
        buff_size = len;
    }

    // create buffer for read
    uint8_t * buff = (uint8_t *) malloc(buff_size);

    if(buff) {
        // read all data from stream and send it to server
        while(connected() && (stream->available() > -1) && (len > 0 || len == -1)) {

            // get available data size
            int sizeAvailable = stream->available();

            if(sizeAvailable) {

                int readBytes = sizeAvailable;

                // read only the asked bytes
                if(len > 0 && readBytes > len) {
                    readBytes = len;
                }

                // not read more the buffer can handle
                if(readBytes > buff_size) {
                    readBytes = buff_size;
                }

                // read data
                int bytesRead = stream->readBytes(buff, readBytes);

                // write it to Stream
                int bytesWrite = _client->write((const uint8_t *) buff, bytesRead);
                bytesWritten += bytesWrite;

                // are all Bytes a writen to stream ?
                if(bytesWrite != bytesRead) {
                    log_d("short write, asked for %d but got %d retry...", bytesRead, bytesWrite);

                    // check for write error
                    if(_client->getWriteError()) {
                        log_d("stream write error %d", _client->getWriteError());

                        //reset write error for retry
                        _client->clearWriteError();
                    }

                    // some time for the stream
                    delay(1);

                    int leftBytes = (readBytes - bytesWrite);

                    // retry to send the missed bytes
                    bytesWrite = _client->write((const uint8_t *) (buff + bytesWrite), leftBytes);
                    bytesWritten += bytesWrite;

                    if(bytesWrite != leftBytes) {
                        // failed again
                        log_d("short write, asked for %d but got %d failed.", leftBytes, bytesWrite);
                        free(buff);
                        return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
                    }
                }

                // check for write error
                if(_client->getWriteError()) {
                    log_d("stream write error %d", _client->getWriteError());
                    free(buff);
                    return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
                }

                // count bytes to read left
                if(len > 0) {
                    len -= readBytes;
                }

                delay(0);
            } else {
                delay(1);
            }
        }

        free(buff);

        if(size && (int) size != bytesWritten) {
            log_d("Stream payload bytesWritten %d and size %d mismatch!.", bytesWritten, size);
            log_d("ERROR SEND PAYLOAD FAILED!");
            return returnError(HTTPC_ERROR_SEND_PAYLOAD_FAILED);
        } else {
            log_d("Stream payload written: %d", bytesWritten);
        }

    } else {
        log_d("too less ram! need %d", HTTP_TCP_BUFFER_SIZE);
        return returnError(HTTPC_ERROR_TOO_LESS_RAM);
    }

    // handle Server Response (Header)
    return returnError(handleHeaderResponse());
}

/**
 * sends HTTP request header
 * @param type (GET, POST, ...)
 * @return status
 */
bool HTTPClientMmw::sendHeaderMmw(const char * type)
{
    if(!connected()) {
        return false;
    }

    header1 = String(type) + " " + _uri + F(" HTTP/1.");

    if(_useHTTP10) {
        header1 += "0";
    } else {
        header1 += "1";
    }

    header1 += String(F("\r\nHost: ")) + _host;

    if (_port != 80 && _port != 443)
    {
        header1 += ':';
        header1 += String(_port);
    }

    /*header1 += String(F("\r\nUser-Agent: ")) + _userAgent +
              F("\r\nConnection: ");

    if(_reuse) {
        header1 += F("keep-alive");
    } else {
        header1 += F("close");
    }
    header1 += "\r\n";
    if(!_useHTTP10) {
        header1 += F("Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n");
    }

    if(_base64Authorization.length()) {
        _base64Authorization.replace("\n", "");
        header1 += F("Authorization: Basic ");
        header1 += _base64Authorization;
        header1 += "\r\n";
    }
*/
    header1 += _headers;
    // This may be too big for log_d
    //log_d("Header%d>>\n\r%s\n\r<<end",header1.length(), header1.c_str());
    return (_client->write((const uint8_t *) header1.c_str(), header1.length()) == header1.length());
}
