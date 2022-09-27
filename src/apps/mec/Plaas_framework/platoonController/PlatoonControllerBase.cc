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

#include "../../Plaas_framework/platoonController/PlatoonControllerBase.h"

#include "../../Plaas_framework/MECPlatooningControllerApp.h"

PlatoonControllerBase::PlatoonControllerBase(MECPlatooningControllerApp* mecPlatooningControllerApp, int index, double controlPeriod, double updatePositionPeriod)
{
    mecPlatooningControllerApp_ = mecPlatooningControllerApp;
    index_ = index;
    controlPeriod_ = controlPeriod;
    updatePositionPeriod_ = updatePositionPeriod;

    platoonPositions_ = mecPlatooningControllerApp_->getPlatoonPositions();
    membersInfo_ = mecPlatooningControllerApp_->getMemberInfo();

    // TODO make these parametric
    minAcceleration_ = -5.0;
    maxAcceleration_ = 5; //1.5;//1.5;

    targetSpeed_ = 25;
}

PlatoonControllerBase::~PlatoonControllerBase()
{
    EV << "PlatoonControllerBase::~PlatoonControllerBase - Destructor called" << endl;
}


//bool PlatoonControllerBase::removePlatoonMember(int mecAppId)
//{
//    PlatoonMembersInfo::iterator it = membersInfo_.find(mecAppId);
//    if (it == membersInfo_.end())
//    {
//        EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] was not part of platoon " << index_ << endl;
//        return false;
//    }
//
//    // remove mecAppId from relevant data structures
//    std::list<int>::iterator pt = platoonPositions_.begin();
//    for ( ; pt != platoonPositions_.end(); ++pt)
//    {
//        if (*pt == mecAppId)
//        {
//            platoonPositions_.erase(pt);
//            break;
//        }
//    }
//    membersInfo_.erase(mecAppId);
//
//    // check if the platoon is now empty
//    if (membersInfo_.empty())
//    {
//        // stop controlling the platoon, stop the timer
//        mecPlatooningControllerApp_->stopTimer(PLATOON_UPDATE_POSITION_TIMER);
//        mecPlatooningControllerApp_->stopTimer(PLATOON_CONTROL_TIMER);
//    }
//
//    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] removed from platoon " << index_ << endl;
//    return true;
//}


//nlohmann::json PlatoonControllerBase::dumpPlatoonToJSON() const
//{
//    nlohmann::json dumpedPlatoon;
//    nlohmann::json platoonMembers;
//    dumpedPlatoon["targetSpeed"] = targetSpeed_;
//    dumpedPlatoon["direction"]["x"] = direction_.x;
//    dumpedPlatoon["direction"]["y"] = direction_.y;
//    dumpedPlatoon["direction"]["z"] = direction_.z;
//    dumpedPlatoon["minAcceleration"] = minAcceleration_;
//    dumpedPlatoon["maxAcceleration"] = maxAcceleration_;
//    dumpedPlatoon["controlPeriod"] = controlPeriod_;
//    dumpedPlatoon["updatePositionPeriod"] = updatePositionPeriod_;
//    for(const auto& vehiclePos: platoonPositions_)
//    {
//        auto vehicle = membersInfo_.at(vehiclePos);
//        nlohmann::json jsonVehicle;
//        jsonVehicle["id"] = vehiclePos;
//        jsonVehicle["timestamp"] = vehicle.getTimestamp();
//        jsonVehicle["speed"] = vehicle.getSpeed();
//        Coord pos = vehicle.getPosition();
//        jsonVehicle["position"]["x"] = pos.x;
//        jsonVehicle["position"]["y"] = pos.y;
//        jsonVehicle["position"]["z"] = pos.z;
//        platoonMembers.push_back(jsonVehicle);
//    }
//    nlohmann::json platoonInfo;
//    platoonInfo["platoonInfo"] = dumpedPlatoon;
//    return  platoonInfo;
//}

//std::vector<PlatoonVehicleInfo> PlatoonControllerBase::getPlatoonMembers()
//{
//    std::vector<PlatoonVehicleInfo> platoons;
//    for(auto& vehicle: membersInfo_)
//    {
//        platoons.push_back(vehicle.second);
//    }
//
//    return platoons;
//}


