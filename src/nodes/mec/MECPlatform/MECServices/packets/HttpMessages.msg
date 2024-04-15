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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

enum HttpMsgType
{
    REQUEST  = 0;
    RESPONSE = 1;
    UNKNOWN  = 2;
};

enum HttpRequestState {
    CORRECT 		= 0;
    BAD_REQ_LINE 	= 1;
    BAD_HEADER 		= 2;
    BAD_HTTP 		= 3;
	BAD_REQUEST		= 4;
    DIFF_HOST 		= 5;
    UNDEFINED 		= 6;
    BAD_REQ_METHOD 	= 7;
    BAD_RES_LINE 	= 8;
    BAD_REQ_URI		= 9;
};

enum HttpResponseStatus { 
	OK 					= 0;	// 200
    CREATED 			= 1;    // 201
    NO_CONTENT 			= 2; 	// 204
    BAD_REQ 			= 3;    // 400
    UNAUTH 				= 4;    // 401
    FORBIDDEN 			= 5;  	// 403
    NOT_FOUND			= 6;  	// 404
    NOT_ACC 			= 7;    // 406
    TOO_REQS 			= 8;   	// 429
    BAD_METHOD 			= 9; 	// 405
    HTTP_NOT_SUPPORTED	= 10; 	// 505
    SERV_UNAV 			= 11;    //503
  };

//
// Base class for HTTP messages
//
class HttpBaseMessage extends inet::FieldsChunk
{
    int sockId;								 // socket Id used to retrieve the tcp socket objects from the map
    HttpRequestState state;					 // used to respond with the correct HTTP response
    bool isReceivingMsg;					 // flag used for HTTP msgs within more TCP segments
    HttpMsgType type;				         // REQUEST or RESPONSE or UNKNOWN
    string body; 							 // body of the HTTp message if present
    string httpProtocol = "HTTP/1.1";		 // http protocol, defaul HTTP/1.1
    int contentLength;						 // Content-Length header
    int remainingDataToRecv;				 // Counter for HTTP msg within more TCP segments
    string contentType = "application/json"; // Content-Type header 
    string connection = "keep-alive";		 // Connection  header
    simtime_t arrivalTime = 0;					 //  Point in time when the messagge arrives (used by the Service to calculate response time)
}

////
// Message class for HTTP requests
//
class HttpRequestMessage_m extends HttpBaseMessage
{
    string method;
    string parameters;
    string uri;
    string host="";		// Host header for POST requests
    double responseTime = 0; // used by the ServiceBase when the load generator flag is set
}

//
// Message class for HTTP replies
//
class HttpResponseMessage_m extends HttpBaseMessage
{
    int code;	// e.g. 200 
    string status; 	// e.g. OK
}
