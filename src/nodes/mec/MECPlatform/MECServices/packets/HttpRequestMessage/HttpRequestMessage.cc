#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"

#include "inet/common/INETDefs.h"


HttpRequestMessage::HttpRequestMessage(const char *name, short kind)
{
    isBackgroundRequest_ = false;
    isLastBackgroundRequest_ = false;
    setContentType("application/json");
    setBody("");
}
HttpRequestMessage::HttpRequestMessage(const std::string method, const char *name, short kind)
{
    isBackgroundRequest_ = false;
    isLastBackgroundRequest_ = false;
    setMethod(method.c_str());
    setContentType("application/json");
    setBody("");

}
HttpRequestMessage::HttpRequestMessage(const char* method, const char *name, short kind)
{
    isBackgroundRequest_ = false;
    isLastBackgroundRequest_ = false;
    setMethod(method);
    setContentType("application/json");
    setBody("");
}


void HttpRequestMessage::setHeaderField(const std::string& key , const std::string& value){
    handleChange();
    headerFields_[key] = value;
}

std::string HttpRequestMessage::getPayload() const{
    std::string crlf = "\r\n";
    std::string payload;
    if(this->parameters.size() == 0)
        payload = this->method.str() + " " + this->uri.str() + " " + this->httpProtocol.str() + crlf;
    else
        payload = this->method.str() + " " + this->uri.str() + "?"+ this->parameters.str() + " " + this->httpProtocol.str() + crlf;
    if(host != "")
        payload += "Host: " + this->host.str() + crlf;

    if(contentLength != 0)
        payload += "Content-Length: " + std::to_string(this->contentLength) + crlf;
    payload += "Content-Type: " + this->contentType.str() + crlf;
    payload += "Connection: " + this->connection.str() + crlf;

    if(!headerFields_.empty()){
        std::map<std::string, std::string>::const_iterator it = headerFields_.begin();
        std::map<std::string, std::string>::const_iterator end = headerFields_.end();
        for(; it != end; ++it){
            payload += it->first + it->second + crlf;
        }
    }
    payload += crlf + body.str();
    return payload;
}

void HttpRequestMessage::addBodyChunk(const std::string& bodyChunk)
{
    handleChange();
    if(this->body.empty())
        this->body = bodyChunk;
    else
        this->body += bodyChunk;
}

void HttpRequestMessage::copy(const HttpRequestMessage& other)
{
    this->headerFields_ = other.headerFields_;
}


