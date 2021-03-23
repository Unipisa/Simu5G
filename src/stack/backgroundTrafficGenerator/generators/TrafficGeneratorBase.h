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

    // reference to the traffic manager
    BackgroundTrafficManager* bgTrafficManager_;

    // index of the bg UE within the vector of bg UEs
    int bgUeIndex_;

    // self messages for DL and UL
    cMessage* selfSource_[2];

    // starting time for DL and UL traffic
    simtime_t startTime_[2];

    bool trafficEnabled_[2];

    // total length of above-the-MAC-layer headers
    unsigned int headerLen_;

    // tx power of the bg UE
    double txPower_;

    // if true, the CQI of the bg UE is affected by external interference
    bool enableInterference_;

    // CQI reporting period
    simtime_t fbPeriod_;

    // message for scheduling CQI reporting
    cMessage* fbSource_;

    /*
     * STATUS
     */

    // current DL and UL backlog
    unsigned int bufferedBytes_[2];

    // the physical position of the UE (derived from display string or from mobility models)
    inet::Coord pos_;

    // flag that signals when new SNR and CQI must be computed
    bool positionUpdated_;

    // current CQI based on the last position update
    Cqi cqi_[2];

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override {return inet::INITSTAGE_SINGLE_MOBILITY+1; }
    virtual void handleMessage(cMessage* msg) override;

    // get new values for sinr and cqi
    void updateMeasurements();

    // virtual functions that implement the generation of
    // traffic according to some distribution
    virtual unsigned int generateTraffic(Direction dir);
    virtual simtime_t getNextGenerationTime(Direction dir);

  public:
    TrafficGeneratorBase();
    virtual ~TrafficGeneratorBase();

    // returns the tx power of this bg UE
    virtual double getTxPwr() { return txPower_; }

    // returns the position of this bg UE
    virtual inet::Coord getCoord() { return pos_; }

    // This module is subscribed to position changes.
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *) override;

    // returns the number of buffered bytes for the given direction
    unsigned int getBufferLength(Direction dir);

    // returns the CQI for the given direction
    Cqi getCqi(Direction dir);

    // consume bytes from the queue and returns the updated buffer length
    unsigned int consumeBytes(int bytes, Direction dir);
};

#endif
