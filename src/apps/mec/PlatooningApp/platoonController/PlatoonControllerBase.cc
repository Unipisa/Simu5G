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

PlatoonControllerBase::PlatoonControllerBase(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod, double updatePositionPeriod)
{
    mecPlatooningProviderApp_ = mecPlatooningProviderApp;
    index_ = index;
    controlPeriod_ = controlPeriod;
    updatePositionPeriod_ = updatePositionPeriod;

    // TODO make these parametric
    minAcceleration_ = -5.0;
    maxAcceleration_ = 1.5;

    targetSpeed_ = 10.0;
}

PlatoonControllerBase::~PlatoonControllerBase()
{
    EV << "PlatoonControllerBase::~PlatoonControllerBase - Destructor called" << endl;
}

bool PlatoonControllerBase::addPlatoonMember(int mecAppId, inet::Coord position, inet::L3Address ueAddress)
{
    if (membersInfo_.empty())
    {
        // update positions, set a timer
        UpdatePositionTimer* posTimer = new UpdatePositionTimer("PlatooningTimer");
        posTimer->setType(PLATOON_UPDATE_POSITION_TIMER);
        posTimer->setControllerIndex(index_);
        posTimer->setPeriod(controlPeriod_);
        mecPlatooningProviderApp_->startTimer(posTimer, controlPeriod_ - 0.02); // TODO check timing

        // start controlling the platoon, set a timer
        ControlTimer* ctrlTimer = new ControlTimer("PlatooningTimer");
        ctrlTimer->setType(PLATOON_CONTROL_TIMER);
        ctrlTimer->setControllerIndex(index_);
        ctrlTimer->setPeriod(controlPeriod_);
        mecPlatooningProviderApp_->startTimer(ctrlTimer, controlPeriod_);
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
    membersInfo_[mecAppId] = newVehicleInfo;

    EV << "PlatoonControllerBase::addPlatoonMember - New member [" << mecAppId << "] added to the platoon" << endl;
    return true;
}

bool PlatoonControllerBase::removePlatoonMember(int mecAppId)
{
    PlatoonMembersInfo::iterator it = membersInfo_.find(mecAppId);
    if (it == membersInfo_.end())
    {
        EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] was not part of this platoon" << endl;
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
        mecPlatooningProviderApp_->stopTimer(index_, PLATOON_UPDATE_POSITION_TIMER);
        mecPlatooningProviderApp_->stopTimer(index_, PLATOON_CONTROL_TIMER);
    }

    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] removed from the platoon" << endl;
    return true;
}

std::set<inet::L3Address> PlatoonControllerBase::getUeAddressList()
{
    std::set<inet::L3Address> ueAddresses;
    PlatoonMembersInfo::const_iterator it = membersInfo_.begin();
    for(; it != membersInfo_.end(); ++it)
    {
        ueAddresses.insert(it->second.getUeAddress());
    }
    EV << "PlatoonControllerBase::getUeAddressList "<< ueAddresses.size() << endl;

    return ueAddresses;
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




