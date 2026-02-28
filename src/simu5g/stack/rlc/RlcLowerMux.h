#ifndef _RLC_LOWER_MUX_H_
#define _RLC_LOWER_MUX_H_

#include <map>
#include <set>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rlc/um/UmRxEntity.h"

namespace simu5g {

using namespace omnetpp;

class RlcUpperMux;
class UmRxEntity;

/**
 * @class RlcLowerMux
 * @brief Lower-layer RLC packet dispatcher.
 *
 * Owns the RX entity map. Creates RX entities with gate wiring
 * to itself (in gate) and to the UpperMux (out gate).
 */
class RlcLowerMux : public cSimpleModule
{
  protected:
    static simsignal_t sentPacketToLowerLayerSignal_;

    RlcUpperMux *upperMux_ = nullptr;

    bool hasD2DSupport_ = false;
    RanNodeType nodeType_;
    cModuleType *rxEntityModuleType_ = nullptr;

    cGate *macInGate_ = nullptr;
    cGate *macOutGate_ = nullptr;
    cGate *toUmGate_ = nullptr;
    cGate *fromUmGate_ = nullptr;

    typedef std::map<DrbKey, UmRxEntity *> UmRxEntities;
    UmRxEntities rxEntities_;

  public:
    UmRxEntity *lookupRxBuffer(DrbKey id);
    UmRxEntity *createRxBuffer(DrbKey id, FlowControlInfo *lteInfo);
    void deleteRxEntities(MacNodeId nodeId);
    void activeUeUL(std::set<MacNodeId> *ueSet);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
