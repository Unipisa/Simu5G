//

#include <inet/common/INETDefs.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"

namespace simu5g {

HttpRequestMessage::HttpRequestMessage(const char *name, short kind) : isBackgroundRequest_(false), isLastBackgroundRequest_(false)
{
    setContentType("application/json");
    setBody("");
}

HttpRequestMessage::HttpRequestMessage(const std::string method, const char *name, short kind) : isBackgroundRequest_(false), isLastBackgroundRequest_(false)
{
    setMethod(method.c_str());
    setContentType("application/json");
    setBody("");
}

HttpRequestMessage::HttpRequestMessage(const char *method, const char *name, short kind) : isBackgroundRequest_(false), isLastBackgroundRequest_(false)
{
    setMethod(method);
    setContentType("application/json");
    setBody("");
}

void HttpRequestMessage::setHeaderField(const std::string& key, const std::string& value) {
    handleChange();
    headerFields_[key] = value;
}

std::string HttpRequestMessage::getPayload() const {
    std::string crlf = "\r\n";
    std::string payload;
    if (this->parameters.size() == 0)
        payload = this->method.str() + " " + this->uri.str() + " " + this->httpProtocol.str() + crlf;
    else
        payload = this->method.str() + " " + this->uri.str() + "?" + this->parameters.str() + " " + this->httpProtocol.str() + crlf;
    if (host != "")
        payload += "Host: " + this->host.str() + crlf;

    if (contentLength != 0)
        payload += "Content-Length: " + std::to_string(this->contentLength) + crlf;
    payload += "Content-Type: " + this->contentType.str() + crlf;
    payload += "Connection: " + this->connection.str() + crlf;

    if (!headerFields_.empty()) {
        for (const auto& header : headerFields_) {
            payload += header.first + header.second + crlf;
        }
    }
    payload += crlf + body.str();
    return payload;
}

void HttpRequestMessage::addBodyChunk(const std::string& bodyChunk)
{
    handleChange();
    if (this->body.empty())
        this->body = bodyChunk;
    else
        this->body += bodyChunk;
}

void HttpRequestMessage::copy(const HttpRequestMessage& other)
{
    this->headerFields_ = other.headerFields_;
}

} //namespace

