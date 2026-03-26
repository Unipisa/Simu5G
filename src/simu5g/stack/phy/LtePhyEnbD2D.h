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

#ifndef _LTE_AIRPHYENBD2D_H_
#define _LTE_AIRPHYENBD2D_H_

#include "simu5g/stack/phy/LtePhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class LtePhyEnbD2D : public LtePhyEnb
{

    bool enableD2DCqiReporting_;

  protected:

    void initialize(int stage) override;
    void requestFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, inet::Packet *pkt) override;
    void handleAirFrame(cMessage *msg) override;
};

} //namespace

#endif /* _LTE_AIRPHYENBD2D_H_ */
