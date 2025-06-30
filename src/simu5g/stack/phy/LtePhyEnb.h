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

#include "stack/phy/LtePhyBase.h"

namespace simu5g {

using namespace omnetpp;

class DasFilter;
class LteFeedbackPkt;

class LtePhyEnb : public LtePhyBase
{
    friend class DasFilter;

  protected:
    /** Broadcast message interval (equal to updatePos interval for mobility) */
    double bdcUpdateInterval_;

    /** Self-message to trigger broadcast message sending for handover purposes */
    cMessage *bdcStarter_ = nullptr;

    /**
     * Pointer to the DAS Filter: used to call DAS functions
     * when receiving broadcasts and to retrieve physical
     * antenna properties on packet reception
     */
    DasFilter *das_ = nullptr;

    void initialize(int stage) override;

    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;
    bool handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    void handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame);
    virtual void requestFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, inet::Packet *pkt);
    /**
     * Getter for the DAS Filter
     */
    DasFilter *getDasFilter();
    // Feedback computation for PisaPhy
    LteFeedbackComputation *getFeedbackComputationFromName(std::string name, ParameterMap& params);
    void initializeFeedbackComputation();

    virtual void emitDistanceFromMaster() {}

  public:
    ~LtePhyEnb() override;

};

} //namespace

#endif /* _LTE_AIRPHYENB_H_ */

