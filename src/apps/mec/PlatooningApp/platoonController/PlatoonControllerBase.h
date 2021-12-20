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
#include "apps/mec/PlatooningApp/MECPlatooningProviderApp.h"

using namespace std;
using namespace omnetpp;

typedef std::set<int> MecAppIdSet;
typedef std::map<int, double> CommandList;

class MECPlatooningProviderApp;

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

  protected:

    friend class MECPlatooningProviderApp;

    // reference to the MEC Platooning App
    MECPlatooningProviderApp* mecPlatooningProviderApp_;

    // platoon controller identifier within a PlatoonProviderApp
    int index_;

    // periodicity for running the controller
    double controlPeriod_;

    // set of platoon members. It includes the ID of their MEC apps
    MecAppIdSet members_;

    // @brief add a new member to the platoon
    virtual bool addPlatoonMember(int mecAppId);

    // @brief remove a member from the platoon
    virtual bool removePlatoonMember(int mecAppId);

    // @brief run the global platoon controller
    virtual const CommandList* controlPlatoon() = 0;

  public:
    PlatoonControllerBase();
    PlatoonControllerBase(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod = 1.0);
    virtual ~PlatoonControllerBase();
};

#endif
