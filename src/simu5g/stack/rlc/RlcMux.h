#ifndef _RLC_MUX_H_
#define _RLC_MUX_H_

#include <map>
#include <set>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/mec/utils/MecCommon.h"
#include "simu5g/stack/rlc/RlcRxEntityBase.h"
#include "simu5g/stack/rlc/um/UmRxEntity.h"

namespace simu5g {

using namespace omnetpp;

class BearerManagement;

/**
 * @class RlcMux
 * @brief Lower-layer RLC packet dispatcher.
 *
 * Owns the RX entity map. Creates RX entities with gate wiring
 * to itself (in gate) and to the UpperMux (out gate).
 */
class RlcMux : public cSimpleModule
{
  protected:
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;

    BearerManagement *bearerManagement_ = nullptr;

    bool hasD2DSupport_ = false;

    cGate *macInGate_ = nullptr;
    cGate *macOutGate_ = nullptr;

    typedef std::map<DrbKey, RlcRxEntityBase *> RxEntities;
    RxEntities rxEntities_;

    typedef std::map<MacNodeId, Throughput> ULThroughputPerUE;
    ULThroughputPerUE ulThroughput_;

  public:
    RlcRxEntityBase *lookupRxBuffer(DrbKey id);
    void registerRxBuffer(DrbKey id, RlcRxEntityBase *rxEnt);
    void unregisterRxBuffer(DrbKey id);
    void activeUeUL(std::set<MacNodeId> *ueSet);

    void addUeThroughput(MacNodeId nodeId, Throughput throughput);
    double getUeThroughput(MacNodeId nodeId);
    void resetThroughputStats(MacNodeId nodeId);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void fromMacLayer(cPacket *pkt);
};

} // namespace simu5g

#endif
