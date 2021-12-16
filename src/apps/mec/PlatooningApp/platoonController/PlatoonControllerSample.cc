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

#include "../platoonController/PlatoonControllerSample.h"

PlatoonControllerSample::PlatoonControllerSample()
{
}

PlatoonControllerSample::PlatoonControllerSample(MECPlatooningApp* mecPlatooningApp)
    : PlatoonControllerBase(mecPlatooningApp)
{
}

PlatoonControllerSample::~PlatoonControllerSample()
{
    EV << "PlatoonControllerSample::~PlatoonControllerSample - Destructor called" << endl;
}

bool PlatoonControllerSample::controlPlatoon()
{
    EV << "PlatoonControllerSample::controlPlatoon - calculating new acceleration values for all platoon members" << endl;

    // TODO implement controller

    EV << "PlatoonControllerSample::controlPlatoon() - Sending new acceleration values to the UEs" << endl;

    return true;
}






