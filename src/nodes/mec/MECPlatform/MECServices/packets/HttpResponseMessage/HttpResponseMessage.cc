//

#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include <inet/common/INETDefs.h>

namespace simu5g {

using namespace omnetpp;

HttpResponseMessage::HttpResponseMessage(const char *name, short kind)
{
    setContentType("application/json");
    setConnection("keep-alive");
    setBody("");
}

HttpResponseMessage::HttpResponseMessage(const HttpResponseStatus res, const char *name, short kind)
{
    setStatus(res);
    setContentType("application/json");
    setConnection("keep-alive");
    setBody("");
}

void HttpResponseMessage::addBodyChunk(const std::string& bodyChunk)
{
    handleChange();
    if (this->body.empty())
        this->body = bodyChunk;
    else
        this->body += bodyChunk;
}

void HttpResponseMessage::setStatus(const char *status) {
    handleChange();
    this->status = status;
}

void HttpResponseMessage::setStatus(HttpResponseStatus res) {
    handleChange();
    switch (res) {
        case (OK):
            this->code = 200;
            this->status = "OK";
            break;
        case (CREATED):
            this->code = 201;
            this->status = "Created";
            break;
        case (NO_CONTENT):
            this->code = 204;
            this->status = "No Content";
            break;
        case (BAD_REQ):
            this->code = 400;
            this->status = "Bad Request";
            break;
        case (UNAUTH):
            this->code = 401;
            this->status = "Unauthorized";
            break;
        case (FORBIDDEN):
            this->code = 403;
            this->status = "Forbidden";
            break;
        case (NOT_FOUND):
            this->code = 404;
            this->status = "Not Found";
            break;
        case (NOT_ACC):
            this->code = 406;
            this->status = "Not Acceptable";
            break;
        case (TOO_REQS):
            this->code = 429;
            this->status = "Too Many Requests";
            break;
        case (BAD_METHOD):
            this->code = 405;
            this->status = "Method Not Allowed";
            break;
        case (HTTP_NOT_SUPPORTED):
            this->code = 505;
            this->status = "HTTP Version Not Supported";
            break;
        default:
            throw cRuntimeError("Response code not allowed");
    }
}

void HttpResponseMessage::setHeaderField(const std::string& key, const std::string& value) {
    handleChange();
    headerFields_[key] = value;
}

std::string HttpResponseMessage::getHeaderField(const std::string& key) const
{
    auto it = headerFields_.find(key);
    if (it == headerFields_.end())
        return std::string();
    return it->second;
}

std::string HttpResponseMessage::getPayload() const {
    std::string crlf = "\r\n";
    std::string payload = this->httpProtocol.str() + " " + std::to_string(this->code) + " " + status.str() + crlf;
    if (contentLength != 0)
        payload += "Content-Length: " + std::to_string(this->contentLength) + crlf;
    payload += "Content-Type: " + this->contentType.str() + crlf;
    payload += "Connection: " + this->connection.str() + crlf;

    if (!headerFields_.empty()) {
        for (const auto& headerField : headerFields_) {
            payload += headerField.first + ": " + headerField.second + crlf; // Added colon separator
        }
    }
    payload += crlf + body.str();
    return payload;
}

void HttpResponseMessage::copy(const HttpResponseMessage& other)
{
    this->headerFields_ = other.headerFields_;
}

} //namespace

