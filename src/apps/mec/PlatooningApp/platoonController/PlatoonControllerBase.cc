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





