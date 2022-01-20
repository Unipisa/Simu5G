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

#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerSample.h"

PlatoonControllerSample::PlatoonControllerSample(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod)
    : PlatoonControllerBase(mecPlatooningProviderApp, index, controlPeriod)
{
}

PlatoonControllerSample::~PlatoonControllerSample()
{
    EV << "PlatoonControllerSample::~PlatoonControllerSample - Destructor called" << endl;
}

const CommandList* PlatoonControllerSample::controlPlatoon()
{
    EV << "PlatoonControllerSample::controlPlatoon - calculating new acceleration values for all platoon members" << endl;

    // TODO implement controller

    CommandList* cmdList = new CommandList();

    PlatoonMembersInfo::iterator it = membersInfo_.begin();
    for (; it != membersInfo_.end(); ++it)
    {
        EV << "PlatoonControllerSample::control() - Computing new command for MEC app " << it->first << "..." << endl;

        // Dummy controller
        double newAcceleration = 0.0;

        (*cmdList)[it->first] = newAcceleration;

        EV << "PlatoonControllerSample::control() - New acceleration value for " << it->first << " [" << newAcceleration << "]" << endl;
    }
    return cmdList;
}
