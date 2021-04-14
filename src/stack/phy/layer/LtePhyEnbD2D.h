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

#ifndef _LTE_AIRPHYENBD2D_H_
#define _LTE_AIRPHYENBD2D_H_

#include "stack/phy/layer/LtePhyEnb.h"

class LtePhyEnbD2D : public LtePhyEnb
{
    friend class DasFilter;

    bool enableD2DCqiReporting_;

  protected:

    virtual void initialize(int stage) override;
    virtual void requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, inet::Packet * pkt) override;
    virtual void handleAirFrame(omnetpp::cMessage* msg) override;

  public:
    virtual ~LtePhyEnbD2D();

};

#endif  /* _LTE_AIRPHYENBD2D_H_ */
