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

#ifndef __SIMU5G_NTNPHYGNB_H_
#define __SIMU5G_NTNPHYGNB_H_

#include "simu5g/stack/phy/LtePhyEnb.h"

namespace simu5g {

class NtnPhyGnb : public LtePhyEnb
{
  protected:
    cGate *ntnInGate_ = nullptr;
    cGate *ntnOutGate_ = nullptr;

    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    LteAirFrame *createAirFrame(const char *name) override;
    LteAirFrame *createCsiReferenceSignalFrame(inet::GHz carrierFrequency) override;
    void sendBroadcast(LteAirFrame *airFrame) override;
    void sendUnicast(LteAirFrame *airFrame) override;
    void sendCsiReferenceSignalFrameToAttachedUes(LteAirFrame *frame) override;

    void handleNtnAirFrame(cMessage *msg);
    void sendNtn(LteAirFrame *airFrame);
};

} // namespace simu5g

#endif
