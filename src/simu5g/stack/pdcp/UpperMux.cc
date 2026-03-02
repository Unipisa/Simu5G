#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/pdcp/PdcpOutputRoutingTag_m.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(UpperMux);

using namespace omnetpp;
using namespace inet;

void UpperMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");

        lowerMux_ = check_and_cast<LowerMux *>(getParentModule()->getSubmodule("lowerMux"));
        dcMux_ = check_and_cast<DcMux *>(getParentModule()->getSubmodule("dcMux"));
    }
}

void UpperMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    if (incoming == upperLayerInGate_) {
        fromDataPort(pkt);
    }
    else if (incoming->isName("fromRxEntity")) {
        // Packet from an RX entity — route to upper layer
        auto inetPkt = check_and_cast<Packet *>(pkt);
        auto routeTag = inetPkt->removeTag<PdcpOutputRoutingTag>();
        if (routeTag->getRoute() != PDCP_OUT_UPPER)
            throw cRuntimeError("UpperMux: unexpected route %d from RX entity", (int)routeTag->getRoute());
        send(pkt, upperLayerOutGate_);
    }
    else {
        throw cRuntimeError("UpperMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void UpperMux::fromDataPort(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
    PdcpTxEntityBase *entity = lookupTxEntity(id);

    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " multicast=" << lteInfo->getMulticastGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << std::endl;

    ASSERT2(entity != nullptr, "TX entity not found -- connection should have been established by Ip2Nic");

    send(pkt, entity->gate("in")->getPreviousGate());
}

PdcpTxEntityBase *UpperMux::lookupTxEntity(DrbKey id)
{
    auto it = txEntities_.find(id);
    return it != txEntities_.end() ? it->second : nullptr;
}

void UpperMux::registerTxEntity(DrbKey id, PdcpTxEntityBase *txEnt)
{
    if (txEntities_.find(id) != txEntities_.end())
        throw cRuntimeError("PDCP TX entity for %s already exists", id.str().c_str());
    txEntities_[id] = txEnt;
    EV << "UpperMux::registerTxEntity - Registered TxPdcpEntity for " << id << "\n";
}

void UpperMux::deleteTxEntities(MacNodeId nodeId)
{
    MacNodeId nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());
    bool isEnb = (getNodeTypeById(nodeId_) == NODEB);
    bool isNR = getParentModule()->par("isNR").boolValue();

    if (isEnb || isNR) {
        for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
            auto& [id, txEntity] = *tit;
            if (id.getNodeId() == nodeId) {
                txEntity->deleteModule();
                tit = txEntities_.erase(tit);
            }
            else {
                ++tit;
            }
        }
    }
    else {
        for (auto& [txId, txEntity] : txEntities_)
            txEntity->deleteModule();
        txEntities_.clear();
    }
}

} // namespace simu5g
