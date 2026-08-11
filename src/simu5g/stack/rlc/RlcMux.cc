//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/stack/rlc/RlcRxEntityBase.h"
#include "simu5g/stack/rlc/um/RlcUmTxEntityBase.h"
#include "simu5g/stack/rlc/um/RlcUmRxEntityBase.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include <inet/networklayer/common/NetworkInterface.h>
#include "simu5g/stack/rrc/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"

namespace simu5g {

Define_Module(RlcMux);

simsignal_t RlcMux::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t RlcMux::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");

void RlcMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        macInGate_ = gate("macIn");
        macOutGate_ = gate("macOut");

        bearerManagement_ = inet::getModuleFromPar<BearerManagement>(par("bearerManagementModule"), this);

        hasD2DSupport_ = inet::getContainingNicModule(this)->par("d2dCapable").boolValue();

        WATCH_MAP(rxGateIndices_);
    }
}

void RlcMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == macInGate_) {
        fromMacLayer(check_and_cast<cPacket *>(msg));
    }
    else if (incoming->isName("fromTxEntity")) {
        // Packet from a TX entity — forward to MAC
        emit(sentPacketToLowerLayerSignal_, msg);
        send(msg, macOutGate_);
    }
    else {
        throw cRuntimeError("RlcMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void RlcMux::fromMacLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    EV << "RlcMux::fromMacLayer - Received packet " << pkt->getName() << " from lower layer\n";
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    auto chunk = pkt->peekAtFront<inet::Chunk>();

    ASSERT(pkt->findTag<PdcpTrackingTag>() == nullptr);

    // D2D: handle mode switch notification
    if (hasD2DSupport_ && inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

        if (switchPkt->getTxSide()) {
            // get the corresponding Tx buffer & call handler
            DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
            RlcTxEntityBase *txbuf = bearerManagement_->lookupRlcTxBuffer(id);
            if (txbuf == nullptr)
                txbuf = bearerManagement_->createRlcTxBuffer(id, lteInfo.get());
            RlcUmTxEntityBase *umTxbuf = check_and_cast<RlcUmTxEntityBase *>(txbuf);
            umTxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
        else { // rx side
            DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
            auto it = rxGateIndices_.find(id);
            RlcRxEntityBase *rxbuf = it != rxGateIndices_.end()
                    ? check_and_cast<RlcRxEntityBase *>(gate("toRxEntity", it->second)->getPathEndGate()->getOwnerModule())
                    : bearerManagement_->createRlcRxBuffer(id, lteInfo.get());
            RlcUmRxEntityBase *umRxbuf = check_and_cast<RlcUmRxEntityBase *>(rxbuf);
            umRxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
        return;
    }

    if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr) {
        // MAC SDU request — dispatch to TX entity via macToTxEntity gate
        DrbKey id = ctrlInfoToTxDrbKey(lteInfo.get());
        RlcTxEntityBase *txbuf = bearerManagement_->lookupRlcTxBuffer(id);
        if (txbuf == nullptr) {
            // The bearer was torn down (radio link failure) after the MAC issued this
            // request. There is no entity left to serve it, so discard it rather than
            // dereferencing nothing, as the MAC does for data belonging to deleted
            // connections.
            EV << "RlcMux::fromMacLayer - no TX entity for " << id
               << " (torn down); dropping MAC SDU request\n";
            delete pkt;
            return;
        }

        send(pkt, txbuf->gate("macIn")->getPathStartGate());  // path start = our macToTxEntity gate (crosses the RlcEntity compound boundary)
    }
    else {
        // RLC PDU — dispatch to RX entity via toRxEntity gate
        emit(receivedPacketFromLowerLayerSignal_, pkt);

        DrbKey id = ctrlInfoToRxDrbKey(lteInfo.get());
        auto it = rxGateIndices_.find(id);
        if (it == rxGateIndices_.end()) {
            // Bearers are established duplex, so both sides normally exist -- but a radio
            // link failure unregisters them while PDUs are still in flight, which over a
            // satellite link means for a further round-trip time. Dereferencing the end
            // iterator here is what made such a run segfault in a release build, where the
            // assertion this replaces was compiled out.
            EV << "RlcMux::fromMacLayer - no RX entity for " << id
               << " (torn down); dropping PDU " << pkt->getName() << "\n";
            delete pkt;
            return;
        }

        EV << "RlcMux::fromMacLayer - Enqueue packet " << pkt->getName() << " into RX entity\n";
        send(pkt, "toRxEntity", it->second);
    }
}

void RlcMux::registerRxEntity(DrbKey id, int gateIndex)
{
    if (rxGateIndices_.find(id) != rxGateIndices_.end())
        throw cRuntimeError("RLC RX entity for %s already registered", id.str().c_str());
    ASSERT(gate("toRxEntity", gateIndex)->isConnectedOutside());
    rxGateIndices_[id] = gateIndex;
    EV << "RlcMux::registerRxEntity - Registered RX entity for " << id << " on toRxEntity gate " << gateIndex << "\n";
}

void RlcMux::unregisterRxEntity(DrbKey id)
{
    rxGateIndices_.erase(id);
}

void RlcMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, gateIndex] : rxGateIndices_) {
        MacNodeId nodeId = id.getNodeId();
        // path end crosses the RlcEntity compound boundary, so the owner is its rx submodule
        RlcUmRxEntityBase *umEnt = dynamic_cast<RlcUmRxEntityBase *>(gate("toRxEntity", gateIndex)->getPathEndGate()->getOwnerModule());
        if (umEnt != nullptr && (ueSet->find(nodeId) == ueSet->end()) && !umEnt->isEmpty())
            ueSet->insert(nodeId);
    }
}

void RlcMux::addUeThroughput(MacNodeId nodeId, Throughput throughput)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount += throughput.pktSizeCount;
    nodeUlThroughput.time += throughput.time;
}

double RlcMux::getUeThroughput(MacNodeId nodeId)
{
    ULThroughputPerUE::iterator it = ulThroughput_.find(nodeId);
    return it == ulThroughput_.end() ? 0 : it->second.pktSizeCount / it->second.time.dbl();
}

void RlcMux::resetThroughputStats(MacNodeId nodeId)
{
    auto& nodeUlThroughput = ulThroughput_[nodeId];
    nodeUlThroughput.pktSizeCount = 0;
    nodeUlThroughput.time = 0;
}

} // namespace simu5g
