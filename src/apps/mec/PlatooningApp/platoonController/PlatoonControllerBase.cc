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

PlatoonControllerBase::PlatoonControllerBase()
{
    mecPlatooningProviderApp_ = nullptr;
}

PlatoonControllerBase::PlatoonControllerBase(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod)
{
    mecPlatooningProviderApp_ = mecPlatooningProviderApp;
    index_ = index;
    controlPeriod_ = controlPeriod;
}

PlatoonControllerBase::~PlatoonControllerBase()
{
    EV << "PlatoonControllerBase::~PlatoonControllerBase - Destructor called" << endl;
}



bool PlatoonControllerBase::addPlatoonMember(int mecAppId)
{
    EV << "PlatoonControllerBase::addPlatoonMember - New member [" << mecAppId << "] added to the platoon" << endl;

    if (members_.empty())
    {
        // start controlling the platoon, set a timer
        mecPlatooningProviderApp_->startControllerTimer(index_, controlPeriod_);
    }


    members_.insert(mecAppId);

    return true;
}

bool PlatoonControllerBase::addPlatoonMember(int mecAppId, inet::L3Address ueAddress)
{
    EV << "PlatoonControllerBase::addPlatoonMember - New member [" << mecAppId << "] with IP address [" << ueAddress << "] added to the platoon" << endl;

    addPlatoonMember(mecAppId);
    UEStatus newUe;
    ueInfo_.insert(std::pair<inet::L3Address, UEStatus>(ueAddress, newUe) );
    return true;
}



bool PlatoonControllerBase::removePlatoonMember(int mecAppId)
{
    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] removed from the platoon" << endl;

    members_.erase(mecAppId);

    if (members_.empty())
    {
        // stop controlling the platoon, stop the timer
        mecPlatooningProviderApp_->stopControllerTimer(index_);
    }

    return true;
}

bool PlatoonControllerBase::removePlatoonMember(int mecAppId, inet::L3Address ueAddress)
{
    EV << "PlatoonControllerBase::removePlatoonMember - Member [" << mecAppId << "] with IP address [" << ueAddress << "] removed from the platoon" << endl;
    removePlatoonMember(mecAppId);
    //iterate
    ueInfo_.erase(ueAddress);

    return true;
}

void PlatoonControllerBase::requirePlatoonPositions()
{
    EV << "PlatoonControllerBase::requirePlatoonPositions" << endl;
    std::set<inet::L3Address> ueAddresses;
    for(UEStatuses::const_iterator it = ueInfo_.begin(); it != ueInfo_.end(); ++it)
    {
        ueAddresses.insert(it->first);
    }
    EV << "PlatoonControllerBase::requirePlatoonPositions "<< ueAddresses.size() << endl;
    mecPlatooningProviderApp_->requirePlatoonLocations(index_, &ueAddresses);
}

void PlatoonControllerBase::updatePlatoonPositions(std::vector<UEInfo>* uesInfo)
{
    EV << "PlatoonControllerBase::updatePlatoonPositions - size " << uesInfo->size() << endl;
    auto it = uesInfo->begin();
    for(;it != uesInfo->end(); ++it)
    {
        EV << "UE info:\naddress ["<< it->address.str() << "]\n" <<
                "timestamp: " << it->timestamp << "\n" <<
                "coords: ("<<it->position.x<<"," << it->position.y<<"," << it->position.z<<")\n"<<
                "speed: " << it->speed << " mps" << endl;

    }

}




