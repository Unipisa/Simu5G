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

#ifndef _PHYENB_H_
#define _PHYENB_H_

#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/phy/feedback/LteFeedbackComputationRealistic.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"

namespace simu5g {

using namespace omnetpp;

class LteFeedbackPkt;

class PhyEnb : public PhyBase
{

  protected:
    /** Broadcast message interval (equal to updatePos interval for mobility) */
    double beaconInterval_;

    /** Self-message to trigger broadcast message sending for handover purposes */
    cMessage *beaconStarter_ = nullptr;

    /** Computes the feedback of the primary carrier from the received SINR */
    LteFeedbackComputation *lteFeedbackComputation_ = nullptr;

    void initialize(int stage) override;

    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;
    virtual bool handleControlPkt(UserControlInfo *lteinfo, AirFrame *frame);
    virtual void handleFeedbackPkt(UserControlInfo *lteinfo, AirFrame *frame);
    virtual void requestFeedback(UserControlInfo *lteinfo, AirFrame *frame, inet::Packet *pkt);

    /// appends additional per-link feedback to the feedback packet (default:
    /// none); called after the UL and DL vectors have been stored, with the
    /// control info still in its end-of-loop (DL) state
    virtual void appendExtraFeedback(inet::Ptr<LteFeedbackPkt>& header, UserControlInfo *lteinfo, AirFrame *frame, ChannelModelBase *channelModel) {}
    virtual void initializeFeedbackComputation();
    virtual AirFrame *createBeaconMessage();

    virtual void emitDistanceFromMaster() {}

  public:
    ~PhyEnb() override;

};

} //namespace

#endif /* _PHYENB_H_ */
