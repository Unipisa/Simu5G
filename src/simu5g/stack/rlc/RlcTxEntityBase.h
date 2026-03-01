#ifndef _RLC_TX_ENTITY_BASE_H_
#define _RLC_TX_ENTITY_BASE_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>

#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

/**
 * @class RlcTxEntityBase
 * @brief Abstract base class for all RLC TX entities (UM, TM, AM).
 *
 * Defines the minimal interface that the RlcUpperMux uses to manage
 * TX entities regardless of their RLC mode.
 */
class RlcTxEntityBase : public omnetpp::cSimpleModule
{
  protected:
    FlowControlInfo *flowControlInfo_ = nullptr;

    void handleMessage(omnetpp::cMessage *msg) override;

    ~RlcTxEntityBase() override { delete flowControlInfo_; }

  public:
    virtual void setFlowControlInfo(FlowControlInfo *info);
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} // namespace simu5g

#endif
