#ifndef _RLC_UPPER_MUX_H_
#define _RLC_UPPER_MUX_H_

#include <map>
#include <set>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/utils/utils.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"

namespace simu5g {

using namespace omnetpp;

class RlcLowerMux;
class UmTxEntity;

/**
 * @class RlcUpperMux
 * @brief Upper-layer RLC packet dispatcher.
 *
 * Owns the TX entity map. Creates TX entities with gate wiring
 * to itself (in gate) and to the LowerMux (out, macIn gates).
 */
class RlcUpperMux : public cSimpleModule
{
  protected:
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;

    inet::ModuleRefByPar<Binder> binder_;

    RlcLowerMux *lowerMux_ = nullptr;

    bool hasD2DSupport_ = false;
    RanNodeType nodeType_;
    cModuleType *txEntityModuleType_ = nullptr;

    cGate *upperLayerInGate_ = nullptr;
    cGate *upperLayerOutGate_ = nullptr;

    typedef std::map<DrbKey, UmTxEntity *> UmTxEntities;
    UmTxEntities txEntities_;

    // D2D: per-peer TX entity tracking
    std::map<MacNodeId, std::set<UmTxEntity *, simu5g::utils::cModule_LessId>> perPeerTxEntities_;

  public:
    UmTxEntity *lookupTxBuffer(DrbKey id);
    UmTxEntity *createTxBuffer(DrbKey id, FlowControlInfo *lteInfo);
    void deleteTxEntities(MacNodeId nodeId);

    void resumeDownstreamInPackets(MacNodeId peerId);
    bool isEmptyingTxBuffer(MacNodeId peerId);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void fromUpperLayer(cPacket *pkt);
};

} // namespace simu5g

#endif
