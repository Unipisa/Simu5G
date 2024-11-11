//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/mec/utils/httpUtils/httpUtils.h"

#include <inet/common/INETDefs.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/common/TagBase_m.h>
#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/Packet.h>
#include <inet/common/packet/chunk/BytesChunk.h>

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

namespace Http {

/*
 * Method for sending raw bytes.
 */
void sendPacket(const char *payload, inet::TcpSocket *socket) {
    Ptr<Chunk> chunkPayload;
    const auto& bytesChunk = inet::makeShared<BytesChunk>();
    std::vector<uint8_t> vec(payload, payload + strlen(payload));
    bytesChunk->setBytes(vec);
    chunkPayload = bytesChunk;
    chunkPayload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    Packet *packet = new Packet("Packet");
    packet->insertAtBack(chunkPayload);
    socket->send(packet);
    EV << "Http Utils - sendPacket" << endl;
}

bool checkHttpVersion(const std::string& httpVersion) {
    // HTTP/1.1 or HTTP/2
    return httpVersion == "HTTP/1.1" || httpVersion == "HTTP/2";
}

bool checkHttpRequestMethod(const std::string& method) {
    return method == "GET" || method == "POST" || method == "PUT" || method == "DELETE";
}

HttpBaseMessage *parseHeader(const std::string& data)
{
    std::vector<std::string> lines = simu5g::utils::splitString(data, "\r\n");
    std::vector<std::string>::iterator it = lines.begin();
    std::vector<std::string> line;
    EV << "httpUtils::parseHeader - Header: " << data << endl;

    // Request-Line: Method SP Request-URI SP HTTP-Version CRLF
    // Status-Line: HTTP-Version SP Status-Code SP Reason-Phrase CRLF

    line = simu5g::utils::splitString(*it, " ");

    /* it may be a request or a response, so the first line is different
     *
     * response: HTTP/1.1 code reason
     * request:  verb uri HTTP/1.1
     *
     */

    // if the first word is not HTTP/1.1 nor HTTP/2 (NOTE: assuming the HTTP is correct ---> it is a request)
    if (!checkHttpVersion(line[0])) {
        EV << "httpUtils::parseHeader - It is a request" << endl;
        // It is a request
        HttpRequestMessage *httpRequest = new HttpRequestMessage();
        // request line is: VERB uri HTTPversion
        if (line.size() == 3) {
            httpRequest->setType(REQUEST);
            if (!checkHttpRequestMethod(line[0])) {
                httpRequest->setState(BAD_REQ_METHOD);
                return httpRequest;
            }
            httpRequest->setMethod(line[0].c_str());

            /*
             * get URI and parameters
             */
            std::vector<std::string> uriParams = cStringTokenizer(line[1].c_str(), "?").asVector();
            if (uriParams.size() == 2) {
                // debug
                EV << "httpUtils::parseHeader - There are parameters" << endl;
                httpRequest->setUri(uriParams[0].c_str());
                httpRequest->setParameters(uriParams[1].c_str());
            }
            else if (uriParams.size() == 1) {
                // debug
                EV << "httpUtils::parseHeader - There are no parameters" << endl;
                httpRequest->setUri(uriParams[0].c_str());
            }
            else {
                //debug
                EV << "httpUtils::parseHeader - Parameters error" << endl;
                httpRequest->setState(BAD_REQ_URI);
                return httpRequest;
            }

            if (!checkHttpVersion(line[2])) {
                httpRequest->setState(BAD_HTTP);
                return httpRequest;
            }
            httpRequest->setHttpProtocol(line[2].c_str());
        }
        else {
            httpRequest->setState(BAD_REQ_LINE);
            return httpRequest;
        }

        // read for headers
        for (++it; it != lines.end(); ++it) {
            line = simu5g::utils::splitString(*it, ": ");
            if (line.size() == 2) {
                if (line[0] == "Content-Length") {
                    EV << "httpUtils::parseHeader - Content-Length: " << line[1] << endl;
                    httpRequest->setContentLength(std::stoi(line[1]));
                    httpRequest->setRemainingDataToRecv(std::stoi(line[1]));
                }
                else if (line[0] == "Content-Type") {
                    EV << "httpUtils::parseHeader - Content-Type: " << line[1] << endl;
                    httpRequest->setContentType(line[1].c_str());
                }
                else if (line[0] == "Host") {
                    EV << "httpUtils::parseHeader - Host: " << line[1] << endl;
                    httpRequest->setHost(line[1].c_str());
                }
                else if (line[0] == "Connection") {
                    EV << "httpUtils::parseHeader - Connection: " << line[1] << endl;
                    httpRequest->setConnection(line[1].c_str());
                }
                else {
                    EV << "httpUtils::parseHeader - Header: " << line[1] << ": " << line[0] << endl;
                    httpRequest->setHeaderField(line[0], line[1]);
                }
            }
            else {
                httpRequest->setState(BAD_HEADER);
                return httpRequest;
            }
        }
        httpRequest->setState(CORRECT);
        return httpRequest;
    }
    // it's a response
    else {
        EV << "httpUtils::parseHeader - It is a response" << endl;
        HttpResponseMessage *httpResponse = new HttpResponseMessage();
        httpResponse->setType(RESPONSE);

        if (line.size() < 3 || line.size() > 5) {
            EV << "httpUtils::parseHeader - BAD_RES_LINE" << endl;
            httpResponse->setState(BAD_RES_LINE);
            return httpResponse;
        }

        // response line is: HTTPversion code reason
        if (!checkHttpVersion(line[0])) {
            EV << "httpUtils::parseHeader - BAD_HTTP" << endl;
            httpResponse->setState(BAD_HTTP);
            return httpResponse;
        }

        httpResponse->setHttpProtocol(line[0].c_str());
        httpResponse->setCode(std::stoi(line[1]));

        std::string reason;
        int size = line.size();
        for (int i = 2; i < size; ++i) {
            reason += line[i];
            if (i != size - 1)
                reason += " ";
        }

        httpResponse->setStatus(reason.c_str());
        EV << "httpUtils::parseHeader - code " << httpResponse->getCode() << endl;

        // read for headers
        for (++it; it != lines.end(); ++it) {
            line = simu5g::utils::splitString(*it, ": ");
            if (line.size() == 2) {
                if (line[0] == "Content-Length") {
                    EV << "httpUtils::parseHeader - Content-Length: " << line[1] << endl;
                    httpResponse->setContentLength(std::stoi(line[1]));
                    httpResponse->setRemainingDataToRecv(std::stoi(line[1]));
                }
                else if (line[0] == "Content-Type") {
                    EV << "httpUtils::parseHeader - Content-Type: " << line[1] << endl;
                    httpResponse->setContentType(line[1].c_str());
                }
                else if (line[0] == "Connection") {
                    EV << "httpUtils::parseHeader - Connection: " << line[1] << endl;
                    httpResponse->setConnection(line[1].c_str());
                }
                else {
                    EV << "httpUtils::parseHeader - Header: " << line[1] << ": " << line[0] << endl;
                    httpResponse->setHeaderField(line[0], line[1]);
                }
            }
            else {
                httpResponse->setState(BAD_HEADER);
                return httpResponse;
            }
        }
        httpResponse->setState(CORRECT);
        return httpResponse;
    }
}

HttpMsgState parseTcpData(std::string& data, HttpBaseMessage *httpMessage)
{
    if (httpMessage == nullptr)
        throw cRuntimeError("httpUtils parseTcpData - httpMessage must not be null");

    httpMessage->setIsReceivingMsg(true);
    addBodyChunk(data, httpMessage);

    if (data.length() == 0 && httpMessage->getRemainingDataToRecv() == 0)
        return COMPLETE_NO_DATA;
    else if (data.length() > 0 && httpMessage->getRemainingDataToRecv() == 0)
        return COMPLETE_DATA; // there is a new message
    else if (data.length() == 0 && httpMessage->getRemainingDataToRecv() > 0)
        return INCOMPLETE_NO_DATA; //
    else if (data.length() >= 0 && httpMessage->getRemainingDataToRecv() > 0)
        return INCOMPLETE_DATA; // this should not happen
    else {
        throw cRuntimeError("httpUtils parseTcpData - something went wrong: data length: %lu and remaining data to receive: %d", data.length(), httpMessage->getRemainingDataToRecv());
    }
}

bool parseReceivedMsg(const std::string& inPacket, std::string& storedData, HttpBaseMessage *& currentHttpMessage)
{
    EV_INFO << "httpUtils::parseReceivedMsg- start..." << endl;
    std::string delimiter = "\r\n\r\n";
    size_t pos = 0;
    std::string header;
    std::string packet(inPacket);
    if (currentHttpMessage != nullptr && currentHttpMessage->isReceivingMsg()) {
        EV << "MecAppBase::parseReceivedMsg - Continue receiving data for the current HttpMessage" << endl;
        Http::HttpMsgState res = Http::parseTcpData(packet, currentHttpMessage);
        switch (res) {
            case (Http::COMPLETE_NO_DATA):
                EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
                return true;
            case (Http::COMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - This function does not support multiple HTTP messages in one segment");
                break;
            case (Http::INCOMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - current Http Message is incomplete, but there is still data to read");
            case (Http::INCOMPLETE_NO_DATA):
                return false;
        }
    }

    /*
     * If I get here OR:
     *  - I am not receiving an http message
     *  - I was receiving an http message but I still have data (i.e a new HttpMessage) to manage.
     *    Start reading the header
     */

    std::string temp;
    if (storedData.length() > 0) {
        EV << "MecAppBase::parseReceivedMsg - buffered data" << endl;
        temp = packet;
        packet = storedData + temp;
    }

    while ((pos = packet.find(delimiter)) != std::string::npos) {
        EV << "MecAppBase::parseReceivedMsg - new HTTP message" << endl;
        header = packet.substr(0, pos);
        packet.erase(0, pos + delimiter.length()); //remove header
        currentHttpMessage = Http::parseHeader(header);

        Http::HttpMsgState res = Http::parseTcpData(packet, currentHttpMessage);

        switch (res) {
            case (Http::COMPLETE_NO_DATA):
                EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
                return true;
            case (Http::COMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - This function does not support multiple HTTP messages in one segment");
            case (Http::INCOMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - current Http Message is incomplete, but there is still data to read");
            case (Http::INCOMPLETE_NO_DATA):
                return false;
        }
    }

    /*
     * If I did not find the delimiter ("\r\n\r\n")
     * it could mean that the HTTP message is fragmented at the header, so the data
     * should be saved and aggregated with the subsequent fragmented
     */
    if (packet.length() != 0) {
        EV << "MecAppBase::parseReceivedMsg - stored data: " << packet << endl;
        storedData = packet;
        return false;
    }
    return false;
}

bool parseReceivedMsg(int socketId, const std::string& inPacket, cQueue& messageQueue, std::string& storedData, HttpBaseMessage *& currentHttpMessage)
{
    EV_INFO << "httpUtils::parseReceivedMsg" << endl;
    //std::cout << "MecAppBase::parseReceivedMsg" << std::endl;
    //std::cout << "MecAppBase::parseReceivedMsg MSG :" << packet << std::endl;

    std::string delimiter = "\r\n\r\n";
    size_t pos = 0;
    std::string header;
    std::string packet(inPacket);
    bool completeMsg = false;

    // continue receiving the message
    if (currentHttpMessage != nullptr && currentHttpMessage->isReceivingMsg()) {
        // EV << "MecAppBase::parseReceivedMsg - Continue receiving data for the current HttpMessage" << endl;
        Http::HttpMsgState res = Http::parseTcpData(packet, currentHttpMessage);
        switch (res) {
            case (Http::COMPLETE_NO_DATA):
                // EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
                currentHttpMessage->setSockId(socketId);
                messageQueue.insert(currentHttpMessage);
                completeMsg = true;
                currentHttpMessage = nullptr;
                return completeMsg;
            case (Http::COMPLETE_DATA):
                // EV << "MecAppBase::parseReceivedMsg - passing HttpMessage to application: " << res << endl;
                currentHttpMessage->setSockId(socketId);
                messageQueue.insert(currentHttpMessage);
                completeMsg = true;
                currentHttpMessage = nullptr;
                break;
            case (Http::INCOMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - current Http Message is incomplete, but there is still data to read");
            case (Http::INCOMPLETE_NO_DATA):
                return completeMsg;
        }
    }

    /*
     * If I get here OR:
     *  - I am not receiving an http message
     *  - I was receiving an http message but I still have data (i.e a new HttpMessage) to manage.
     *    Start reading the header
     */

    std::string temp;
    if (storedData.length() > 0) {
        // EV << "MecAppBase::parseReceivedMsg - buffered data" << endl;
        //std::cout << "MecAppBase::parseReceivedMsg - buffered data" << std::endl;
        //std::cout << "MecAppBase::parseReceivedMsg buffered data" << storedData  << std::endl;

        temp = packet;
        packet = storedData + temp;
        storedData.clear();
    }

    while ((pos = packet.find(delimiter)) != std::string::npos) {
        header = packet.substr(0, pos);
        packet.erase(0, pos + delimiter.length()); //remove header
        currentHttpMessage = Http::parseHeader(header);

        Http::HttpMsgState res = Http::parseTcpData(packet, currentHttpMessage);
        switch (res) {
            case (Http::COMPLETE_NO_DATA):
                currentHttpMessage->setSockId(socketId);
                messageQueue.insert(currentHttpMessage);
                completeMsg = true;
                currentHttpMessage = nullptr;
                return completeMsg;
            case (Http::COMPLETE_DATA):
                currentHttpMessage->setSockId(socketId);
                messageQueue.insert(currentHttpMessage);
                completeMsg = true;
                currentHttpMessage = nullptr;
                break;
            case (Http::INCOMPLETE_DATA):
                throw cRuntimeError("httpUtils parseReceivedMsg - current Http Message is incomplete, but there is still data to read");
            case (Http::INCOMPLETE_NO_DATA):
                return completeMsg;
        }
    }

    /*
     * If I did not find the delimiter ("\r\n\r\n")
     * it could mean that the HTTP message is fragmented at the header, so the data
     * should be saved and aggregated with the subsequent fragmented
     */
    if (packet.length() != 0) {
        storedData = packet;
        return completeMsg;
    }
    return true;
}

void addBodyChunk(std::string& data, HttpBaseMessage *httpMessage)
{
    int len = data.length();
    int remainingLength = httpMessage->getRemainingDataToRecv();
    if (remainingLength == 0 && len == 0) {
        EV << "httpUtils::addBodyChunk - no body" << endl;
        return;
    }
    EV << "httpUtils - addBodyChunk: data length: " << len << "B. Remaining bytes: " << remainingLength << endl;

    if (httpMessage->getType() == RESPONSE) {
        EV << "httpUtils::addBodyChunk - RESPONSE " << endl;
        HttpResponseMessage *resp = dynamic_cast<HttpResponseMessage *>(httpMessage);
        resp->addBodyChunk(data.substr(0, remainingLength));
    }
    else if (httpMessage->getType() == REQUEST) {
        EV << "httpUtils::addBodyChunk - REQUEST " << endl;
        HttpRequestMessage *resp = dynamic_cast<HttpRequestMessage *>(httpMessage);
        resp->addBodyChunk(data.substr(0, remainingLength));
    }

    data.erase(0, remainingLength);
    httpMessage->setRemainingDataToRecv(remainingLength - (len - data.length()));
    EV << "httpUtils - addBodyChunk: Remaining bytes: " << httpMessage->getRemainingDataToRecv() << endl;
}

void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, const char *body)
{
    EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;

    inet::Packet *packet = new inet::Packet("HttpResponsePacket");
    auto resPkt = inet::makeShared<HttpResponseMessage>();

    resPkt->setCode(code);
    resPkt->setStatus(reason);
    if (body != nullptr) {
        resPkt->setBody(body);
        resPkt->setContentLength(strlen(body));
    }
    resPkt->setChunkLength(B(resPkt->getPayload().size()));
    packet->insertAtBack(resPkt);
    socket->send(packet);
}

void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, std::pair<std::string, std::string>& header, const char *body)
{
    EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;

    inet::Packet *packet = new inet::Packet("HttpResponsePacket");
    auto resPkt = inet::makeShared<HttpResponseMessage>();

    resPkt->setCode(code);
    resPkt->setStatus(reason);
    if (body != nullptr) {
        resPkt->setBody(body);
        resPkt->setContentLength(strlen(body));
    }
    resPkt->setHeaderField(header.first, header.second);

    resPkt->setChunkLength(B(resPkt->getPayload().size()));
    resPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(resPkt);
    socket->send(packet);
}

void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, std::map<std::string, std::string>& headers, const char *body)
{
    EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
    inet::Packet *packet = new inet::Packet("HttpResponsePacket");
    auto resPkt = inet::makeShared<HttpResponseMessage>();

    resPkt->setCode(code);
    resPkt->setStatus(reason);
    if (body != nullptr) {
        resPkt->setBody(body);
        resPkt->setContentLength(strlen(body));
    }
    for (const auto& header : headers) {
        resPkt->setHeaderField(header.first, header.second);
    }

    resPkt->setChunkLength(B(resPkt->getPayload().size()));
    resPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(resPkt);
    socket->send(packet);
}

void sendHttpRequest(inet::TcpSocket *socket, const char *method, const char *host, const char *uri, const char *parameters, const char *body)
{
    EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
    inet::Packet *packet = new inet::Packet("HttpRequestPacket");
    auto reqPkt = inet::makeShared<HttpRequestMessage>();

    reqPkt->setMethod(method);
    reqPkt->setUri(uri);
    reqPkt->setHost(host);
    if (body != nullptr) {
        reqPkt->setBody(body);
        reqPkt->setContentLength(strlen(body));
    }
    if (parameters != nullptr) {
        reqPkt->setParameters(parameters);
    }
    reqPkt->setChunkLength(B(reqPkt->getPayload().size())); // TODO get size more efficiently
    reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(reqPkt);
    socket->send(packet);
}

void sendHttpRequest(inet::TcpSocket *socket, const char *method, const char *host, std::pair<std::string, std::string>& header, const char *uri, const char *parameters, const char *body)
{
    EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
    inet::Packet *packet = new inet::Packet("HttpRequestPacket");
    auto reqPkt = inet::makeShared<HttpRequestMessage>();

    reqPkt->setMethod(method);
    reqPkt->setUri(uri);
    reqPkt->setHost(host);
    if (body != nullptr) {
        reqPkt->setBody(body);
        reqPkt->setContentLength(strlen(body));
    }
    if (parameters != nullptr) {
        reqPkt->setParameters(parameters);
    }
    reqPkt->setHeaderField(header.first, header.second);
    reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
    reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(reqPkt);
    socket->send(packet);
}

void sendHttpRequest(inet::TcpSocket *socket, const char *method, const char *host, std::map<std::string, std::string>& headers, const char *uri, const char *parameters, const char *body)
{
    EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
    inet::Packet *packet = new inet::Packet("HttpRequestPacket");
    auto reqPkt = inet::makeShared<HttpRequestMessage>();

    reqPkt->setMethod(method);
    reqPkt->setUri(uri);
    reqPkt->setHost(host);
    if (body != nullptr) {
        reqPkt->setBody(body);
        reqPkt->setContentLength(strlen(body));
    }
    if (parameters != nullptr) {
        reqPkt->setParameters(parameters);
    }
    for (const auto& header : headers) {
        reqPkt->setHeaderField(header.first, header.second);
    }
    reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
    reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(reqPkt);
    socket->send(packet);
}

void send200Response(inet::TcpSocket *socket, const char *body) {
    sendHttpResponse(socket, 200, "OK", body);
}

void send201Response(inet::TcpSocket *socket, const char *body) {
    sendHttpResponse(socket, 201, "Created", body);
}

void send201Response(inet::TcpSocket *socket, const char *body, std::pair<std::string, std::string>& header) {
    sendHttpResponse(socket, 201, "Created", header, body);
}

void send201Response(inet::TcpSocket *socket, const char *body, std::map<std::string, std::string>& headers) {
    sendHttpResponse(socket, 201, "Created", headers, body);
}

void send204Response(inet::TcpSocket *socket) {
    sendHttpResponse(socket, 204, "No Content");
}

void send405Response(inet::TcpSocket *socket, const char *methods) {
    std::pair<std::string, std::string> header;
    header.first = "Allow: ";
    if (methods == nullptr)
        header.second = "GET, POST, DELETE, PUT";
    else {
        header.second = std::string(methods);
    }
    sendHttpResponse(socket, 405, "Method Not Allowed", header);
}

void send400Response(inet::TcpSocket *socket)
{
    sendHttpResponse(socket, 400, "Bad Request", "{ \"send400Response\" : \"TODO implement ProblemDetails\"}");
}

void send400Response(inet::TcpSocket *socket, const char *reason)
{
    ProblemDetailBase probDet;
    probDet.detail = reason;
    probDet.status = "400";
    probDet.title = "400 Response";
    probDet.type = "Client Error";

    sendHttpResponse(socket, 400, "Bad Request", probDet.toJson().dump().c_str());
}

void send404Response(inet::TcpSocket *socket, const char *reason)
{
    ProblemDetailBase probDet;
    probDet.detail = reason;
    probDet.status = "404";
    probDet.title = "404 Response";
    probDet.type = "Client Error";

    sendHttpResponse(socket, 400, "Bad Request", probDet.toJson().dump().c_str());
    sendHttpResponse(socket, 400, "Bad Request", reason);
}

void send404Response(inet::TcpSocket *socket)
{
    sendHttpResponse(socket, 404, "Not Found", "{ \"send404Response\" : \"TODO implement ProblemDetails\"}");
}

void send505Response(inet::TcpSocket *socket)
{
    sendHttpResponse(socket, 505, "HTTP Version Not Supported");
}

void send503Response(inet::TcpSocket *socket, const char *reason)
{
    sendHttpResponse(socket, 503, "HTTP Version Not Supported", reason);
}

void send500Response(inet::TcpSocket *socket, const char *reason)
{
    sendHttpResponse(socket, 500, "Internal Error", reason);
}

void sendPostRequest(inet::TcpSocket *socket, const char *body, const char *host, const char *uri, const char *parameters)
{
    sendHttpRequest(socket, "POST", host, uri, parameters, body);
}

void sendPutRequest(inet::TcpSocket *socket, const char *body, const char *host, const char *uri, const char *parameters)
{
    sendHttpRequest(socket, "PUT", host, uri, parameters, body);
}

void sendGetRequest(inet::TcpSocket *socket, const char *host, const char *uri, const char *parameters, const char *body)
{
    sendHttpRequest(socket, "GET", host, uri, parameters, body);
}

void sendDeleteRequest(inet::TcpSocket *socket, const char *host, const char *uri)
{
    sendHttpRequest(socket, "DELETE", host, uri);
}

} // namespace Http

} //namespace

