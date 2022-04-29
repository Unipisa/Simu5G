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

#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"
#include "apps/mec/PlatooningApp/MECPlatooningControllerApp.h"

PlatoonControllerBase::PlatoonControllerBase(MECPlatooningControllerApp* mecPlatooningControllerApp, int index, double controlPeriod, double updatePositionPeriod)
{
    mecPlatooningControllerApp_ = mecPlatooningControllerApp;
    index_ = index;
    controlPeriod_ = controlPeriod;
    updatePositionPeriod_ = updatePositionPeriod;

    // TODO make these parametric
    minAcceleration_ = -5.0;
    maxAcceleration_ = 1.5;

    targetSpeed_ = 25;
}

PlatoonControllerBase::~PlatoonControllerBase()
{
    EV << "PlatoonControllerBase::~PlatoonControllerBase - Destructor called" << endl;
}

bool PlatoonControllerBase::addPlatoonMember(int mecAppId, int producerAppId, inet::Coord position, inet::L3Address ueAddress)
{
    if (membersInfo_.empty())
    {
        // update positions, set a timer
        UpdatePositionTimer* posTimer = new UpdatePositionTimer("PlatooningTimer");
        posTimer->setType(PLATOON_UPDATE_POSITION_TIMER);
        posTimer->setControllerId(index_);
        posTimer->setPeriod(controlPeriod_);
        mecPlatooningControllerApp_->startTimer(posTimer, updatePositionPeriod_); // TODO check timing

        // start controlling the platoon, set a timer
        ControlTimer* ctrlTimer = new ControlTimer("PlatooningTimer");
        ctrlTimer->setType(PLATOON_CONTROL_TIMER);
        ctrlTimer->setControllerId(index_);
        ctrlTimer->setPeriod(controlPeriod_);
        mecPlatooningControllerApp_->startTimer(ctrlTimer, controlPeriod_);
    }

    // assign correct positions in the platoon
    // we compare the current position of the new vehicle against the position of other members,
    // starting from the platoon leader. We compute the of coordinates difference between the new vehicle and
    // the current member in the loop. The position of the new vehicle is found as soon as we find that
    // the difference has not the same sign as the direction of the platoon
    bool found = false;
    std::list<int>::iterator pt = platoonPositions_.begin();
    for (; pt != platoonPositions_.end(); ++pt)
    {
        inet::Coord curMemberPos = membersInfo_.at(*pt).getPosition();
        inet::Coord diff = curMemberPos - position;

        if (diff.getSign().x != direction_.getSign().x || diff.getSign().y != direction_.getSign().y)
        {
            // insert the new vehicle in front of the current member
            platoonPositions_.insert(pt, mecAppId);
            found = true;
            break;
        }
        // else current member is in front of the new vehicle, keep scanning
    }
    if (!found)
        platoonPositions_.push_back(mecAppId);

    // add vehicle info to the relevant data structure
    PlatoonVehicleInfo newVehicleInfo;
    newVehicleInfo.setPosition(position);
    newVehicleInfo.setUeAddress(ueAddress);
    newVehicleInfo.setMecAppId(mecAppId);
    newVehicleInfo.setProducerAppId(producerAppId);
    membersInfo_[mecAppId] = newVehicleInfo;

    EV << "PlatoonControllerBase::addPlatoonMember - New member [" << mecAppId << "] added to platoon " << index_ << endl;
    return true;
}

bool PlatoonControllerBase::removePlatoonMember(int mecAppId)
{
    PlatoonMembersInfo::iterator it = membersInfo_.find(mecAppId);
    if (it == membersInfo_.end())
    {
        EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] was not part of platoon " << index_ << endl;
        return false;
    }

    // remove mecAppId from relevant data structures
    std::list<int>::iterator pt = platoonPositions_.begin();
    for ( ; pt != platoonPositions_.end(); ++pt)
    {
        if (*pt == mecAppId)
        {
            platoonPositions_.erase(pt);
            break;
        }
    }
    membersInfo_.erase(mecAppId);

    // check if the platoon is now empty
    if (membersInfo_.empty())
    {
        // stop controlling the platoon, stop the timer
        mecPlatooningControllerApp_->stopTimer(PLATOON_UPDATE_POSITION_TIMER);
        mecPlatooningControllerApp_->stopTimer(PLATOON_CONTROL_TIMER);
    }

    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] removed from platoon " << index_ << endl;
    return true;
}

std::map<int, std::set<inet::L3Address> > PlatoonControllerBase::getUeAddressList()
{
    std::map<int, std::set<inet::L3Address> > ueAddresses;
    PlatoonMembersInfo::const_iterator it = membersInfo_.begin();
    for(; it != membersInfo_.end(); ++it)
    {
        ueAddresses[it->second.getProducerAppId()].insert(it->second.getUeAddress());
    }
    EV << "PlatoonControllerBase::getUeAddressList "<< ueAddresses.size() << endl;

    return ueAddresses;
}

void PlatoonControllerBase::adjustPositions()
{
    /*
     * find the most recent timestamp
     * for every older timestamp compute the new position
     */
    double now = simTime().dbl()*1000;

    for(auto &vehicle: membersInfo_)
    {
        double deltaTs = (now - vehicle.second.getTimestamp())/1000; //to ms
        EV << "PlatoonControllerBase::adjustPositions() - deltaTs is " << deltaTs << endl;

        // TODO consider to adjust the position for every coord!
        EV << "PlatoonControllerBase::adjustPositions() - old position: " << vehicle.second.getPosition().str() << endl;
        double newPosition = vehicle.second.getPosition().x + vehicle.second.getSpeed() * deltaTs + 0.5 * vehicle.second.getAcceleration() * deltaTs * deltaTs; // TODO add acceleration part

        double newSpeed = vehicle.second.getSpeed() + vehicle.second.getAcceleration() * deltaTs;
        vehicle.second.setPositionX(newPosition);
        vehicle.second.setSpeed(newSpeed);
        vehicle.second.setTimestamp(now);

        EV << "PlatoonControllerBase::adjustPositions() - new position: " << vehicle.second.getPosition().str() << endl;
    }
}


void PlatoonControllerBase::updatePlatoonPositions(std::vector<UEInfo>* uesInfo)
{
    EV << "PlatoonControllerBase::updatePlatoonPositions - size " << uesInfo->size() << endl;
    auto it = uesInfo->begin();
    for(;it != uesInfo->end(); ++it)
    {
        PlatoonMembersInfo::iterator mit = membersInfo_.begin();
        for(; mit != membersInfo_.end(); ++mit)
        {
            if (mit->second.getUeAddress() == it->address)
            {
                mit->second.setLastPosition(mit->second.getPosition());
                mit->second.setLastSpeed(mit->second.getSpeed());
                mit->second.setLastTimestamp(mit->second.getTimestamp());

                mit->second.setPosition(it->position);
                mit->second.setSpeed(it->speed);
                mit->second.setTimestamp(it->timestamp);
            }
        }

        EV << "UE info:\naddress ["<< it->address.str() << "]\n" <<
                "timestamp: " << it->timestamp << "\n" <<
                "coords: ("<<it->position.x<<"," << it->position.y<<"," << it->position.z<<")\n"<<
                "speed: " << it->speed << " mps" << endl;
    }
}


nlohmann::json PlatoonControllerBase::dumpPlatoonToJSON() const
{
    nlohmann::json dumpedPlatoon;
    nlohmann::json platoonMembers;
    dumpedPlatoon["targetSpeed"] = targetSpeed_;
    dumpedPlatoon["direction"]["x"] = direction_.x;
    dumpedPlatoon["direction"]["y"] = direction_.y;
    dumpedPlatoon["direction"]["z"] = direction_.z;
    dumpedPlatoon["minAcceleration"] = minAcceleration_;
    dumpedPlatoon["maxAcceleration"] = maxAcceleration_;
    dumpedPlatoon["controlPeriod"] = controlPeriod_;
    dumpedPlatoon["updatePositionPeriod"] = updatePositionPeriod_;
    for(const auto& vehiclePos: platoonPositions_)
    {
        auto vehicle = membersInfo_.at(vehiclePos);
        nlohmann::json jsonVehicle;
        jsonVehicle["id"] = vehiclePos;
        jsonVehicle["timestamp"] = vehicle.getTimestamp();
        jsonVehicle["speed"] = vehicle.getSpeed();
        Coord pos = vehicle.getPosition();
        jsonVehicle["position"]["x"] = pos.x;
        jsonVehicle["position"]["y"] = pos.y;
        jsonVehicle["position"]["z"] = pos.z;
        platoonMembers.push_back(jsonVehicle);
    }
    nlohmann::json platoonInfo;
    platoonInfo["platoonInfo"] = dumpedPlatoon;
    return  platoonInfo;
}

std::vector<PlatoonVehicleInfo> PlatoonControllerBase::getPlatoonMembers()
{
    std::vector<PlatoonVehicleInfo> platoons;
    for(auto& vehicle: membersInfo_)
    {
        platoons.push_back(vehicle.second);
    }

    return platoons;
}


