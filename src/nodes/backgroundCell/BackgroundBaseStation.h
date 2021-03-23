//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __BACKGROUNDBASESTATION_H_
#define __BACKGROUNDBASESTATION_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "nodes/binder/LteBinder.h"
#include "nodes/backgroundCell/BackgroundCellChannelModel.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"
#include "stack/mac/scheduler/LteScheduler.h"  // for SortedDesc

typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
typedef std::priority_queue<ScoreDesc> ScoreList;

class BackgroundBaseStation : public omnetpp::cSimpleModule, public cListener
{
    // base station coordinates
    inet::Coord pos_;

    // id among all the background cells
    int id_;

    // tx power
    double txPower_;

    // tx direction
    TxDirectionType txDirection_;

    // tx angle
    double txAngle_;

    // if true, this is a NR base station
    // if false, this is a LTE base station
    bool isNr_;

    // numerology
    unsigned int numerologyIndex_;

    // carrier frequency
    double carrierFrequency_;

    // number of logical bands
    unsigned int numBands_;

    // reference to the binder
    LteBinder* binder_;

    // reference to the background traffic manager - one per carrier
    BackgroundTrafficManager* bgTrafficManager_;

    // reference to the channel model for this background base station
    BackgroundCellChannelModel* bgChannelModel_;

    // TTI self message
    omnetpp::cMessage* ttiTick_;
    double ttiPeriod_;


    /*** ALLOCATION MANAGEMENT ***/

    // Current and previous band occupation status (both for UL and DL). Used for interference computation
    BandStatus bandStatus_[2];
    BandStatus prevBandStatus_[2];

    // for the UL, we need to store which UE uses which block
    std::vector<int> ulBandAllocation_;
    std::vector<int> ulPrevBandAllocation_;


    // update the band status. Called at each TTI (not used for FULL_ALLOC)
    virtual void updateAllocation(Direction dir);
    // move the current status in the prevBandStatus structure and reset the former
    void resetAllocation(Direction dir);
    /*****************************/

  protected:
    virtual void initialize(int stage);
    virtual int numInitStages() const { return inet::INITSTAGE_LOCAL+2; }
    virtual void handleMessage(omnetpp::cMessage *msg);

  public:

    // This module is subscribed to position changes.
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *) override;

    BackgroundCellChannelModel* getChannelModel() { return bgChannelModel_; }

    const inet::Coord getPosition() { return pos_; }

    int getId() { return id_; }

    double getTtiPeriod() {return ttiPeriod_; }

    double getTxPower() { return txPower_; }

    TxDirectionType getTxDirection() { return txDirection_; }

    double getTxAngle() { return txAngle_; }

    bool isNr() { return isNr_; }

    unsigned int getNumBands() { return numBands_; }

    int getBandStatus(int band, Direction dir) { return bandStatus_[dir].at(band); }
    int getPrevBandStatus(int band, Direction dir) { return prevBandStatus_[dir].at(band); }

    TrafficGeneratorBase* getBandInterferingUe(int band);
    TrafficGeneratorBase* getPrevBandInterferingUe(int band);
};

#endif
