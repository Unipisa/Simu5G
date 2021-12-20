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

int PlatoonSelectionSample::findBestPlatoon(const ControllerMap& activeControllers)
{
    EV << "PlatoonSelectionSample::findBestPlatoon - finding the best platoon for a given UE" << endl;

    // TODO implement custom algorithm

    int selectedPlatoon = -1;
    ControllerMap::const_iterator it = activeControllers.begin();
    for (; it != activeControllers.end(); ++it)
    {
        selectedPlatoon = it->first;
    }

    EV << "PlatoonSelectionSample::findBestPlatoon - selected platoon with index " << selectedPlatoon << endl;

    return selectedPlatoon;
}






