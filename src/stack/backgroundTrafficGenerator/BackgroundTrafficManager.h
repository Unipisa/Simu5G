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

#ifndef BACKGROUNDTRAFFICMANAGER_H_
#define BACKGROUNDTRAFFICMANAGER_H_

#include "common/LteCommon.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class LteChannelModel;

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
    std::list<int> backloggedBgUes_[2];

    // references to the MAC and PHY layer of the e/gNodeB
    LteMacEnb* mac_;

    // carrier frequency for these bg UEs
    double carrierFrequency_;

    // tx power of the e/gNodeB
    double enbTxPower_;

    // reference to the channel model for the given carrier
    LteChannelModel* channelModel_;

    virtual void initialize(int stage);
    virtual int numInitStages() const  {return inet::INITSTAGE_LINK_LAYER+1; }


    // define functions for interactions with the NIC

  public:
    BackgroundTrafficManager();
    virtual ~BackgroundTrafficManager() {}

    // invoked by the UE's traffic generator when new data is backlogged
    void notifyBacklog(int index, Direction dir);

    // returns the CQI based on the given position and power
    Cqi computeCqi(Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0);

    // returns the begin (end) iterator of the vector of backlogged UEs
    std::list<int>::const_iterator getBackloggedUesBegin(Direction dir);
    std::list<int>::const_iterator getBackloggedUesEnd(Direction dir);

    // returns the buffer of the given UE for in the given direction
    unsigned int getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir);

    // returns the bytes per block of the given UE for in the given direction
    unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir);

    // update background UE's backlog and returns true if the buffer is empty
    unsigned int consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir);
};

#endif
