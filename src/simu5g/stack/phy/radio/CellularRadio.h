//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_RADIO_CELLULARRADIO_H_
#define STACK_PHY_RADIO_CELLULARRADIO_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/phy/medium/CellularRadioMedium.h"

namespace simu5g {

using namespace omnetpp;

class AirFrame;
class IRadioEndpoint;

/**
 * See the NED documentation of CellularRadio. A received frame is held until
 * the end of its transmission (it is rescheduled as a self-message) and then
 * passed up unchanged; a frame from the PHY carries a RadioTransmissionRequest
 * that says where to send it.
 */
class CellularRadio : public cSimpleModule
{
  protected:
    int radioInGateId_ = -1;
    int upperLayerInGateId_ = -1;
    int upperLayerOutGateId_ = -1;
    inet::ModuleRefByPar<CellularRadioMedium> radioMedium_;
    IRadioEndpoint *endpoint_ = nullptr; // the PHY the radio sends and receives frames for

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    virtual void transmit(AirFrame *frame);

  public:
    /** Sets the PHY the radio sends and receives frames for, whose position is the radio's. */
    virtual void setEndpoint(IRadioEndpoint *endpoint) { endpoint_ = endpoint; }
};

} // namespace simu5g

#endif
