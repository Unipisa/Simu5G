
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/INETDefs.h"
#include "common/utils/utils.h"

#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"



namespace Http {
    using namespace inet;
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
     *
     *
     * {
     *  "type": "https://example.net/validation-error",
     *  "title": "Your request parameters didn't validate.",
     *  "invalid-params": [ {
     *      "name": "age",
     *      "reason": "must be a positive integer"
     *   },
     *   {
     *   "name": "color",
     *   "reason": "must be 'green', 'red' or 'blue'"}
     *   ]
     *}
     *
     *  Content-Type: application/problem+json
     *
     *  RNI API
     *
     *  {
     *      "ProblemDetails": {
     *          "type": "string",
     *          "title": "string",
     *          "status": 0,
     *          "detail": "string",
     *          "instance": "string"
     *      }
     *  }
     *
     *
     */

    void sendPacket(const char* payload, inet::TcpSocket *socket){
        Ptr<Chunk> chunkPayload;
        const auto& bytesChunk = inet::makeShared<BytesChunk>();
        std::vector<uint8_t> vec(payload, payload + strlen(payload));
        bytesChunk->setBytes(vec);
        chunkPayload = bytesChunk;
        chunkPayload->addTag<CreationTimeTag>()->setCreationTime(simTime());
        Packet *packet = new Packet("HTTPPacket");
        packet->insertAtBack(chunkPayload);
        socket->send(packet);
        EV <<"Http Utils - sendPacket" << endl;
    }


    bool checkHttpVersion(std::string& httpVersion){
        // HTTP/1.1
        // HTTP/2
        // HTTP/3
        return (httpVersion.compare("HTTP/1.1") == 0 || httpVersion.compare("HTTP/2") == 0);
    }

    bool checkHttpRequestMethod(const std::string& method){
        return (method.compare("GET") == 0 || method.compare("POST") == 0 || method.compare("PUT") == 0 || method.compare("DELETE") == 0);
    }

    HttpBaseMessage* parseHeader(const std::string& data)
    {

        std::vector<std::string> lines = lte::utils::splitString(data, "\r\n");
        std::vector<std::string>::iterator it = lines.begin();
        std::vector<std::string> line;
        EV << "Header: " << data << endl;
        line = lte::utils::splitString(*it, " ");  // Request-Line e.g POST uri Http
        if(line.size() < 3 ){
            // neither a request nor response, return a null pointer
            return nullptr;
        }

        /* it may be a request or a response, so the first line is different
         *
         * response: HTTP/1.1 code reason
         * request:  method uri HTTP/1.1
         *
         */

        // if it is not HTTP/1.1 or HTTP/2 (assuming the HTTP is correct ---> it is a request)
        if(!checkHttpVersion(line[0]))
        {
            EV << "It is a request" << endl;
            // It is a request
            HttpRequestMessage* httpRequest = new HttpRequestMessage();
            // request line is: VERB uri HTTPversion
            if(line.size() == 3)
            {

               httpRequest->setType(REQUEST);
               if(!checkHttpRequestMethod(line[0]))
               {
                   httpRequest->setState(eBAD_METHOD);
                   return httpRequest;
               }
               httpRequest->setMethod(line[0].c_str());
               httpRequest->setUri(line[1].c_str());
               if(!checkHttpVersion(line[2]))
               {
                   httpRequest->setState(eBAD_HTTP);
                   return httpRequest;
               }
               httpRequest->setHttpProtocol(line[2].c_str());

            }
            else
            {
                httpRequest->setState(eBAD_REQ_LINE);
                return httpRequest;
            }

            //check headers
            // read the other headers
            for(++it; it != lines.end(); ++it) {
                line = lte::utils::splitString(*it, ": ");
                if(line.size() == 2)
                {
                    if(line[0].compare("Content-Length") == 0 )
                    {
                        EV << "CL: " << line[1] << endl;
                        httpRequest->setContentLength(std::stoi(line[1]));
                        httpRequest->setRemainingDataToRecv(std::stoi(line[1]));
                    }
                    else if (line[0].compare("Content-Type") == 0)
                        httpRequest->setContentType(line[1].c_str());
                    else if (line[0].compare("Host")== 0)
                        httpRequest->setHost(line[1].c_str());
                    else if (line[0].compare("Connection")== 0)
                        httpRequest->setConnection(line[1].c_str());

                    else
                        httpRequest->setHeaderField(line[0], line[1]);
                }else
                {
                    httpRequest->setState(eBAD_HEADER);
                    return httpRequest;
                }
            }
            httpRequest->setState(eCORRECT);
            return httpRequest;
        }

        // its a response
        else
        {
            EV << "It is a response" << endl;
            HttpResponseMessage* httpResponse = new HttpResponseMessage();

            // response line is: HTTPversion code reason
            if(line.size() == 3)
            {
                httpResponse->setType(RESPONSE);

                if(!checkHttpVersion(line[0]))
                  {
                    httpResponse->setState(eBAD_HTTP);
                      return httpResponse;
                  }
                httpResponse->setHttpProtocol(line[0].c_str());

                httpResponse->setCode(line[1].c_str());
                httpResponse->setStatus(line[2].c_str());
            }

            else if (line.size() == 4)
            {
                httpResponse->setType(RESPONSE);

               if(!checkHttpVersion(line[0]))
                 {
                   httpResponse->setState(eBAD_HTTP);
                     return httpResponse;
                 }
               httpResponse->setHttpProtocol(line[0].c_str());

               httpResponse->setCode(line[1].c_str());
               httpResponse->setStatus((line[2]+line[3]).c_str());

            }
            else
            {
                httpResponse->setState(eBAD_RES_LINE);
                return httpResponse;
            }


            /////

            for(++it; it != lines.end(); ++it) {
                line = lte::utils::splitString(*it, ": ");
                if(line.size() == 2)
                {
                    if(line[0].compare("Content-Length")== 0)
                    {
                        httpResponse->setContentLength(std::stoi(line[1]));
                        httpResponse->setRemainingDataToRecv(std::stoi(line[1]));
                    }
                    else if (line[0].compare("Content-Type")== 0)
                        httpResponse->setContentType(line[1].c_str());
                    else if (line[0].compare("Connection")== 0)
                        httpResponse->setConnection(line[1].c_str());
                    else
                        httpResponse->setHeaderField(line[0], line[1]);
                }else
                {
                    httpResponse->setState(eBAD_HEADER);
                    return httpResponse;
                }
            }
            httpResponse->setState(eCORRECT);
            return httpResponse;
            /////
        }
    }

    HttpMsgState parseTcpData(std::string* data, HttpBaseMessage* httpMessage)
    {
        if(httpMessage == nullptr)
            throw cRuntimeError("httpUtils parseTcpData - httpMessage must be not null");

        httpMessage->setIsReceivingMsg(true);
        addBodyChunk(data, httpMessage);

        if(data->length() == 0  && httpMessage->getRemainingDataToRecv() == 0)
            return COMPLETE_NO_DATA;
        else if(data->length() > 0 && httpMessage->getRemainingDataToRecv() == 0)
            return COMPLETE_DATA;
        else if(data->length() == 0  && httpMessage->getRemainingDataToRecv() > 0)
            return INCOMPLETE_NO_DATA;
        else if(data->length() >= 0  && httpMessage->getRemainingDataToRecv() > 0)
            return INCOMPLETE_DATA;
        else
        {
            throw cRuntimeError("httpUtils parseTcpData - something went wrong: data length: %d and ramaining data to rcv: %d", data->length(),httpMessage->getRemainingDataToRecv() );
        }
    }

    void addBodyChunk(std::string* data, HttpBaseMessage* httpMessage)
    {
        int len = data->length();
        int remainingLength = httpMessage->getRemainingDataToRecv();
        EV << "httpUtils - addBodyChunk: data length: "<< len << "B. Remaining bytes: "<< remainingLength<< endl;

        if(httpMessage->getType() == RESPONSE)
        {
            HttpResponseMessage* resp = dynamic_cast<HttpResponseMessage*>(httpMessage);
            resp->addBodyChunk(data->substr(0, remainingLength));

        }
        else if(httpMessage->getType() == REQUEST)
        {
            HttpRequestMessage* resp = dynamic_cast<HttpRequestMessage*>(httpMessage);
            EV << "httpUtils - REQUEST "<< endl;
            resp->addBodyChunk(data->substr(0, remainingLength));
        }

        data->erase(0, remainingLength);
        httpMessage->setRemainingDataToRecv(remainingLength - (len - data->length()));
        EV << "httpUtils - addBodyChunk: Remaining bytes: "<< httpMessage->getRemainingDataToRecv()<< endl;

    }

    void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, const char* body)
    {
        EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpResponsePacket");
        auto resPkt = inet::makeShared<HttpResponseMessage>();
        resPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        resPkt->setCode(code);
        resPkt->setStatus(reason);
        if(body != nullptr)
        {
            resPkt->setBody(body);
            resPkt->setContentLength(strlen(body));
        }
        resPkt->setChunkLength(B(resPkt->getPayload().size()));
        packet->insertAtBack(resPkt);
        socket->send(packet);
    }

    void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, std::pair<std::string, std::string>& header, const char* body)
    {
        EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpResponsePacket");
        auto resPkt = inet::makeShared<HttpResponseMessage>();
        resPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        resPkt->setCode(code);
        resPkt->setStatus(reason);
        if(body != nullptr)
        {
            resPkt->setBody(body);
            resPkt->setContentLength(strlen(body));
        }
        resPkt->setHeaderField(header.first, header.second);

        resPkt->setChunkLength(B(resPkt->getPayload().size()));
        packet->insertAtBack(resPkt);
        socket->send(packet);
    }

    void sendHttpResponse(inet::TcpSocket *socket, const char* code, const char* reason, std::map<std::string, std::string>& headers, const char* body)
    {
        EV << "httpUtils - sendHttpResponse: code: " << code << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpResponsePacket");
        auto resPkt = inet::makeShared<HttpResponseMessage>();
        resPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        resPkt->setCode(code);
        resPkt->setStatus(reason);
        if(body != nullptr)
        {
            resPkt->setBody(body);
            resPkt->setContentLength(strlen(body));
        }
        std::map<std::string, std::string>::iterator it = headers.begin();
        std::map<std::string, std::string>::iterator end = headers.end();
        for(; it != end ; ++it)
        {
            resPkt->setHeaderField(it->first, it->second);
        }

        resPkt->setChunkLength(B(resPkt->getPayload().size()));
        packet->insertAtBack(resPkt);
        socket->send(packet);

    }

    void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, const char* body)
    {
        EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpRequestPacket");
        auto reqPkt = inet::makeShared<HttpRequestMessage>();
        reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        reqPkt->setMethod(method);
        reqPkt->setUri(uri);
        reqPkt->setHost(host);
        if(body != nullptr)
        {
            reqPkt->setBody(body);
            reqPkt->setContentLength(strlen(body));
        }
        reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
        packet->insertAtBack(reqPkt);
        socket->send(packet);
    }

    void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, std::pair<std::string, std::string>& header, const char* body)
    {
        EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpRequestPacket");
        auto reqPkt = inet::makeShared<HttpRequestMessage>();
        reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        reqPkt->setMethod(method);
        reqPkt->setUri(uri);
        reqPkt->setHost(host);
        if(body != nullptr)
        {
            reqPkt->setBody(body);
            reqPkt->setContentLength(strlen(body));
        }
        reqPkt->setHeaderField(header.first,header.second);
        reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
        packet->insertAtBack(reqPkt);
        socket->send(packet);

    }
    void sendHttpRequest(inet::TcpSocket *socket, const char* method, const char* uri, const char* host, std::map<std::string, std::string>& headers, const char* body)
    {
        EV << "httpUtils - sendHttpRequest: method: " << method << " to: " << socket->getRemoteAddress() << ":" << socket->getRemotePort() << endl;
        inet::Packet* packet = new inet::Packet("HttpRequestPacket");
        auto reqPkt = inet::makeShared<HttpRequestMessage>();
        reqPkt->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
        reqPkt->setMethod(method);
        reqPkt->setUri(uri);
        reqPkt->setHost(host);
        if(body != nullptr)
        {
            reqPkt->setBody(body);
            reqPkt->setContentLength(strlen(body));
        }
        std::map<std::string, std::string>::iterator it = headers.begin();
        std::map<std::string, std::string>::iterator end = headers.end();
        for(; it != end ; ++it)
        {
           reqPkt->setHeaderField(it->first, it->second);
        }
        reqPkt->setChunkLength(B(reqPkt->getPayload().size()));
        packet->insertAtBack(reqPkt);
        socket->send(packet);
    }




    void send200Response(inet::TcpSocket *socket, const char* body){
        sendHttpResponse(socket, "200", "OK" , body);
        return;
    }

    void send201Response(inet::TcpSocket *socket, const char* body){
            sendHttpResponse(socket, "201", "Created" , body);
            return;
    }

    void send201Response(inet::TcpSocket *socket, const char* body, std::pair<std::string, std::string>& header){
            HTTPResponsePacket resp = HTTPResponsePacket(CREATED);
            resp.setConnection("keep-alive");
            resp.setBody(body);
            resp.setHeaderField(header.first, header.second);
            sendPacket(resp.getPayload(), socket);
    }

    void send201Response(inet::TcpSocket *socket, const char* body,std::map<std::string, std::string>& headers){
            HTTPResponsePacket resp = HTTPResponsePacket(CREATED);
            resp.setConnection("keep-alive");
            resp.setBody(body);
            std::map<std::string, std::string>::iterator it = headers.begin();
            std::map<std::string, std::string>::iterator end = headers.end();
            for(; it != end ; ++it)
            {
                resp.setHeaderField(it->first, it->second);
            }
            sendPacket(resp.getPayload(), socket);
    }

    void send204Response(inet::TcpSocket *socket){
        sendHttpResponse(socket, "204", "No Content");
        return;
    }

    void send405Response(inet::TcpSocket *socket, const char* methods){
        HTTPResponsePacket resp = HTTPResponsePacket(BAD_METHOD);
        if(strcmp (methods,"") == 0)
            resp.setHeaderField("Allow: ", "GET, POST, DELETE, PUT");
        else{
            resp.setHeaderField("Allow: ", std::string(methods));
        }
        sendPacket(resp.getPayload(), socket);
    }

    void send400Response(inet::TcpSocket *socket){
        sendHttpResponse(socket, "400", "Bad Request" , "{ \"send400Response\" : \"TODO implement ProblemDetails\"}");
    }

    void send400Response(inet::TcpSocket *socket, const char *reason)
    {
        sendHttpResponse(socket, "400", "Bad Request" , reason);
        return;
    }

    void send404Response(inet::TcpSocket *socket){
        sendHttpResponse(socket, "404", "Not Found" , "{ \"send404Response\" : \"TODO implement ProblemDetails\"}");
        return;
    }

    void send505Response(inet::TcpSocket *socket){
        sendHttpResponse(socket, "505", "HTTP Version Not Supported");
        return;
    }

void send503Response(inet::TcpSocket *socket, const char *reason)
    {
        sendHttpResponse(socket, "503", "HTTP Version Not Supported" , reason);
        return;

    }

    void sendPostRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri)
    {
        sendHttpRequest(socket, "POST", uri, host, body);
        return;
    }

    void sendPutRequest(inet::TcpSocket *socket, const char* body, const char* host, const char* uri)
    {
        sendHttpRequest(socket, "PUT", uri, host, body);
        return;
    }


    void sendGetRequest(inet::TcpSocket *socket, const char* host, const char* uri,const char* body)
    {
        sendHttpRequest(socket, "GET", uri, host, body);
        return;
    }

    void sendDeleteRequest(inet::TcpSocket *socket, const char* host, const char* uri)
    {
        sendHttpRequest(socket, "DELETE", uri, host);
        return;
    }



    }
