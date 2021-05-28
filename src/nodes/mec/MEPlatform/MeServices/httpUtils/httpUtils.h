// @author Alessandro Noferi
//

#ifndef __HTTPUTILS_H
#define __HTTPUTILS_H


#include "inet/transportlayer/contract/tcp/TcpSocket.h"

#include "nodes/mec/MEPlatform/MeServices/packets/HttpMessages_m.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpMessages_m.h"

#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"


#include <string>

namespace Http {


enum HttpMsgState {
    COMPLETE_DATA,
    COMPLETE_NO_DATA,
    INCOMPLETE_DATA,
    INCOMPLETE_NO_DATA
};



/*
     * TODO
     *
     * ProblemDetail structure from RFC 7807
     * {
     * "type": "https://example.com/probs/out-of-credit",
     * "title": "You do not have enough credit.",
     * "detail": "Your current balance is 30, but that costs 50.",
     * "instance": "/account/12345/msgs/abc",
     * "balance": 30,
     * "accounts": ["/account/12345","/account/67890"],
     * "status": 403
     * }
     */

typedef struct
{
    std::string type;
    std::string title;
    std::string detail;
    std::string status;

    nlohmann::json toJson()
    {
        nlohmann::json problemDetails;
        problemDetails["type"] = type;
        problemDetails["title"] = title;
        problemDetails["detail"] = detail;
        problemDetails["status"] = status;
        return problemDetails;
    }

} ProblemDetailBase;


HttpBaseMessage* parseHeader(const std::string& header);

HttpMsgState parseTcpData(std::string* data, HttpBaseMessage* httpMessage);


bool parseReceivedMsg(std::string& packet, std::string* storedData, HttpBaseMessage** currentHttpMessage);


// This function is not used, since our test cases does not require the possibility of receiving TCP segments with two http messages inside...
HttpBaseMessage* parseReceivedMsg(inet::TcpSocket *socket, std::string& packet, omnetpp::cQueue& messageQueue, std::string* storedData, HttpBaseMessage* currentHttpMessage = nullptr );


void addBodyChunk(std::string* data, HttpBaseMessage* httpMessage);

void sendPacket(const char* pck, inet::TcpSocket *socket);

bool checkHttpVersion(std::string& httpVersion);

bool checkHttpRequestMethod(const std::string& method);

void send200Response(inet::TcpSocket *socket, const char *body);
void send201Response(inet::TcpSocket *socket, const char *body);
void send201Response(inet::TcpSocket *socket, const char* body, std::pair<std::string, std::string>& header);
void send201Response(inet::TcpSocket *socket, const char* body, std::map<std::string, std::string>& headers);
void send204Response(inet::TcpSocket *socket);
void send405Response(inet::TcpSocket *socket, const char *methods =  "" );
void send400Response(inet::TcpSocket *socket, const char *reason);
void send400Response(inet::TcpSocket *socket);

void send404Response(inet::TcpSocket *socket);


void send500Response(inet::TcpSocket *socket, const char *reason);
void send505Response(inet::TcpSocket *socket);
void send503Response(inet::TcpSocket *socket, const char *reason);


void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, const char* body = nullptr);
void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, std::pair<std::string, std::string>& header, const char* body = nullptr);
void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, std::map<std::string, std::string>& headers, const char* body = nullptr);



void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, const char* body = nullptr);
void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, std::pair<std::string, std::string>& header, const char* body = nullptr);
void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, std::map<std::string, std::string>& headers, const char* body = nullptr);



void sendPostRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri);
void sendPutRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri);
void sendGetRequest(inet::TcpSocket *socket, const char* host, const char* uri,const char* body = nullptr);
void sendDeleteRequest(inet::TcpSocket *socket, const char* host, const char* uri);


}




#endif
