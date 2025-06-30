//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/um/LteRlcUmD2D.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"

namespace simu5g {

Define_Module(LteRlcUmD2D);
using namespace omnetpp;

UmTxEntity *LteRlcUmD2D::getTxBuffer(inet::Ptr<FlowControlInfo> lteInfo)
{
    MacNodeId nodeId = NODEID_NONE;
    LogicalCid lcid = 0;
    if (lteInfo != nullptr) {
        nodeId = ctrlInfoToUeId(lteInfo);
        lcid = lteInfo->getLcid();
    }
    else
        throw cRuntimeError("LteRlcUmD2D::getTxBuffer - lteInfo is a NULL pointer. Aborting.");

    // Find TXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    EV << "LteRlcUmD2D::getTxBuffer for node [" << nodeId << "]" << " with lcid [" << lcid << "]" << " and cid [" << cid << "]" << endl;
    UmTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end()) {
        // Not found: create
        MacNodeId d2dPeer = NODEID_NONE;
        std::stringstream buf;

        buf << "UmTxEntity Lcid: " << lcid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.rlc.um.UmTxEntity");
        UmTxEntity *txEnt = check_and_cast<UmTxEntity *>(moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        txEntities_[cid] = txEnt;    // Add to tx_entities map

        if (lteInfo != nullptr) {
            // store control info for this flow
            txEnt->setFlowControlInfo(lteInfo.get());
            d2dPeer = lteInfo->getD2dRxPeerId();
        }

        EV << "LteRlcUmD2D : Added new UmTxEntity: " << txEnt->getId() <<
            " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        // store per-peer map
        if (d2dPeer != NODEID_NONE)
            perPeerTxEntities_[d2dPeer].insert(txEnt);

        if (isEmptyingTxBuffer(d2dPeer))
            txEnt->startHoldingDownstreamInPackets();

        return txEnt;
    }
    else {
        // Found
        EV << "LteRlcUmD2D : Using old UmTxBuffer: " << it->second->getId() <<
            " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUmD2D::handleLowerMessage(cPacket *pktAux)
{

    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto chunk = pkt->peekAtFront<inet::Chunk>();

    if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        EV << NOW << " LteRlcUmD2D::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
        auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

        // add here specific behavior for handling mode switch at the RLC layer

        if (switchPkt->getTxSide()) {
            // get the corresponding Rx buffer & call handler
            UmTxEntity *txbuf = getTxBuffer(lteInfo);
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            // forward packet to PDCP
            EV << "LteRlcUmD2D::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
            send(pkt, upOutGate_);
        }
        else { // rx side
            // get the corresponding Rx buffer & call handler
            UmRxEntity *rxbuf = getRxBuffer(lteInfo);
            rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete pkt;
        }
    }
    else
        LteRlcUm::handleLowerMessage(pkt);
}

void LteRlcUmD2D::resumeDownstreamInPackets(MacNodeId peerId)
{
    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return;

    for (auto& txEntity : perPeerTxEntities_.at(peerId)) {
        if (txEntity->isHoldingDownstreamInPackets())
            txEntity->resumeDownstreamInPackets();
    }
}

bool LteRlcUmD2D::isEmptyingTxBuffer(MacNodeId peerId)
{
    EV << NOW << " LteRlcUmD2D::isEmptyingTxBuffer - peerId " << peerId << endl;

    if (peerId == NODEID_NONE || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return false;

    for (auto& entity : perPeerTxEntities_.at(peerId)) {
        if (entity->isEmptyingBuffer()) {
            EV << NOW << " LteRlcUmD2D::isEmptyingTxBuffer - found " << endl;
            return true;
        }
    }
    return false;
}

void LteRlcUmD2D::deleteQueues(MacNodeId nodeId)
{
    // at the UE, delete all connections
    // at the eNB, delete connections related to the given UE
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        // if the entity refers to a D2D_MULTI connection, do not erase it
        if (tit->second->isD2DMultiConnection()) {
            ++tit;
            continue;
        }

        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && MacCidToNodeId(tit->first) == nodeId)) {
            tit->second->deleteModule(); // Delete Entity
            tit = txEntities_.erase(tit);    // Delete Elem
        }
        else {
            ++tit;
        }
    }

    // clear also perPeerTxEntities
    // no need to delete pointed objects (they were already deleted in the previous for loop)
    perPeerTxEntities_.clear();

    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        // if the entity refers to a D2D_MULTI connection, do not erase it
        if (rit->second->isD2DMultiConnection()) {
            ++rit;
            continue;
        }

        if (nodeType == UE || ((nodeType == ENODEB || nodeType == GNODEB) && MacCidToNodeId(rit->first) == nodeId)) {
            rit->second->deleteModule(); // Delete Entity
            rit = rxEntities_.erase(rit);    // Delete Elem
        }
        else {
            ++rit;
        }
    }
}

} //namespace

