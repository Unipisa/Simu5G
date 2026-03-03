#include "simu5g/stack/pdcp/UpperMux.h"
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

        isNR_ = par("isNR").boolValue();
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
        // Packet from an RX entity — forward to upper layer
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

void UpperMux::unregisterTxEntity(DrbKey id)
{
    txEntities_.erase(id);
}

} // namespace simu5g
