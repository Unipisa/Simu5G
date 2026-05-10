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
    bool useTransparentNtn_ = false;
    cGate* ntnInGate_ = nullptr;
    cGate* ntnOutGate_ = nullptr;

    /** Broadcast message interval (equal to updatePos interval for mobility) */
    double bdcUpdateInterval_;

    /** Self-message to trigger broadcast message sending for handover purposes */
    cMessage *bdcStarter_ = nullptr;
    cMessage *csiRsStarter_ = nullptr;

    simtime_t csiRsPeriod_ = 0;
    simtime_t srsPeriod_ = 0;

    int randomChannelIndex_;
    inet::ModuleRefByPar<LteUlFeedbackGenerator> ulFbGen_;

    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    void handleSelfMessage(cMessage *msg) override;

    /**
     * Create a new LteAirFrame with the given name. If the frame should be sent via a transparent NTN node, create an NtnAirFrame object
     */
    LteAirFrame *createAirFrame(const char *name, const UserControlInfo& lteInfo) override;

    void handleAirFrame(cMessage *msg) override;
    void handleNtnAirFrame(cMessage *msg);
    void sendDecodedDataFrame(LteAirFrame *frame, UserControlInfo *lteInfo, bool result);
    void sendBroadcast(LteAirFrame *airFrame) override;
    void sendUnicast(LteAirFrame *airFrame) override;
    void sendNtn(LteAirFrame *airFrame);
    bool handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual void handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual void handleSrsReferenceSignal(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual LteAirFrame *createCsiReferenceSignalFrame(inet::GHz carrierFrequency);
    virtual void sendCsiReferenceSignalFrameToAttachedUes(LteAirFrame *frame);
    virtual void sendCsiReferenceSignalFrameToNtnAttachedUes(LteAirFrame *frame);
    void initializeFeedbackComputation();

    virtual void emitDistanceFromMaster() {}

  public:
    ~LtePhyEnb() override;
    CellInfo *getCellInfo() const { return cellInfo_.get(); }
    simtime_t getSrsPeriod() const { return srsPeriod_; }

};

} //namespace

#endif /* _LTE_AIRPHYENB_H_ */
