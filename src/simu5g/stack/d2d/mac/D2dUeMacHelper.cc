//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/d2d/mac/D2dUeMacHelper.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/stack/d2d/rrc/D2DModeSwitchNotification_m.h"

namespace simu5g {

using namespace inet;

D2dUeMacHelper::~D2dUeMacHelper()
{
    delete preconfiguredTxParams_;
}

void D2dUeMacHelper::initWatches()
{
    WATCH(racD2DMulticastRequested_);
    WATCH(bsrD2DMulticastTriggered_);
}

//Function to create only a BSR for the eNB
Packet *D2dUeMacHelper::makeBsr(int size)
{
    auto macPkt = new Packet("LteMacPdu");
    auto header = makeShared<LteMacPdu>();
    header->setHeaderLength(MAC_HEADER);
    macPkt->setTimestamp(NOW);

    MacBsr *bsr = new MacBsr();
    bsr->setTimestamp(simTime().dbl());
    bsr->setSize(size);
    header->pushCe(bsr);
    macPkt->insertAtFront(header);
    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(mac_->getMacNodeId());
    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(mac_->getMacCellId());
    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);

    mac_->cancelBsr();
    EV << "D2dUeMacHelper::makeBsr() - BSR with size " << size << " bytes created" << endl;
    return macPkt;
}

void D2dUeMacHelper::rebuildPreconfiguredTxParams(Binder *binder)
{
    delete preconfiguredTxParams_;
    preconfiguredTxParams_ = buildPreconfiguredTxParams(binder);
}

UserTxParams *D2dUeMacHelper::buildPreconfiguredTxParams(Binder *binder)
{
    UserTxParams *txParams = new UserTxParams();

    // default parameters for D2D
    txParams->setValid(true);
    txParams->writeTxMode(TRANSMIT_DIVERSITY);
    Rank ri = 1;                                              // rank for TxD is one
    txParams->writeRank(ri);

    Cqi cqi = mac_->par("d2dCqi");
    if (cqi < 0 || cqi > 15) {
        delete txParams;
        throw cRuntimeError("D2dUeMacHelper::buildPreconfiguredTxParams - CQI %hu is not a valid value", cqi);
    }
    txParams->writeCqi(std::vector<Cqi>(1, cqi));

    BandSet b;
    CellInfo *cellInfo = binder->getCellInfoByNodeId(mac_->getMacNodeId());
    if (cellInfo != nullptr) {
        for (Band i = 0; i < cellInfo->getNumBands(); ++i)
            b.insert(i);
    }
    else {
        delete txParams;
        throw cRuntimeError("D2dUeMacHelper::buildPreconfiguredTxParams - cellInfo is a NULL pointer");
    }

    RemoteSet antennas;
    antennas.insert(MACRO);
    txParams->writeAntennas(antennas);

    return txParams;
}

void D2dUeMacHelper::macHandleD2DModeSwitch(cPacket *pktAux)
{
    EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - Start" << endl;

    // All data in the MAC buffers of the connection to be switched are deleted.

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

    bool txSide = switchPkt->getTxSide();
    MacNodeId peerId = switchPkt->getPeerId();
    LteD2DMode newMode = switchPkt->getNewMode();
    LteD2DMode oldMode = switchPkt->getOldMode();

    if (txSide) {
        mac_->emit(rcvdD2DModeSwitchNotificationSignal_, (long)1);

        Direction newDirection = (newMode == DM) ? D2D : UL;
        Direction oldDirection = (oldMode == DM) ? D2D : UL;

        // Find the correct connections involved in the mode switch
        // Use two-phase approach: first collect, then process

        // Phase 1: Collect CIDs and flow info that need processing
        std::vector<std::pair<MacCid, FlowControlInfo>> oldConnections;
        std::vector<std::pair<MacCid, FlowControlInfo>> newConnections;

        for (const auto& [cid, connDesc] : mac_->getOutgoingConnectionInfoMap()) {
            const auto& connInfo = connDesc.flowInfo;
            if (connInfo.getD2dRxPeerId() == peerId && connInfo.getDirection() == oldDirection) {
                oldConnections.emplace_back(cid, connInfo);
            }
            else if (connInfo.getD2dRxPeerId() == peerId && connInfo.getDirection() == newDirection) {
                newConnections.emplace_back(cid, connInfo);
            }
        }

        // Interrupt the H-ARQ processes once per switch, not per old connection: the eNB
        // purges its UL RX H-ARQ for this UE unconditionally, and the old connection may
        // not be recognizable here -- the UL connection is the UE's DRB to the eNB, and when
        // the eNB established it, its flow carries no D2D peer ids. A UL process left
        // waiting for feedback the eNB will never send is reused later while not empty.
        if (oldDirection != newDirection && switchPkt->getInterruptHarq()) {
            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - interrupting H-ARQ processes" << endl;

            for (auto& [carrierFrequency, harqTxBuffers] : *mac_->getHarqTxBuffers()) {
                for (MacNodeId id : {peerId, mac_->getMacCellId()}) { // SL, then UL
                    auto hit = harqTxBuffers.find(id);
                    if (hit != harqTxBuffers.end()) {
                        for (unsigned int proc = 0; proc < (unsigned int)mac_->harqProcesses(); proc++)
                            hit->second->forceDropProcess(proc);
                    }
                }
            }
        }

        // Phase 2: Process old connections (safe to modify containers now)
        for (const auto& [cid, connInfo] : oldConnections) {
            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - found old connection with cid " << cid << ", erasing buffered data" << endl;
            if (oldDirection != newDirection) {
                if (switchPkt->getClearRlcBuffer()) {
                    EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - erasing buffered data" << endl;

                    // Clear buffers but keep the connection alive for potential mode switch back
                    mac_->clearOutgoingConnectionBuffers(cid);
                }
            }

            // Abort BSR requests
            mac_->cancelBsr();

            auto pktDup = pkt->dup();
            auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
            switchPkt_dup->setOldConnection(true);
            {
                const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                switchPkt_dup->setLcg(lcConfig.lcg);
                switchPkt_dup->setRlcMode(lcConfig.rlcMode);
            }
            pktDup->insertAtFront(switchPkt_dup);
            *(pktDup->addTagIfAbsent<FlowControlInfo>()) = connInfo;
            mac_->sendUpperPackets(pktDup);

            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - send switch signal to the RLC TX entity corresponding to the old mode, cid " << cid << endl;
        }

        // Phase 3: Process new connections
        for (const auto& [cid, connInfo] : newConnections) {
            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - send switch signal to the RLC TX entity corresponding to the new mode, cid " << cid << endl;
            if (oldDirection != newDirection) {
                auto pktDup = pkt->dup();
                auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                switchPkt_dup->setOldConnection(false);
                {
                    const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                    switchPkt_dup->setLcg(lcConfig.lcg);
                    switchPkt_dup->setRlcMode(lcConfig.rlcMode);
                }
                pktDup->insertAtFront(switchPkt_dup);
                *(pktDup->addTagIfAbsent<FlowControlInfo>()) = connInfo;
                mac_->sendUpperPackets(pktDup);
            }
        }
    }
    else { // rx side
        Direction newDirection = (newMode == DM) ? D2D : DL;
        Direction oldDirection = (oldMode == DM) ? D2D : DL;

        // Find the correct connections involved in the mode switch
        // Use two-phase approach: first collect, then process

        // Phase 1: Collect CIDs and flow info that need processing
        std::vector<std::pair<MacCid, FlowControlInfo>> oldRxConnections;
        std::vector<std::pair<MacCid, FlowControlInfo>> newRxConnections;

        for (const auto& [cid, connInfo] : mac_->getIncomingConnectionInfoMap()) {
            if (connInfo.getD2dTxPeerId() == peerId && connInfo.getDirection() == oldDirection) {
                oldRxConnections.emplace_back(cid, connInfo);
            }
            else if (connInfo.getD2dTxPeerId() == peerId && connInfo.getDirection() == newDirection) {
                newRxConnections.emplace_back(cid, connInfo);
            }
        }

        // Phase 2: Process old RX connections
        for (const auto& [cid, connInfo] : oldRxConnections) {
            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - found old connection with cid " << cid << ", send signal to the RLC RX entity" << endl;
            if (oldDirection != newDirection) {
                if (switchPkt->getInterruptHarq()) {
                    // Interrupt H-ARQ processes for SL
                    MacNodeId id = peerId;
                    for (auto& mrit : *mac_->getHarqRxBuffers()) {
                        HarqRxBuffers::iterator hit = mrit.second.find(id);
                        if (hit != mrit.second.end()) {
                            for (unsigned int proc = 0; proc < (unsigned int)mac_->harqProcesses(); proc++) {
                                unsigned int numUnits = hit->second->getProcess(proc)->getNumHarqUnits();
                                for (unsigned int i = 0; i < numUnits; i++) {
                                    hit->second->getProcess(proc)->purgeCorruptedPdu(i); // delete contained PDU
                                    hit->second->getProcess(proc)->resetCodeword(i);     // reset unit
                                }
                            }
                        }
                    }

                    // Clear mirror H-ARQ buffers
                    enb_->deleteHarqBuffersMirrorD2D(peerId, mac_->getMacNodeId());

                    // Notify that this UE is switching during this TTI
                    mac_->recordHarqReset(peerId);
                }

                auto pktDup = pkt->dup();
                auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                switchPkt_dup->setOldConnection(true);
                {
                    const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                    switchPkt_dup->setLcg(lcConfig.lcg);
                    switchPkt_dup->setRlcMode(lcConfig.rlcMode);
                }
                pktDup->insertAtFront(switchPkt_dup);
                *(pktDup->addTagIfAbsent<FlowControlInfo>()) = connInfo;
                mac_->sendUpperPackets(pktDup);
            }
        }

        // Phase 3: Process new RX connections
        for (const auto& [cid, connInfo] : newRxConnections) {
            EV << NOW << " D2dUeMacHelper::macHandleD2DModeSwitch - found new connection with cid " << cid << ", send signal to the RLC RX entity" << endl;
            if (oldDirection != newDirection) {
                auto pktDup = pkt->dup();
                auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                switchPkt_dup->setOldConnection(false);
                {
                    const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                    switchPkt_dup->setLcg(lcConfig.lcg);
                    switchPkt_dup->setRlcMode(lcConfig.rlcMode);
                }
                pktDup->insertAtFront(switchPkt_dup);
                *(pktDup->addTagIfAbsent<FlowControlInfo>()) = connInfo;
                mac_->sendUpperPackets(pktDup);
            }
        }
    }

    delete pkt;
}

} //namespace
