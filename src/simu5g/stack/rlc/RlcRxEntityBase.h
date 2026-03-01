#ifndef _RLC_RX_ENTITY_BASE_H_
#define _RLC_RX_ENTITY_BASE_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>

#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

/**
 * @class RlcRxEntityBase
 * @brief Abstract base class for all RLC RX entities (UM, TM, AM).
 *
 * Defines the minimal interface that the RlcLowerMux uses to manage
 * RX entities regardless of their RLC mode.
 */
class RlcRxEntityBase : public omnetpp::cSimpleModule
{
  protected:
    FlowControlInfo *flowControlInfo_ = nullptr;

    void handleMessage(omnetpp::cMessage *msg) override;

  public:
    virtual void setFlowControlInfo(FlowControlInfo *info);
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} // namespace simu5g

#endif
