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

#ifndef __PLATOONSELECTIONBASE_H_
#define __PLATOONSELECTIONBASE_H_

#include "omnetpp.h"
#include "apps/mec/PlatooningApp/MECPlatooningProducerApp.h"

using namespace std;
using namespace omnetpp;

class MECPlatooningProducerApp;
class PlatoonControllerBase;

typedef std::map<int, PlatoonControllerStatus*> ControllerMap;
typedef std::map<int, std::map<int, PlatoonControllerStatus *> > GlobalAvailablePlatoons;


/*
 * PlatoonSelectionBase
 *
 * This abstract class provides basic methods and data structure for implementing
 * a platoon selection algorithm, i.e. which platoon should be selected when a new
 * join request is received.
 * To implement your own algorithm, create your own class by extending this base
 * class and implement the findBestPlatoon() member function.
 */
class PlatoonSelectionBase
{

    friend class MECPlatooningApp;
    friend class MECPlatooningProducerApp;

  protected:

    // @brief find the best platoon for the given UE
    //
    // this function must return the index of the selected platoon, -1 if no
    // platoons are available
    virtual int findBestPlatoon(const ControllerMap& activeControllers, inet::Coord position, inet::Coord direction) = 0;
    virtual PlatoonIndex findBestPlatoon(const int localProducerApp, const ControllerMap& activeControllers, const GlobalAvailablePlatoons& globalControllers, inet::Coord position, inet::Coord direction) = 0;


  public:
    PlatoonSelectionBase(){}
    virtual ~PlatoonSelectionBase() {}
};

#endif
