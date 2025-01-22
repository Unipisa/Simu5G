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

#ifndef _LTE_AIRPHYUED2D_H_
#define _LTE_AIRPHYUED2D_H_

#include "stack/phy/LtePhyUe.h"

namespace simu5g {

using namespace omnetpp;

class LtePhyUeD2D : public LtePhyUe
{
  protected:

    // D2D Tx Power
    double d2dTxPower_;

    /*
     * Capture Effect for D2D Multicast communications
     */
    bool d2dMulticastEnableCaptureEffect_;
    double nearestDistance_;
    std::vector<double> bestRsrpVector_;
    double bestRsrpMean_;
    std::vector<LteAirFrame *> d2dReceivedFrames_; // airframes received in the current TTI. Only one will be decoded
    cMessage *d2dDecodingTimer_ = nullptr;    // timer for triggering decoding at the end of the TTI. Started
    // when the first airframe is received
    void storeAirFrame(LteAirFrame *newFrame);
    LteAirFrame *extractAirFrame();
    void decodeAirFrame(LteAirFrame *frame, UserControlInfo *lteInfo);
    // ---------------------------------------------------------------- //

    void initialize(int stage) override;
    void finish() override;
    void handleAirFrame(cMessage *msg) override;
    void handleUpperMessage(cMessage *msg) override;
    void handleSelfMessage(cMessage *msg) override;

    void triggerHandover() override;
    void doHandover() override;

  public:

    void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req) override;
    double getTxPwr(Direction dir = UNKNOWN_DIRECTION) override
    {
        if (dir == D2D)
            return d2dTxPower_;
        return txPower_;
    }

};

} //namespace

#endif /* _LTE_AIRPHYUED2D_H_ */

