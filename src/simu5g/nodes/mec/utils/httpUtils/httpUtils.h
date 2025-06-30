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

#ifndef __HTTPUTILS_H
#define __HTTPUTILS_H

#include <string>

#include <inet/transportlayer/contract/tcp/TcpSocket.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

using namespace omnetpp;

/*
 * httpUtils collects all the functions needed to manage HTTP messages:
 * - body parsing
 * - header parsing
 * - send HTTP messages (both requests and responses)
 *
 */

namespace Http {

enum HttpMsgState {
    COMPLETE_DATA,
    COMPLETE_NO_DATA,
    INCOMPLETE_DATA,
    INCOMPLETE_NO_DATA
};

/*
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
 *
 * ProblemDetailBase provides a basic usage of the ProblemDetail structure
 */

struct ProblemDetailBase
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

};

/**************************** HTTP MESSAGE PARSING ******************************
* The management of HTTP messages as raw bytes is due to the fact that the      *
* simulator can also run in emulation mode and the ExtLowerInterface modules    *
* treat incoming packets as raw bytes. For now, a better management using the  *
* INET framework has not been found, so the following set of functions to parse *
* strings into HttpBaseMessage is used.                                         *
********************************************************************************/

/*
 * This function parses a string containing the HTTP header and returns a
 * HttpMessage according to whether it is a request or a response.
 *
 * @param header - string representing the header of the HTTP message
 * @return HttpBaseMessage pointer to a new HTTP message with a field
 * named HttpRequestState that labels the correctness of the message
 */
HttpBaseMessage *parseHeader(const std::string& header);

HttpMsgState parseTcpData(std::string& data, HttpBaseMessage *httpMessage);

/*
 * This function adds body content (using addBodyChunk)  to a fragmented HTTP Message
 * @param data - string representing the body of a fragmented HTTP Message
 * @param httpMessage - HttpBaseMessage of the current message
 * @return HttpMsgState - state of the HTTP message
 */
void addBodyChunk(std::string& data, HttpBaseMessage *httpMessage);

/*
 * This function parses an incoming message by calling the above functions,
 * i.e. parseHeader and parseTcpData.
 *
 * @param inPacket raw bytes as string of the incoming message
 * @param storedData pointer to a variable where to store undefined data (e.g. segmented header)
 * @param currentHttpMessage variable for storing the current HTTP message
 * @return bool - if true the currentHttpMessage is completed and ready to be
 * processed by the application
 */
bool parseReceivedMsg(const std::string& inPacket, std::string& storedData, HttpBaseMessage *& currentHttpMessage);

/*
 *  WARNING: This function is not used in our scenarios, so it is not fully tested
 *
 * This function parses an incoming message by calling the above functions,
 * i.e. parseHeader and parseTcpData (as the parseReceivedMsg above), with the
 * addition of the management of TCP segments containing more than one HTTP message.
 * Every completed HTTP message is queued in the messageQueue.
 *
 * @param socketId needed to know to whom to send back the response
 * @param inPacket raw bytes as string of the incoming message
 * @param storedData reference to a variable where to store undefined data (e.g. segmented header)
 * @param currentHttpMessage variable for storing the current HTTP message
 * @param messageQueue queue where to insert completed Http Messages
 */
bool parseReceivedMsg(int socketId, const std::string& inPacket, cQueue& messageQueue, std::string& storedData, HttpBaseMessage *& currentHttpMessage);

/*************************************************************************************/

void sendPacket(const char *pck, inet::TcpSocket *socket);

/*
 * This function checks the version of the HTTP protocol used.
 * Supported versions: 1.1 and 2
 */
bool checkHttpVersion(const std::string& httpVersion);
/*
 * This function checks the method used in the HTTP request.
 * Supported methods: GET, POST, PUT, DELETE
 */
bool checkHttpRequestMethod(const std::string& method);

/*
 * Self-explanatory functions to send HTTP requests and responses.
 * Other modules should use only:
 * send***Response (with *** the code) for responses
 * and
 * send***Request (with *** the method) for requests
 *
 * Such functions internally call sendHttpRequest/sendHttpResponse
 */

void send200Response(inet::TcpSocket *socket, const char *body);
void send201Response(inet::TcpSocket *socket, const char *body);
void send201Response(inet::TcpSocket *socket, const char *body, std::pair<std::string, std::string>& header);
void send201Response(inet::TcpSocket *socket, const char *body, std::map<std::string, std::string>& headers);
void send204Response(inet::TcpSocket *socket);

void send405Response(inet::TcpSocket *socket, const char *methods = nullptr);
void send400Response(inet::TcpSocket *socket, const char *reason);
void send400Response(inet::TcpSocket *socket);
void send404Response(inet::TcpSocket *socket, const char *reason);
void send404Response(inet::TcpSocket *socket);

void send500Response(inet::TcpSocket *socket, const char *reason);
void send505Response(inet::TcpSocket *socket);
void send503Response(inet::TcpSocket *socket, const char *reason);

void sendHttpRequest(inet::TcpSocket *socket, const char *method, const char *host, const char *uri, const char *parameters = nullptr, const char *body = nullptr);
void sendHttpRequest(inet::TcpSocket *socket, const char *method, std::pair<std::string, std::string>& header, const char *uri, const char *parameters = nullptr, const char *body = nullptr);
void sendHttpRequest(inet::TcpSocket *socket, const char *method, std::map<std::string, std::string>& headers, const char *uri, const char *parameters = nullptr, const char *body = nullptr);

void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, const char *body = nullptr);
void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, std::pair<std::string, std::string>& header, const char *body = nullptr);
void sendHttpResponse(inet::TcpSocket *socket, int code, const char *reason, std::map<std::string, std::string>& headers, const char *body = nullptr);

void sendPostRequest(inet::TcpSocket *socket, const char *body, const char *host, const char *uri, const char *parameters = nullptr);
void sendPutRequest(inet::TcpSocket *socket, const char *body, const char *host, const char *uri, const char *parameters = nullptr);
void sendGetRequest(inet::TcpSocket *socket, const char *host, const char *uri, const char *body = nullptr, const char *parameters = nullptr);
void sendDeleteRequest(inet::TcpSocket *socket, const char *host, const char *uri);

} // namespace Http

} //namespace

#endif

