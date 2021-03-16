/**
 * @author Alessandro Noferi
 *
 * This class create and manages HTTP requests packets, i.e.:
 *
 * METHOD uri HTTP/1.1 \r\n
 * headers[] \r\n
 * \r\n
 * body
 *
 *
 * The required headers for our scope are:
 * - Content-Type
 * They are managed by specific setters and getters.
 * It is also possible add other headers via generic method (setHeaderField)
 *
 */

#ifndef _HTTREQUESTPACKET_H_
#define  _HTTREQUESTPACKET_H_



#include <omnetpp.h>
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/request/HttpRequestPacket_m.h"

enum verb { GET,
              POST,
              DELETE,
              PUT,
            };


/**
 * in HTTPRequestPacket_Base
 * {
 *     \@customize(true);
 *     string method;
 *     string length;
 *     string contentType;
 *     string body;
 *     string payload;
 * }
 */

class HTTPRequestPacket : public HTTPRequestPacket_Base
{
    public:
        HTTPRequestPacket(const char *name=nullptr, short kind=0);
        HTTPRequestPacket(const verb method, const char *name=nullptr, short kind=0);
        HTTPRequestPacket(const char* method, const char *name=nullptr, short kind=0);
        HTTPRequestPacket& operator=(const HTTPRequestPacket& other){if (this==&other) return *this; HTTPRequestPacket_Base::operator=(other); copy(other); return *this;}

        virtual HTTPRequestPacket *dup() const override {return new HTTPRequestPacket(*this);}

        virtual void setMethod(verb method);
        virtual void setContentType(const char * contentType);
        virtual void setLength(const char * length_);
        virtual void setLength(unsigned int length_);

        virtual void setBody(const char * body)override ;
        virtual void setBody(const std::string& body);
        virtual void setHost(const char *host) ;
        virtual void setHost(const std::string& host);
        virtual void setUri(const std::string& uri);
        // key MUST be like "key: "
        virtual void setHeaderField(const std::string& key, const std::string& value);

        virtual const char * getPayload();
    private:
        void copy(const HTTPRequestPacket& other);
        std::map<std::string, std::string> headerFields_;
        ::omnetpp::opp_string httpVersion = "HTTP/1.1";

};

#endif //_HTTREQUESTPACKET_H_
