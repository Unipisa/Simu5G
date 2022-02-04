#ifndef __PLATOONINGUTILS_
#define __PLATOONINGUTILS_

#include "inet/networklayer/common/L3Address.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
//#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"

//#define REGISTRATION_REQUEST "RegistrationRequest"
//#define REGISTRATION_RESPONSE "RegistrationResponse"
//
//#define JOIN_REQUEST "JoinRequest"
//#define JOIN_RESPONSE "JoinResponse"
//
//#define LEAVE_REQUEST "LeaveRequest"
//#define LEAVE_RESPONSE "LeaveResponse"
//
//#define PLATOON_CMD "PlatoonCommand"
//
//#define ADD_CONTROLLER "AddController"
//#define DEL_CONTROLLER "RemoveController"
//
//#define ADD_MEMBER "AddMember"
//#define DEL_MEMBER "RemoveMember"

class PlatoonControllerBase;

typedef enum
{
    REGISTRATION_REQUEST = 0,
    REGISTRATION_RESPONSE = 1,

    JOIN_REQUEST = 2,
    JOIN_RESPONSE = 3,

    LEAVE_REQUEST = 4,
    LEAVE_RESPONSE = 5,

    PLATOON_CMD = 6,

    AVAILABLE_PLATOONS_REQUEST = 7,
    AVAILABLE_PLATOONS_RESPONSE = 8,

    ADD_MEMBER_REQUEST = 9,
    ADD_MEMBER_RESPONSE = 10,

    REMOVE_MEMBER_REQUEST = 11,
    REMOVE_MEMBER_RESPONSE = 12

} PlatooningPacketType;




typedef enum
{
    PLATOON_CONTROL_TIMER = 0,
    PLATOON_UPDATE_POSITION_TIMER = 1,
} PlatooningTimerType;

//typedef struct
//{
//    PlatoonControllerBase* controller;
//    int mecProducerAppId;
//} PlatoonControllerStatus;

typedef struct
{
    inet::L3Address address;
    inet::Coord position;
    double speed;
    double timestamp;
} UEInfo;


typedef struct
{
    inet::L3Address address;
    int port;
} AppEndpoint;

typedef struct
{
    inet::L3Address address;
    int port;
    int producerAppId;
} ConsumerAppInfo;

typedef struct
{
    inet::L3Address address;
    int port;
    inet::TcpSocket *locationServiceSocket;
    inet::L3Address locationServiceAddress;
    int locationServicePort;
    // FIFO queue with the platoon index id of the pending requests
    std::queue<int> controllerPendingRequests;
} ProducerAppInfo;


typedef struct
{
    int producerApp;
    int platoonIndex;
} PlatoonIndex;



#endif // __PLATOONINGUTILS_
