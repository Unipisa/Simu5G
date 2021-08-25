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

#ifndef _HTTREQUESTMESSAGE_H_
#define  _HTTREQUESTMESSAGE_H_



#include <omnetpp.h>
#include <string>
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"



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




class HttpRequestMessage : public HttpRequestMessage_m
{
    public:
        HttpRequestMessage(const char *name=nullptr, short kind=0);
        HttpRequestMessage(const std::string method, const char *name=nullptr, short kind=0);
        HttpRequestMessage(const char* method, const char *name=nullptr, short kind=0);
        HttpRequestMessage& operator=(const HttpRequestMessage& other){if (this==&other) return *this; HttpRequestMessage_m::operator=(other); copy(other); return *this;}

        virtual HttpRequestMessage *dup() const override {return new HttpRequestMessage(*this);}

        virtual void addBodyChunk(const std::string& bodyChunk);
        // key MUST be like "key: "
        virtual void setHeaderField(const std::string& key, const std::string& value);

        virtual bool isBackgroundRequest() const { return isBackgroundRequest_;}
        virtual bool isLastBackgroundRequest() const { return isLastBackgroundRequest_;}

        virtual void setBackGroundRequest(bool bgReq) {isBackgroundRequest_ = bgReq;}
        virtual void setLastBackGroundRequest(bool bgReq)
        {
            if(bgReq == true)
                isBackgroundRequest_ = true;
            isLastBackgroundRequest_ = bgReq;
        }

        virtual std::string getPayload() const;
    private:
        void copy(const HttpRequestMessage& other);
        std::map<std::string, std::string> headerFields_;

        bool isBackgroundRequest_;
        bool isLastBackgroundRequest_;

};

#endif //_HTTREQUESTMESSAGE_H_
