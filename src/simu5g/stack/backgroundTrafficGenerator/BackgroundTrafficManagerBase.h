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

#ifndef BACKGROUNDTRAFFICMANAGERBASE_H_
#define BACKGROUNDTRAFFICMANAGERBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "common/blerCurves/PhyPisaData.h"
#include "stack/backgroundTrafficGenerator/IBackgroundTrafficManager.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/phy/LtePhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class LteChannelModel;

//
// BackgroundTrafficManagerBase
//
class BackgroundTrafficManagerBase : public cSimpleModule, public IBackgroundTrafficManager
{
  protected:

    // number of background UEs
    int numBgUEs_;

    // reference to all the background UEs
    std::vector<TrafficGeneratorBase *> bgUe_;

    // indexes of the backlogged bg UEs
    std::list<int> backloggedBgUes_[2];

    // indexes of the backlogged bg UEs for retransmission
    std::list<int> backloggedRtxBgUes_[2];

    // indexes of the backlogged bg UEs waiting for RAC+BSR handshake
    std::list<int> waitingForRac_;

    // reference to binder module
    inet::ModuleRefByPar<Binder> binder_;

    //pointer to pisadata
    PhyPisaData *phyPisaData_ = nullptr;

    /// TTI for this node
    double ttiPeriod_;

    // carrier frequency for these bg UEs
    double carrierFrequency_;

    // tx power of the e/gNodeB
    double bsTxPower_;

    // position of the e/gNodeB
    inet::Coord bsCoord_;

    /**************************************
     * Support to average CQI computation *
     * ***********************************/

    // average load for average interference computation
    double avgCellLoad_;
    std::vector<double> avgUeLoad_;

    // do preliminary actions to enable average cqi computation
    // this is called by the initialize() in the INITSTAGE_PHYSICAL_LAYER
    virtual void initializeAvgInterferenceComputation();

    /*************************************/

    void initialize(int stage) override;
    int numInitStages() const override { return inet::INITSTAGE_LAST; }
    void handleMessage(cMessage *msg) override;

    static const double nrCqiTable[16];
    static double getCqiFromTable(double snr);

    virtual double getTtiPeriod() = 0;
    virtual bool isSetBgTrafficManagerInfoInit() = 0;
    virtual std::vector<double> getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower) = 0;

    // define functions for interactions with the NIC

  public:

    // set carrier frequency
    void setCarrierFrequency(double carrierFrequency) override { carrierFrequency_ = carrierFrequency; }

    // get the tx power of the BS
    double getBsTxPower() override { return bsTxPower_; }

    // get the position of the BS
    inet::Coord getBsCoord() override { return bsCoord_; }

    // invoked by the UE's traffic generator when new data is backlogged
    void notifyBacklog(int index, Direction dir, bool rtx = false) override;

    // returns the CQI based on the given position and power
    Cqi computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0) override;

    // returns the CQI based on the given sinr
    Cqi computeCqiFromSinr(double sinr) override;

    // returns the pointer to the traffic generator of the given background UE
    TrafficGeneratorBase *getTrafficGenerator(MacNodeId bgUeId) override;

    // returns the begin (end) iterator of the vector of UEs
    std::vector<TrafficGeneratorBase *>::const_iterator getBgUesBegin() override;
    std::vector<TrafficGeneratorBase *>::const_iterator getBgUesEnd() override;

    // returns the begin (end) iterator of the vector of backlogged UEs
    std::list<int>::const_iterator getBackloggedUesBegin(Direction dir, bool rtx = false) override;
    std::list<int>::const_iterator getBackloggedUesEnd(Direction dir, bool rtx = false) override;

    // returns the begin (end) iterator of the vector of backlogged UEs that are waiting for RAC handshake to finish
    std::list<int>::const_iterator getWaitingForRacUesBegin() override;
    std::list<int>::const_iterator getWaitingForRacUesEnd() override;

    // returns the buffer of the given UE for in the given direction
    unsigned int getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir, bool rtx = false) override;

    // signal that the RAC for the given UE has been handled
    void racHandled(MacNodeId bgUeId) override;

    // update background UE's backlog and returns true if the buffer is empty
    unsigned int consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir, bool rtx = false) override;
};

} //namespace

#endif

