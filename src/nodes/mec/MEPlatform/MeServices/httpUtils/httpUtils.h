// @author Alessandro Noferi
//

#ifndef __HTTPUTILS_H
#define __HTTPUTILS_H


#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/MEPlatform/MeServices/packets/response/HttpResponsePacket.h"
#include "nodes/mec/MEPlatform/MeServices/packets/request/HttpRequestPacket.h"

#include "nodes/mec/MEPlatform/MeServices/packets/HttpMessages_m.h"
#include <string>

namespace Http {

/*
 * TODO
 *
 * consider to use only one method with the code as argument
 * e.g sendResponse(code, socket, body, *args)
 *
 *
 */
enum HttpMsgState {
    COMPLETE_DATA,
    COMPLETE_NO_DATA,
    INCOMPLETE_DATA,
    INCOMPLETE_NO_DATA
};

//struct HTTPHeader
//{
//    DataType type;
//    std::map<std::string, std::string> fields;
//};

HttpBaseMessage* parseHeader(const std::string& header);

HttpMsgState parseTcpData(std::string* data, HttpBaseMessage* httpMessage);

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

void send505Response(inet::TcpSocket *socket);
void send503Response(inet::TcpSocket *socket, const char *reason);


void sendPostRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri);
void sendPutRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri);
void sendGetRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri);
void sendDeleteRequest(inet::TcpSocket *socket, const char* host, const char* uri);


}




#endif
