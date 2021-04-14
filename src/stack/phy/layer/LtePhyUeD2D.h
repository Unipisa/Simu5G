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

#include "stack/phy/layer/LtePhyUe.h"

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
    std::vector<LteAirFrame*> d2dReceivedFrames_; // airframes received in the current TTI. Only one will be decoded
    omnetpp::cMessage* d2dDecodingTimer_;                  // timer for triggering decoding at the end of the TTI. Started
                                                  // when the first airframe is received
    void storeAirFrame(LteAirFrame* newFrame);
    LteAirFrame* extractAirFrame();
    void decodeAirFrame(LteAirFrame* frame, UserControlInfo* lteInfo);
    // ---------------------------------------------------------------- //

    virtual void initialize(int stage);
    virtual void finish();
    virtual void handleAirFrame(omnetpp::cMessage* msg);
    virtual void handleUpperMessage(omnetpp::cMessage* msg);
    virtual void handleSelfMessage(omnetpp::cMessage *msg);

    virtual void triggerHandover();
    virtual void doHandover();

  public:
    LtePhyUeD2D();
    virtual ~LtePhyUeD2D();

    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);
    virtual double getTxPwr(Direction dir = UNKNOWN_DIRECTION)
    {
        if (dir == D2D)
            return d2dTxPower_;
        return txPower_;
    }
};

#endif  /* _LTE_AIRPHYUED2D_H_ */
