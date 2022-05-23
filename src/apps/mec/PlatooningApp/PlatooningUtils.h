#ifndef __PLATOONINGUTILS_
#define __PLATOONINGUTILS_

#include <queue>

#include "inet/networklayer/common/L3Address.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"


#include "apps/mec/PlatooningApp/platoonController/PlatoonVehicleInfo.h"

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
    REMOVE_MEMBER_RESPONSE = 12,

    CONF_CONTROLLER = 13,
    CONF_CONTROLLER_RESPONSE = 14,

    DISCOVER_PLATOONS_REQUEST = 15,
    DISCOVER_PLATOONS_RESPONSE = 16,

    DISCOVER_ASSOCIATE_PLATOON_REQUEST = 17,
    DISCOVER_ASSOCIATE_PLATOON_RESPONSE = 18,

    ASSOCIATE_PLATOON_REQUEST = 19,
    ASSOCIATE_PLATOON_RESPONSE = 20,

    CONTROLLER_NOTIFICATION = 21,
    JOIN_MANOEUVRE_NOTIFICATION = 22,
    LEAVE_MANOEUVRE_NOTIFICATION = 23,
    QUEUED_JOIN_NOTIFICATION = 24,
    QUEUED_LEAVE_NOTIFICATION = 25,

    CONTROLLER_INSTANTIATION_REQUEST = 26,
    CONTROLLER_INSTANTIATION_RESPONSE = 27
} PlatooningPacketType;

typedef enum
{
    NEW_MEMBER = 0,
    REMOVED_MEMBER = 1,
    NEW_ORDER= 2,
    HEARTBEAT = 3
} ControllerNotificationType;


typedef enum
{
    IDLE = 0,
    DISCOVERY = 1,
    DISCOVERY_AND_ASSOCIATE = 2,
    ASSOCIATE = 3,
    JOIN = 4,
    JOINED_PLATOON = 5,
    LEAVE = 6,
    MANOEUVRING = 7

} PlatooningConsumerAppState;

typedef enum {
    NOT_JOINED,
    JOINED,
    MANOEUVRE_TO_JOIN_PLATOON,
    MANOEUVRE_TO_LEAVE_PLATOON,
    QUEUED_JOIN,
    QUEUED_LEAVE,
} UePlatoonStatus;


typedef enum
{
    PLATOON_LONGITUDINAL_CONTROL_TIMER = 0,
    PLATOON_LONGITUDINAL_UPDATE_POSITION_TIMER = 1,

    PLATOON_LATERAL_CONTROL_TIMER = 2,
    PLATOON_LATERAL_UPDATE_POSITION_TIMER = 3,
} PlatooningTimerType;

typedef enum
{
    INACTIVE_CONTROLLER = 0,
    CRUISE = 1,
    MANOEUVRE = 2,
    CHECK_NEXT_STATE = 3
} ControllerState;

typedef enum
{
    NO_MANOEUVRE = 0,
    JOIN_MANOUVRE = 1,
    LEAVE_MANOUVRE = 2,
    CHECK_POSITION = 3
} ManoeuvreType;

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
    UePlatoonStatus status;

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
    simtime_t lastResponse = 0;
} ProducerAppInfo;


/*
 * TODO
 * This struct is becoming big, think about transform it in a class
 */
typedef struct
{
    int controllerId;
    AppEndpoint controllerEndpoint;
    inet::Coord direction;
    std::vector<PlatoonVehicleInfo> vehicles;
    int queuedJoinRequests;
    std::map<int, ConsumerAppInfo> associatedConsumerApps;
    cModule * pointerToMECPlatooningControllerApp;
    simtime_t lastHeartBeat;
    int mecAppId;
    bool isInstantiated;
} PlatoonControllerStatus;

typedef struct
{
    int producerApp;
    int platoonIndex;
} PlatoonIndex;

typedef struct
{
    inet::Coord acceleration;
    inet::L3Address address;
    bool isManouveringEnded;
} CommandInfo;





#endif // __PLATOONINGUTILS_
