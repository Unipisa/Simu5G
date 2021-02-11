//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef TRAFFICGENERATORBASE_H_
#define TRAFFICGENERATORBASE_H_

#include "common/LteCommon.h"
#include "inet/mobility/contract/IMobility.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"

using namespace omnetpp;

class BackgroundTrafficManager;

//
// TrafficGeneratorBase
//
class TrafficGeneratorBase : public cSimpleModule, public cListener
{
  protected:

    // the physical position of the UE (derived from display string or from mobility models)
    inet::Coord pos_;

    // current DL and UL backlog
    unsigned int bufferedBytes_[2];

    // self messages for DL and UL
    cMessage* selfSource_[2];

    // starting time for DL and UL traffic
    simtime_t startTime_[2];

    // total length of above-the-MAC-layer headers
    int headerLen_;

    // index of the bg UE within the vector of bg UEs
    int bgUeIndex_;

    // reference to the traffic manager
    BackgroundTrafficManager* bgTrafficManager_;

    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage* msg) override;

    // virtual functions that implement the generation of
    // traffic according to some distribution
    virtual int generateTraffic(Direction dir);
    virtual simtime_t getNextGenerationTime(Direction dir);

  public:
    TrafficGeneratorBase();
    virtual ~TrafficGeneratorBase();

    // This module is subscribed to position changes.
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *) override;

    // consume bytes from the queue
    void consumeBytes(int bytes, Direction dir);

};

#endif
