#ifndef _D2D_MODE_CONTROLLER_H_
#define _D2D_MODE_CONTROLLER_H_

#include <map>
#include <set>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class UmTxEntity;

class D2DModeController : public cSimpleModule
{
  protected:
    typedef std::map<MacNodeId, std::set<UmTxEntity *>> PerPeerTxEntities;
    PerPeerTxEntities perPeerTxEntities_;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    void registerD2DPeerTxEntity(MacNodeId peerId, UmTxEntity *umTxEnt);
    void resumeDownstreamInPackets(MacNodeId peerId);
    bool isEmptyingTxBuffer(MacNodeId peerId);
};

} // namespace simu5g

#endif
