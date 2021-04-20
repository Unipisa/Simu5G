//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

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
};


enum HttpResponseCode { 
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
}


////
// Message class for HTTP requests
//
class HttpRequestMessage_m extends HttpBaseMessage
{
    string method;
    string uri;
    string host="";		// Host header for POST requests
}

//
// Message class for HTTP replies
//
class HttpResponseMessage_m extends HttpBaseMessage
{
    string code;	// e.g. 200 
    string status; 	// e.g. OK
}


