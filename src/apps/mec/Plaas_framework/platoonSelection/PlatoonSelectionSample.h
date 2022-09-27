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

#ifndef __PLATOONSELECTIONSAMPLE_H_
#define __PLATOONSELECTIONSAMPLE_H_

#include "../../Plaas_framework/platoonSelection/PlatoonSelectionBase.h"

using namespace std;
using namespace omnetpp;

/*
 * PlatoonSelectionSample
 *
 * This class implementes a sample algorithm for selecting a platoon when a new
 * join request is received.
 */
class PlatoonSelectionSample : public PlatoonSelectionBase
{
  protected:

    // @brief find the best platoon for the given UE
    //
    // this function must return the index of the selected platoon, -1 if no
    // platoons are available
    virtual int findBestPlatoon(const ControllerMap& activeControllers, inet::Coord position, inet::Coord direction);
    virtual PlatoonIndex findBestPlatoon(const int localProducerApp, const ControllerMap& activeControllers, const GlobalAvailablePlatoons& globalControllers, inet::Coord position, inet::Coord direction);

  public:
    PlatoonSelectionSample();
    virtual ~PlatoonSelectionSample();
};

#endif
