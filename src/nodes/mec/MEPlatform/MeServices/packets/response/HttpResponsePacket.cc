#include "nodes/mec/MEPlatform/MeServices/packets/response/HttpResponsePacket.h"

#include "inet/common/INETDefs.h"



HTTPResponsePacket::HTTPResponsePacket(const char *name, short kind): HTTPResponsePacket_Base(name,kind)
{
    setContentType("application/json");
    setConnection("close");
    setBody("");
    setPayload("");
}
HTTPResponsePacket::HTTPResponsePacket(const responseCode res, const char *name, short kind): HTTPResponsePacket_Base(name, kind)
{
    setStatus(res);
    setContentType("application/json");
    setConnection("close");
    setBody("");
    setPayload("");
}

void HTTPResponsePacket::setStatus(const responseCode res){
    switch(res) {
        case(OK):
        this->status = "200 OK";
            break;
        case(CREATED):
        this->status = "201 Created";
            break;
        case(NO_CONTENT):
        this->status = "204 No Content";
            break;

        case(BAD_REQ):
        this->status = "400 BadRequest";
            break;
        case(UNAUTH):
        this->status = "401 Unauthorized";
            break;
        case(FORBIDDEN):
        this->status = "403 Forbidden";
            break;
        case(NOT_FOUND):
        this->status = "404 Not Found";
            break;
        case(NOT_ACC):
        this->status = "406 Not Acceptable";
            break;
        case(TOO_REQS):
        this->status = "429 Too Many Requests";
            break;
        case(BAD_METHOD):
        this->status = "405 Method Not Allowed";
                break;
        case(HTTP_NOT_SUPPORTED):
        this->status = "505 HTTP Version Not Supported";
                break;
        default:
            throw omnetpp::cRuntimeError("Response code not allowed");
    }
}

void HTTPResponsePacket::setContentType(const char* contentType_){
    headerFields_["Content-Type: "] = std::string(contentType_);
}

void HTTPResponsePacket::setConnection(const char* connection_){
    headerFields_["Connection: "] = std::string(connection_);
}

void HTTPResponsePacket::setBody(const char * body_){
    body = ::omnetpp::opp_string(body_);
    headerFields_["Content-Length: "] = std::to_string(strlen(body_));

}

void HTTPResponsePacket::setBody(const std::string& body_){
    body = ::omnetpp::opp_string(body_);
    headerFields_["Content-Length: "] = std::to_string(body_.size());
}

void HTTPResponsePacket::setHeaderField(const std::string& key, const std::string& value){
    headerFields_[key] = value;
}

const char* HTTPResponsePacket::getPayload(){
    this->payload = httpVersion_ + status + "\r\n";
    if(!headerFields_.empty()){
        std::map<std::string, std::string>::const_iterator it = headerFields_.begin();
        std::map<std::string, std::string>::const_iterator end = headerFields_.end();
        for(; it != end; ++it){
            payload += it->first + it->second + "\r\n";
        }
    }
    this->payload += ::omnetpp::opp_string("\r\n") + body;
    return this->payload.c_str();
}


void HTTPResponsePacket::copy(const HTTPResponsePacket& other)
{
    this->httpVersion_ = other.httpVersion_;
    this->headerFields_ = other.headerFields_;
}


