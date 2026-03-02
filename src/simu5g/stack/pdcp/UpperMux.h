#ifndef _UPPER_MUX_H_
#define _UPPER_MUX_H_

#include <map>
#include <set>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"

namespace simu5g {

using namespace omnetpp;

class LowerMux;
class DcMux;

/**
 * @class UpperMux
 * @brief Upper-layer PDCP packet dispatcher.
 *
 * Dispatches upper-layer packets to the correct TX entity by DrbKey,
 * and collects RX entity output to forward to the upper layer.
 */
class UpperMux : public cSimpleModule
{
  protected:
    LowerMux *lowerMux_ = nullptr;
    DcMux *dcMux_ = nullptr;
    bool isNR_ = false;

    cGate *upperLayerInGate_ = nullptr;
    cGate *upperLayerOutGate_ = nullptr;

    typedef std::map<DrbKey, PdcpTxEntityBase *> PdcpTxEntities;
    PdcpTxEntities txEntities_;

  public:
    PdcpTxEntityBase *lookupTxEntity(DrbKey id);
    void registerTxEntity(DrbKey id, PdcpTxEntityBase *txEnt);
    void deleteTxEntities(MacNodeId nodeId);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void fromDataPort(cPacket *pkt);
};

} // namespace simu5g

#endif
