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

    // indexes of the backlogged bg UEs waiting for RAC+BSR handshake
    std::list<int> waitingForRac_;

    // references to the MAC and PHY layer of the e/gNodeB
    LteMacEnb* mac_;

    /// TTI for this node
    double ttiPeriod_;

    // carrier frequency for these bg UEs
    double carrierFrequency_;

    // tx power of the e/gNodeB
    double enbTxPower_;

    // reference to the channel model for the given carrier
    LteChannelModel* channelModel_;

    virtual void initialize(int stage);
    virtual int numInitStages() const  {return inet::INITSTAGE_LINK_LAYER+1; }
    virtual void handleMessage(cMessage* msg);

    // define functions for interactions with the NIC

  public:
    BackgroundTrafficManager();
    virtual ~BackgroundTrafficManager() {}

    // set carrier frequency
    void setCarrierFrequency(double carrierFrequency) { carrierFrequency_ = carrierFrequency; }

    // invoked by the UE's traffic generator when new data is backlogged
    virtual void notifyBacklog(int index, Direction dir);

    // returns the CQI based on the given position and power
    virtual Cqi computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0);

    // returns the pointer to the traffic generator of the given background UE
    TrafficGeneratorBase* getTrafficGenerator(MacNodeId bgUeId);

    // returns the begin (end) iterator of the vector of backlogged UEs
    std::list<int>::const_iterator getBackloggedUesBegin(Direction dir);
    std::list<int>::const_iterator getBackloggedUesEnd(Direction dir);

    // returns the begin (end) iterator of the vector of backlogged UEs hat are waiting for RAC handshake to finish
    std::list<int>::const_iterator getWaitingForRacUesBegin();
    std::list<int>::const_iterator getWaitingForRacUesEnd();

    // returns the buffer of the given UE for in the given direction
    virtual unsigned int getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir);

    // returns the bytes per block of the given UE for in the given direction
    virtual  unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir);

    // signal that the RAC for the given UE has been handled
    virtual void racHandled(MacNodeId bgUeId);

    // update background UE's backlog and returns true if the buffer is empty
    virtual unsigned int consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir);
};

#endif
