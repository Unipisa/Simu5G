#ifndef _LOWER_MUX_H_
#define _LOWER_MUX_H_

#include <map>
#include <set>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"

namespace simu5g {

using namespace omnetpp;

class UpperMux;

/**
 * @class LowerMux
 * @brief Lower-layer PDCP packet dispatcher.
 *
 * Dispatches RLC packets to the correct RX entity by DrbKey,
 * and collects TX entity output to route to the correct RLC gate.
 * Also handles DC manager input and D2D mode switch notifications.
 */
class LowerMux : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    MacNodeId nodeId_;

    bool isNR_ = false;
    bool hasD2DSupport_ = false;
    bool dualConnectivityEnabled_ = false;

    UpperMux *upperMux_ = nullptr;

    cModuleType *rxEntityModuleType_ = nullptr;
    cModuleType *bypassRxEntityModuleType_ = nullptr;
    cModuleType *bypassTxEntityModuleType_ = nullptr;

    cGate *rlcInGate_ = nullptr;
    cGate *rlcOutGate_ = nullptr;
    cGate *nrRlcOutGate_ = nullptr;

    typedef std::map<DrbKey, PdcpRxEntityBase *> PdcpRxEntities;
    typedef std::map<DrbKey, PdcpTxEntityBase *> PdcpBypassTxEntities;
    PdcpRxEntities rxEntities_;              // normal + bypass RX entities
    PdcpBypassTxEntities bypassTxEntities_;  // bypass TX entities only

  public:
    PdcpRxEntityBase *lookupRxEntity(DrbKey id);
    PdcpRxEntityBase *createRxEntity(DrbKey id);
    PdcpTxEntityBase *createBypassTxEntity(DrbKey id);
    PdcpRxEntityBase *createBypassRxEntity(DrbKey id);
    PdcpTxEntityBase *lookupBypassTxEntity(DrbKey id);
    void deleteRxAndBypassEntities(MacNodeId nodeId);
    void activeUeUL(std::set<MacNodeId> *ueSet);

    bool isDualConnectivityEnabled() { return dualConnectivityEnabled_; }

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void fromLowerLayer(cPacket *pkt);
    void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode);
    virtual void pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode);
};

} // namespace simu5g

#endif
