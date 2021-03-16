#include "nodes/mec/MEPlatform/MeServices/packets/request/HttpRequestPacket.h"

#include "inet/common/INETDefs.h"



HTTPRequestPacket::HTTPRequestPacket(const char *name, short kind): HTTPRequestPacket_Base(name,kind)
{
     setContentType("application/json");
     setBody("");
     setPayload("");
}
HTTPRequestPacket::HTTPRequestPacket(const verb method, const char *name, short kind): HTTPRequestPacket_Base(name, kind)
{
    setMethod(method);
    setContentType("application/json");
    setBody("");
    setPayload("");

}
HTTPRequestPacket::HTTPRequestPacket(const char* method, const char *name, short kind): HTTPRequestPacket_Base(name, kind)
{
    HTTPRequestPacket_Base::setMethod(method);
    setContentType("application/json");
    setBody("\r\n");
    setPayload("");
}


void HTTPRequestPacket::setMethod(const verb method){
    switch(method) {
        case(GET):
            this->method = "GET";
            break;
        case(POST):
            this->method = "POST";
            break;
        case(PUT):
            this->method = "PUT";
            break;
        case(DELETE):
            this->method = "DELETE";
            break;
        default:
            throw omnetpp::cRuntimeError("Method code not allowed");
    }

}

void HTTPRequestPacket::setContentType(const char* contentType_){
    headerFields_["Content-Type: "] = std::string(contentType_);
}

void HTTPRequestPacket::setLength(const char * length_)
{
    headerFields_["Content-Length: "] = std::string(length_);
}
void HTTPRequestPacket::setLength(unsigned int length_)
{
    headerFields_["Content-Length: "] = std::string(std::to_string(length_));
}

void HTTPRequestPacket::setHost(const char *host_)
{
    headerFields_["Host: "] = std::string(host_);
}
void HTTPRequestPacket::setHost(const std::string& host_)
{
    headerFields_["Host: "] = host_;
}

void HTTPRequestPacket::setUri(const std::string& uri_){
    uri = uri_;
}

void HTTPRequestPacket::setBody(const char * body_){
    body = ::omnetpp::opp_string(body_);
    setLength(body.size());
}

void HTTPRequestPacket::setBody(const std::string& body_){
    body = ::omnetpp::opp_string(body_);
    setLength(body.size());
}

void HTTPRequestPacket::setHeaderField(const std::string& key , const std::string& value){
    headerFields_[key] = value;
}

const char* HTTPRequestPacket::getPayload(){
    this->payload = method + " " + uri + " " + httpVersion + "\r\n";
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


