#ifndef __PLATOONINGUTILS_
#define __PLATOONINGUTILS_

#include "inet/networklayer/common/L3Address.h"
#include "inet/common/geometry/common/Coord.h"


#define REGISTRATION_REQUEST "RegistrationRequest"
#define REGISTRATION_RESPONSE "RegistrationResponse"

#define JOIN_REQUEST "JoinRequest"
#define JOIN_RESPONSE "JoinResponse"

#define LEAVE_REQUEST "LeaveRequest"
#define LEAVE_RESPONSE "LeaveResponse"

#define PLATOON_CMD "PlatoonCommand"
typedef enum
{
    PLATOON_CONTROL_TIMER = 0,
    PLATOON_UPDATE_POSITION_TIMER = 1,
} PlatooningTimerType;

typedef struct
{
    inet::L3Address address;
    inet::Coord position;
    double speed;
    double timestamp;
} UEInfo;


#endif // __PLATOONINGUTILS_
