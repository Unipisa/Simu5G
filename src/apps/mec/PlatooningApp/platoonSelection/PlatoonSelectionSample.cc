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

#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionSample.h"

PlatoonSelectionSample::PlatoonSelectionSample()
{
}

PlatoonSelectionSample::~PlatoonSelectionSample()
{
    EV << "PlatoonSelectionSample::~PlatoonSelectionSample - Destructor called" << endl;
}

int PlatoonSelectionSample::findBestPlatoon(const ControllerMap& activeControllers, inet::Coord direction)
{
    EV << "PlatoonSelectionSample::findBestPlatoon - finding the best platoon for a given UE" << endl;

    // TODO implement custom algorithm

    int selectedPlatoon = -1;
    ControllerMap::const_iterator it = activeControllers.begin();
    for (; it != activeControllers.end(); ++it)
    {
        inet::Coord platoonDir = it->second->getDirection();

        // consider the platoon as candidate only if the direction is the same (TODO consider small errors)
        if (direction == platoonDir)
            selectedPlatoon = it->first;

        // TODO we should get the position of the UE, and compare it with the position of platoon members
        // For example, if the UE is closer than 500m, on the same lane, then select the platoon
    }

    EV << "PlatoonSelectionSample::findBestPlatoon - selected platoon with index " << selectedPlatoon << endl;

    return selectedPlatoon;
}






