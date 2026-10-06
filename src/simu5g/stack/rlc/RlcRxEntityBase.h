//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _RLC_RX_ENTITY_BASE_H_
#define _RLC_RX_ENTITY_BASE_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>

#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

/**
 * @brief Abstract base class for all RLC RX entities (UM, TM, AM).
 *
 * Defines the minimal interface that the RlcMux uses to manage
 * RX entities regardless of their RLC mode.
 */
class RlcRxEntityBase : public omnetpp::cSimpleModule
{
  protected:
    FlowControlInfo *flowControlInfo_ = nullptr;

    void handleMessage(omnetpp::cMessage *msg) override;

    ~RlcRxEntityBase() override { delete flowControlInfo_; }

  public:
    virtual void setFlowControlInfo(FlowControlInfo *info);
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} // namespace simu5g

#endif
