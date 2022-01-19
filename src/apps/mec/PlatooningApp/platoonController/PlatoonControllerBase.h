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

#ifndef __PLATOONCONTROLLERBASE_H_
#define __PLATOONCONTROLLERBASE_H_

#include "omnetpp.h"
#include "apps/mec/PlatooningApp/MECPlatooningProviderApp.h"
#include "apps/mec/PlatooningApp/PlatooningUtils.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonVehicleInfo.h"

using namespace std;
using namespace omnetpp;

typedef struct
{
    inet::Coord lastPosition = inet::Coord::NIL;
    simtime_t lastTimeStamp = 0;
} UEStatus;

typedef std::set<int> MecAppIdSet;
typedef std::map<inet::L3Address, UEStatus> UEStatuses;
typedef std::map<int, PlatoonVehicleInfo> PlatoonMembersInfo;
typedef std::map<int, double> CommandList;

class MECPlatooningProviderApp;

/*
 * PlatoonControllerBase
 *
 * This abstract class provides basic methods and data structure for implementing
 * a platoon controller, i.e. the entity responsible for generating the correct
 * commands (i.e. acceleration values) to the platoon members.
 * To implement your own controller, create your own class by extending this base
 * class and implement the controlPlatoon() member function.
 */
class PlatoonControllerBase
{

  protected:

    friend class MECPlatooningProviderApp;

    // reference to the MEC Platooning App
    MECPlatooningProviderApp* mecPlatooningProviderApp_;

    // platoon controller identifier within a PlatoonProviderApp
    int index_;

    // current direction of the platoon
    inet::Coord direction_;

    // target speed for the platoon
    double targetSpeed_;

    // minimum acceptable acceleration value
    double minAcceleration_;

    // maximum acceptable acceleration value
    double maxAcceleration_;

    // periodicity for running the controller
    double controlPeriod_;

    // TODO  not used, yet. Only  the addresses are used to retrieve the location.
    // replace it with the structure provided by GN
    UEStatuses ueInfo_;

    // info about platoon members. The key is the ID of their MEC apps
    PlatoonMembersInfo membersInfo_;

    // store the id of the vehicles sorted according to their position in the platoon
    std::list<int> platoonPositions_;

    // @brief add a new member to the platoon
    virtual bool addPlatoonMember(int mecAppId);

    // @brief remove a member from the platoon
    virtual bool removePlatoonMember(int mecAppId);

    // @brief add a new member to the platoon with address
    virtual bool addPlatoonMember(int mecAppId, inet::L3Address);

    // @brief remove a member from the platoon with address
    virtual bool removePlatoonMember(int mecAppId, inet::L3Address);

    // @brief invoked by the mecPlatooningProviderApp to update the location of the UEs of a platoon
    virtual void updatePlatoonPositions(std::vector<UEInfo>*);
    // @brief require the location of the platoon UEs to the mecPlatooningProviderApp
    virtual void requirePlatoonPositions();


    // @brief run the global platoon controller
    virtual const CommandList* controlPlatoon() = 0;

  public:
    PlatoonControllerBase(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod = 1.0);
    virtual ~PlatoonControllerBase();

    void setDirection(inet::Coord direction) { direction_ = direction; }
    inet::Coord getDirection() { return direction_; }

    void setTargetSpeed(double speed) { targetSpeed_ = speed; }
    double getTargetSpeed() { return targetSpeed_; }
};

#endif
