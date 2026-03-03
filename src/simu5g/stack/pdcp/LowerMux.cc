#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/pdcp/PdcpOutputRoutingTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(LowerMux);

using namespace omnetpp;
using namespace inet;

void LowerMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        rlcInGate_ = gate("rlcIn");
        rlcOutGate_ = gate("rlcOut");

        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        upperMux_ = check_and_cast<UpperMux *>(getParentModule()->getSubmodule("pdcpUpperMux"));
        dcMux_ = check_and_cast<DcMux *>(getParentModule()->getSubmodule("pdcpDcMux"));

        isNR_ = par("isNR").boolValue();
        hasD2DSupport_ = par("hasD2DSupport").boolValue();

        if (isNR_) {
            inet::NetworkInterface *nic = inet::getContainingNicModule(this);
            dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();

            if (getNodeTypeById(nodeId_) == UE) {
                nrRlcOutGate_ = gate("nrRlcOut");
            }
        }

        WATCH(nodeId_);
    }
}

void LowerMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LowerMux : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    // D2D mode switch notification
    if (hasD2DSupport_) {
        auto inet_pkt = check_and_cast<inet::Packet *>(pkt);
        auto chunk = inet_pkt->peekAtFront<Chunk>();
        if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
            EV << "LowerMux::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;
            auto switchPkt = inet_pkt->peekAtFront<D2DModeSwitchNotification>();
            pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
            delete pkt;
            return;
        }
    }

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == rlcInGate_ || incoming->isName("nrRlcIn")) {
        fromLowerLayer(pkt);
    }
    else if (incoming->isName("fromTxEntity")) {
        // Packet from a TX entity — route based on PdcpOutputRoutingTag
        auto inetPkt = check_and_cast<Packet *>(pkt);
        auto routeTag = inetPkt->removeTag<PdcpOutputRoutingTag>();
        switch (routeTag->getRoute()) {
            case PDCP_OUT_RLC:
                send(pkt, rlcOutGate_);
                break;
            case PDCP_OUT_NR_RLC:
                send(pkt, nrRlcOutGate_);
                break;
            default:
                throw cRuntimeError("LowerMux: unexpected route %d from TX entity", (int)routeTag->getRoute());
        }
    }
    else {
        throw cRuntimeError("LowerMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void LowerMux::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    // NrPdcpEnb: trim packet for NR gNBs
    if (isNR_ && getNodeTypeById(nodeId_) == NODEB) {
        pkt->trim();
    }

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());

    // UE with DC: both RLC legs must reach the same reordering entity
    if (isDualConnectivityEnabled() && getNodeTypeById(nodeId_) == UE
            && lteInfo->getMulticastGroupId() == NODEID_NONE) {
        id = DrbKey(binder_->getServingNode(nodeId_), lteInfo->getDrbId());
    }

    PdcpRxEntityBase *entity = lookupRxEntity(id);

    EV << "fromLowerLayer in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       <<  ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << (entity == nullptr ? " (NEW)" : " (existing)") << std::endl;

    ASSERT(entity != nullptr);

    send(pkt, entity->gate("in")->getPreviousGate());
}

void LowerMux::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LowerMux::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;
}

PdcpRxEntityBase *LowerMux::lookupRxEntity(DrbKey id)
{
    auto it = rxEntities_.find(id);
    return it != rxEntities_.end() ? it->second : nullptr;
}

void LowerMux::registerRxEntity(DrbKey id, PdcpRxEntityBase *rxEnt)
{
    if (rxEntities_.find(id) != rxEntities_.end())
        throw cRuntimeError("PDCP RX entity for %s already exists", id.str().c_str());
    rxEntities_[id] = rxEnt;
    EV << "LowerMux::registerRxEntity - Registered RxPdcpEntity for " << id << "\n";
}

void LowerMux::unregisterRxEntity(DrbKey id)
{
    rxEntities_.erase(id);
}


void LowerMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : rxEntities_) {
        MacNodeId nodeId = id.getNodeId();
        if (!(rxEntity->isEmpty()))
            ueSet->insert(nodeId);
    }
}

} // namespace simu5g
