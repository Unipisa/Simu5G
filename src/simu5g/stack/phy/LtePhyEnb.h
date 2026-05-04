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

#ifndef _LTE_AIRPHYENB_H_
#define _LTE_AIRPHYENB_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/phy/LtePhyBase.h"
#include "simu5g/stack/phy/feedback/LteUlFeedbackGenerator.h"

namespace simu5g {

using namespace omnetpp;

class LteFeedbackPkt;
class LtePhyEnb : public LtePhyBase
{

  protected:
    /** Broadcast message interval (equal to updatePos interval for mobility) */
    double bdcUpdateInterval_;

    /** Self-message to trigger broadcast message sending for handover purposes */
    cMessage *bdcStarter_ = nullptr;
    cMessage *csiRsStarter_ = nullptr;

    double csiRsPeriod_ = 0;
    bool useUeDlFeedbackComputation_ = false;

    int randomChannelIndex_;
    inet::ModuleRefByPar<LteUlFeedbackGenerator> ulFbGen_;

    void initialize(int stage) override;

    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;
    bool handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual void handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual LteAirFrame *createCsiReferenceSignal(inet::GHz carrierFrequency);
    virtual void sendCsiReferenceSignalToAttachedUes(LteAirFrame *frame);
    // Feedback computation for PisaPhy
    LteFeedbackComputation *getFeedbackComputationFromName(std::string name, ParameterMap& params);
    void initializeFeedbackComputation();

    virtual void emitDistanceFromMaster() {}

  public:
    ~LtePhyEnb() override;
    CellInfo *getCellInfo() const { return cellInfo_.get(); }

};

} //namespace

#endif /* _LTE_AIRPHYENB_H_ */
