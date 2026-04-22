#ifndef __SIMU5G_NTNPHYBASE_H_
#define __SIMU5G_NTNPHYBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

class NtnPhyBase : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    MacNodeId nodeId_ = NODEID_NONE;
    RanNodeType nodeType_ = UNKNOWN_NODE_TYPE;
    bool isFeederLink_ = false;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(omnetpp::cMessage *msg) override;

    omnetpp::cGate *resolvePeerGate() const;
    omnetpp::cModule *resolvePeerNode() const;
    int getReceiverGateIndex(const omnetpp::cModule *receiver, bool isNr) const;
};

} // namespace simu5g

#endif
