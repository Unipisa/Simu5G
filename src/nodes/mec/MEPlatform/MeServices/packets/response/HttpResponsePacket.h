/**
 * @author Alessandro Noferi
 *
 * This class create and manages HTTP responses packets, i.e.:
 *
 * HTTP/1.1 STATUS \r\n
 * headers[] \r\n
 * \r\n
 * body
 *
 *
 * The required headers for our scope are:
 * - Content-Type
 * - Connection
 * They are managed by specific setters and getters.
 * It is also possible add other headers via generic method (setHeaderField)
 *
 */

#ifndef _HTTRESPONSEPACKET_H_
#define  _HTTRESPONSEPACKET_H_



#include <omnetpp.h>
#include <string>
#include "nodes/mec/MEPlatform/MeServices/packets/response/HttpResponsePacket_m.h"

enum responseCode { OK,         // 200
                    CREATED,    // 201
                    NO_CONTENT, // 204
                    BAD_REQ,    // 400
                    UNAUTH,     // 401
                    FORBIDDEN,  // 403
                    NOT_FOUND,  // 404
                    NOT_ACC,    // 406
                    TOO_REQS,   // 429
                    BAD_METHOD, // 405
                    HTTP_NOT_SUPPORTED, // 505
                    SERV_UNAV    //503
                  };


/**
 * in HTTPResponsePacket_Base
 * {
 *     \@customize(true);
 *     string status;
 *     string connection;
 *     string contentType;
 *     string body;
 *     string payload;
 * }
 */

class HTTPResponsePacket : public HTTPResponsePacket_Base
{
    public:
        HTTPResponsePacket(const char *name=nullptr, short kind=0);
        HTTPResponsePacket(const responseCode res, const char *name=nullptr, short kind=0);
        HTTPResponsePacket& operator=(const HTTPResponsePacket& other){if (this==&other) return *this; HTTPResponsePacket_Base::operator=(other); copy(other); return *this;}

        virtual HTTPResponsePacket *dup() const override {return new HTTPResponsePacket(*this);}

//        virtual void setStatus(const char * status);
        virtual void setStatus(const responseCode res);
        virtual void setConnection(const char * connection_);
        virtual void setContentType(const char * contentType_);
        virtual void setBody(const char * body_) override;
        virtual void setBody(const std::string& body_);
        virtual void setHeaderField(const std::string& key, const std::string& value);

        virtual const char * getPayload();
    private:
          void copy(const HTTPResponsePacket& other);
          std::map<std::string, std::string> headerFields_;
          ::omnetpp::opp_string httpVersion_ = "HTTP/1.1 ";

};

#endif //_HTTRESPONSEPACKET_H_
