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

#include "simu5g/stack/d2d/mac/D2dEnbMacHelper.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/d2d/mac/conflictgraph/DistanceBasedConflictGraph.h"
#include "simu5g/stack/d2d/mac/harq/LteHarqBufferMirrorD2D.h"
#include "simu5g/stack/d2d/rrc/D2DModeSwitchNotification_m.h"

namespace simu5g {

using namespace inet;

D2dEnbMacHelper::~D2dEnbMacHelper()
{
    for (auto& [carrierFrequency, buffers] : harqBuffersMirrorD2D_)
        for (auto& [pair, buffer] : buffers)
            delete buffer;
    delete conflictGraph_;
}

HarqBuffersMirrorD2D *D2dEnbMacHelper::getHarqBuffersMirrorD2D(GHz carrierFrequency)
{
    if (harqBuffersMirrorD2D_.find(carrierFrequency) == harqBuffersMirrorD2D_.end())
        return nullptr;
    return &harqBuffersMirrorD2D_[carrierFrequency];
}

void D2dEnbMacHelper::deleteHarqBuffersMirrorD2D(MacNodeId nodeId)
{
    // delete all "mirror" buffers that have nodeId as sender or receiver
    for (auto& mit : harqBuffersMirrorD2D_) {
        for (auto it = mit.second.begin(); it != mit.second.end(); ) {
            // get current nodeIDs
            MacNodeId senderId = (it->first).first; // Transmitter
            MacNodeId destId = (it->first).second;  // Receiver

            if (senderId == nodeId || destId == nodeId) {
                delete it->second;
                it = mit.second.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void D2dEnbMacHelper::deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer)
{
    // delete all "mirror" buffers that have nodeId as sender or receiver
    for (auto& mit : harqBuffersMirrorD2D_) {
        for (auto it = mit.second.begin(); it != mit.second.end(); ) {
            // get current nodeIDs
            MacNodeId senderId = (it->first).first; // Transmitter
            MacNodeId destId = (it->first).second;  // Receiver

            if (senderId == txPeer && destId == rxPeer) {
                delete it->second;
                it = mit.second.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void D2dEnbMacHelper::createDistanceBasedConflictGraph(Binder *binder, D2dBinder *d2dBinder, double threshold,
        double d2dInterferenceRadius, double d2dMultiTxRadius, double d2dMultiInterferenceRadius)
{
    auto cg = new DistanceBasedConflictGraph(binder, d2dBinder, mac_, reuseD2D_, reuseD2DMulti_, threshold);
    cg->setThresholds(d2dInterferenceRadius, d2dMultiTxRadius, d2dMultiInterferenceRadius);
    conflictGraph_ = cg;
}

void D2dEnbMacHelper::computeConflictGraph()
{
    conflictGraph_->computeConflictGraph();
}

void D2dEnbMacHelper::macHandleD2DModeSwitch(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
    auto uinfo = pkt->getTag<UserControlInfo>();

    MacNodeId nodeId = uinfo->getDestId();
    LteD2DMode oldMode = switchPkt->getOldMode();

    if (!switchPkt->getTxSide()) { // address the receiving endpoint of the D2D flow (tx entities at the eNB)
        // get the outgoing connection corresponding to the DL connection for the RX endpoint of the D2D flow
        for (const auto& [cid, connInfo] : mac_->getOutgoingConnectionInfoMap()) {
            if (cid.getNodeId() == nodeId) {
                EV << NOW << " D2dEnbMacHelper::macHandleD2DModeSwitch - send signal for TX entity to upper layers in the eNB (cid=" << cid << ")" << endl;

                auto pktTx = pkt->dup();
                pktTx->removeTagIfPresent<UserControlInfo>();
                auto switchPktTx = pktTx->removeAtFront<D2DModeSwitchNotification>();
                switchPktTx->setTxSide(true);

                if (oldMode == IM)
                    switchPktTx->setOldConnection(true);
                else
                    switchPktTx->setOldConnection(false);
                const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                switchPktTx->setLcg(lcConfig.lcg);
                switchPktTx->setRlcMode(lcConfig.rlcMode);
                pktTx->insertAtFront(switchPktTx);
                *(pktTx->addTag<FlowControlInfo>()) = connInfo.flowInfo.toFlowControlInfo();
                mac_->sendUpperPackets(pktTx);
                break;
            }
        }
    }
    else { // tx side: address the transmitting endpoint of the D2D flow (rx entities at the eNB)
        // clear BSR buffers for the UE
        mac_->clearBsrBuffers(nodeId);

        // get the incoming connection corresponding to the UL connection for the TX endpoint of the D2D flow
        for (const auto& [cid, lteInfo] : mac_->getIncomingConnectionInfoMap()) {
            if (cid.getNodeId() == nodeId) {
                if (msHarqInterrupt_) { // interrupt H-ARQ processes for UL
                    for (auto& mit : *mac_->getHarqRxBuffers()) {
                        HarqRxBuffers::iterator hit = mit.second.find(nodeId);
                        if (hit != mit.second.end()) {
                            for (unsigned int proc = 0; proc < (unsigned int)mac_->harqProcesses(); proc++) {
                                unsigned int numUnits = hit->second->getProcess(proc)->getNumHarqUnits();
                                for (unsigned int i = 0; i < numUnits; i++) {
                                    hit->second->getProcess(proc)->purgeCorruptedPdu(i); // delete contained PDU
                                    hit->second->getProcess(proc)->resetCodeword(i);     // reset unit
                                }
                            }
                        }
                    }

                    // notify that this UE is switching during this TTI
                    mac_->recordHarqReset(nodeId);
                }

                auto pktRx = pkt->dup();
                pktRx->removeTagIfPresent<UserControlInfo>();
                auto switchPktRx = pktRx->removeAtFront<D2DModeSwitchNotification>();

                EV << NOW << " D2dEnbMacHelper::macHandleD2DModeSwitch - send signal for RX entity to upper layers in the eNB (cid=" << cid << ")" << endl;

                switchPktRx->setTxSide(false);
                if (oldMode == IM)
                    switchPktRx->setOldConnection(true);
                else
                    switchPktRx->setOldConnection(false);
                const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
                switchPktRx->setLcg(lcConfig.lcg);
                switchPktRx->setRlcMode(lcConfig.rlcMode);

                pktRx->insertAtFront(switchPktRx);
                *(pktRx->addTag<FlowControlInfo>()) = lteInfo.toFlowControlInfo();
                mac_->sendUpperPackets(pktRx);
                break;
            }
        }
    }
}

} //namespace
