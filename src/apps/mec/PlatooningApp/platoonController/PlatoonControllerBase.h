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

#ifndef __PLATOONCONTROLLERBASE_H_
#define __PLATOONCONTROLLERBASE_H_

#include "omnetpp.h"
#include "apps/mec/PlatooningApp/MECPlatooningApp.h"

using namespace std;
using namespace omnetpp;

class MECPlatooningApp;

/*
 * PlatoonControllerBase
 *
 * This abstract class provides basic methods and data structure for implementing
 * a platoon controller, i.e. the entity responsible for generating the correct
 * commands (i.e. acceleration values) to the platoon members.
 * To implement your own controller, create your own class by extending this base
 * class and implement the controlPlatoon() member function.
 */
class PlatoonControllerBase
{
    friend class MECPlatooningApp;
    friend class MECPlatooningProviderApp;

    // reference to the MEC Platooning App
    MECPlatooningApp* mecPlatooningApp_;

    // temp
    double  newAcceleration_;

  protected:

    // @brief add a new member to the platoon
    virtual bool addPlatoonMember();

    // @brief remove a member from the platoon
    virtual bool removePlatoonMember();

    // @brief run the global platoon controller
    virtual bool controlPlatoon() = 0;

  public:
    PlatoonControllerBase();
    PlatoonControllerBase(MECPlatooningApp* mecPlatooningApp);
    virtual ~PlatoonControllerBase();
};

#endif
