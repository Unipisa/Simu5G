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

#include "simu5g/stack/pdcp/PdcpMux.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(PdcpMux);

using namespace omnetpp;
using namespace inet;

void PdcpMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperLayerInGate_ = gate("upperLayerIn");
        upperLayerOutGate_ = gate("upperLayerOut");

        WATCH(txGateIndices_);
    }
}

void PdcpMux::handleMessage(cMessage *msg)
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
        throw cRuntimeError("PdcpMux: unexpected message from gate %s", incoming->getFullName());
    }
}

void PdcpMux::fromDataPort(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    verifyControlInfo(lteInfo.get());

    DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
    auto it = txGateIndices_.find(id);

    EV << "fromDataPort in " << getFullPath() << " event #" << getSimulation()->getEventNumber()
       << ": Processing packet " << pkt->getName() << " src=" << lteInfo->getSourceId() << " dest=" << lteInfo->getDestId()
       << " d2dGroup=" << lteInfo->getD2dGroupId() << " direction=" << dirToA((Direction)lteInfo->getDirection())
       << " ---> " << id << std::endl;

    // A missing entity must never be silently dropped -- ASSERT is compiled out in release
    // builds, so fail with a real throw. Ip2Nic/SDAP establish the connection for every
    // packet whose TX entity does not exist yet, so a miss here is a dispatch bug.
    if (it == txGateIndices_.end())
        throw cRuntimeError("PdcpMux::fromDataPort: no PDCP TX entity for %s -- the connection "
                            "should have been established by Ip2Nic or SDAP", id.str().c_str());

    send(pkt, "toTxEntity", it->second);
}

void PdcpMux::registerTxEntity(DrbKey id, int gateIndex)
{
    if (txGateIndices_.find(id) != txGateIndices_.end())
        throw cRuntimeError("PDCP TX entity for %s already registered", id.str().c_str());
    ASSERT(gate("toTxEntity", gateIndex)->isConnectedOutside());
    txGateIndices_[id] = gateIndex;
    EV << "PdcpMux::registerTxEntity - Registered TX entity for " << id << " on toTxEntity gate " << gateIndex << "\n";
}

void PdcpMux::unregisterTxEntity(DrbKey id)
{
    txGateIndices_.erase(id);
}

} // namespace simu5g
