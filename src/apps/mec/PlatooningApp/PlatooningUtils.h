#ifndef __PLATOONINGUTILS_
#define __PLATOONINGUTILS_


typedef struct
{
    inet::L3Address address;
    inet::Coord position;
    double speed;
    double timestamp;
} UEInfo;


#endif // __PLATOONINGUTILS_
