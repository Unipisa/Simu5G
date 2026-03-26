//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AIRPHYENB_H_
#define _LTE_AIRPHYENB_H_

#include "simu5g/stack/phy/LtePhyBase.h"

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

    int randomChannelIndex_;

    void initialize(int stage) override;

    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;
    bool handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    void handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual void requestFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, inet::Packet *pkt);
    // Feedback computation for PisaPhy
    LteFeedbackComputation *getFeedbackComputationFromName(std::string name, ParameterMap& params);
    void initializeFeedbackComputation();

    virtual void emitDistanceFromMaster() {}

  public:
    ~LtePhyEnb() override;

};

} //namespace

#endif /* _LTE_AIRPHYENB_H_ */
