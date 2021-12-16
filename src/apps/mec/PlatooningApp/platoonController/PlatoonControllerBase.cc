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
    mecPlatooningApp_ = nullptr;
}

PlatoonControllerBase::PlatoonControllerBase(MECPlatooningApp* mecPlatooningApp)
{
    mecPlatooningApp_ = mecPlatooningApp;
}

PlatoonControllerBase::~PlatoonControllerBase()
{
    EV << "PlatoonControllerBase::~PlatoonControllerBase - Destructor called" << endl;
}



bool PlatoonControllerBase::addPlatoonMember()
{
    EV << "PlatoonControllerBase::addPlatoonMember - New member added to the platoon" << endl;

    return true;
}

bool PlatoonControllerBase::removePlatoonMember()
{
    EV << "PlatoonControllerBase::removePlatoonMember - Member removed from the platoon" << endl;

    return true;
}





