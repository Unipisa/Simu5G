//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef BACKGROUNDTRAFFICMANAGER_H_
#define BACKGROUNDTRAFFICMANAGER_H_

#include "common/LteCommon.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;

//
// BackgroundTrafficManager
//
class BackgroundTrafficManager : public cSimpleModule
{
  protected:

    // number of background UEs
    int numBgUEs_;

    // reference to all the background UEs
    std::vector<TrafficGeneratorBase*> bgUe_;

    // indexes of the backlogged bg UEs
    std::vector<int> backloggedBgUes_[2];

    // reference to the MAC layer of the e/gNodeB
    LteMacEnb* mac_;

    virtual void initialize(int stage);

    // define functions for interactions with the NIC

  public:
    BackgroundTrafficManager() {}
    virtual ~BackgroundTrafficManager() {}

    // invoked by the UE's traffic generator when new data is backlogged
    void notifyBacklog(int index, Direction dir);
};

#endif
