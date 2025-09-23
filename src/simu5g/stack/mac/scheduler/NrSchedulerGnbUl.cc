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

#include "simu5g/stack/mac/scheduler/NrSchedulerGnbUl.h"
#include "simu5g/stack/mac/NrMacGnb.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/allocator/LteAllocationModule.h"

namespace simu5g {

bool NrSchedulerGnbUl::checkEligibility(MacNodeId id, Codeword& cw, GHz carrierFrequency)
{
    HarqRxBuffers *harqRxBuff = mac_->getHarqRxBuffers(carrierFrequency);
    if (harqRxBuff == nullptr)                              // a new HARQ buffer will be created at reception
        return true;

    // check if HARQ buffer has already been created for this node
    if (harqRxBuff->find(id) != harqRxBuff->end()) {
        LteHarqBufferRx *ulHarq = harqRxBuff->at(id);
        UnitList freeUnits = ulHarq->firstAvailable();

        if (freeUnits.first != HARQ_NONE) {
            if (freeUnits.second.empty())
                // there is a process currently selected for user <id>, but all of its CWs have already been used.
                return false;
            // retrieving the CW index
            cw = freeUnits.second.front();
            // DEBUG check
            if (cw > MAX_CODEWORDS)
                throw cRuntimeError("NrSchedulerGnbUl::checkEligibility(): abnormal codeword id %d", (int)cw);
            return true;
        }
    }
    return true;
}

bool NrSchedulerGnbUl::rtxschedule(GHz carrierFrequency, BandLimitVector *bandLim)
{
    try {
        EV << NOW << " NrSchedulerGnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
        EV << NOW << " NrSchedulerGnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << " - Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        // retrieving reference to HARQ entities
        HarqRxBuffers *harqQueues = mac_->getHarqRxBuffers(carrierFrequency);
        if (harqQueues != nullptr) {
            for (auto [nodeId, currHarq] : *harqQueues) {
                if (nodeId == NODEID_NONE || !binder_->nodeExists(nodeId)) {
                    // UE has left the simulation - erase queue and continue
                    harqRxBuffers_->at(carrierFrequency).erase(nodeId);
                    continue;
                }

                // Get user transmission parameters
                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);// get the user info
                // TODO SK Get the number of codewords - FIX with correct mapping
                // TODO is there a way to get codewords without calling computeTxParams??
                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords

                EV << NOW << " NrSchedulerGnbUl::rtxschedule UE: " << nodeId << endl;

                // get the number of HARQ processes
                unsigned int maxProcesses = currHarq->getProcesses();

                for (unsigned int process = 0; process < maxProcesses; ++process) {
                    // for each HARQ process
                    LteHarqProcessRx *currProc = currHarq->getProcess(process);

                    if (allocatedCws_[nodeId] == codewords)
                        break;

                    unsigned int allocatedBytes = 0;
                    for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                        if (allocatedCws_[nodeId] == codewords)
                            break;

                        // skip processes which are not in RTX status
                        if (currProc->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
                            EV << NOW << " NrSchedulerGnbUl::rtxschedule UE " << nodeId << " - detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                            continue;
                        }

                        // if the process is in CORRUPTED state, then schedule a retransmission for this process

                        // FIXME PERFORMANCE: check for RTX status before calling rtxAcid

                        // perform a retransmission on available codewords for the selected acid
                        unsigned int rtxBytes = schedulePerAcidRtx(nodeId, carrierFrequency, cw, process, bandLim);
                        if (rtxBytes > 0) {
                            --codewords;
                            allocatedBytes += rtxBytes;

                            mac_->signalProcessForRtx(nodeId, carrierFrequency, UL, false);
                        }
                    }
                    EV << NOW << "NrSchedulerGnbUl::rtxschedule UE " << nodeId << " - allocated bytes : " << allocatedBytes << endl;
                }
            }
        }
        if (mac_->isD2DCapable()) {
            // --- START Schedule D2D retransmissions --- //
            Direction dir = D2D;
            HarqBuffersMirrorD2D *harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D *>(mac_.get())->getHarqBuffersMirrorD2D(carrierFrequency);
            if (harqBuffersMirrorD2D != nullptr) {
                for (auto it_d2d = harqBuffersMirrorD2D->begin(); it_d2d != harqBuffersMirrorD2D->end(); ) {
                    auto& [d2dPair, currHarq] = *it_d2d;
                    // get current nodeIDs
                    MacNodeId senderId = d2dPair.first; // Transmitter
                    MacNodeId destId = d2dPair.second;  // Receiver

                    if (senderId == NODEID_NONE || !binder_->nodeExists(senderId)) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }
                    if (destId == NODEID_NONE || !binder_->nodeExists(destId)) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }

                    // Get user transmission parameters
                    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir, carrierFrequency);// get the user info

                    unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                    unsigned int allocatedBytes = 0;

                    // TODO handle the codewords join case (size of(cw0+cw1) < currentTBS && currentLayers ==1)

                    EV << NOW << " NrSchedulerGnbUl::rtxschedule D2D TX UE: " << senderId << " - RX UE: " << destId << endl;

                    // get the number of HARQ processes
                    unsigned int maxProcesses = currHarq->getProcesses();

                    for (unsigned int process = 0; process < maxProcesses; ++process) {
                        // for each HARQ process
                        LteHarqProcessMirrorD2D *currProc = currHarq->getProcess(process);

                        if (allocatedCws_[senderId] == codewords)
                            break;

                        for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                            EV << NOW << " NrSchedulerGnbUl::rtxschedule process " << process << endl;
                            EV << NOW << " NrSchedulerGnbUl::rtxschedule ------- CODEWORD " << cw << endl;

                            // skip processes which are not in RTX status
                            if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED) {
                                EV << NOW << " NrSchedulerGnbUl::rtxschedule D2D UE: " << senderId << " detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                                continue;
                            }

                            // FIXME PERFORMANCE: check for RTX status before calling rtxAcid

                            // perform a retransmission on available codewords for the selected acid
                            unsigned int rtxBytes = schedulePerAcidRtxD2D(destId, senderId, carrierFrequency, cw, process, bandLim);
                            if (rtxBytes > 0) {
                                --codewords;
                                allocatedBytes += rtxBytes;

                                mac_->signalProcessForRtx(senderId, carrierFrequency, D2D, false);
                            }
                        }
                        EV << NOW << " NrSchedulerGnbUl::rtxschedule - D2D UE: " << senderId << " allocated bytes : " << allocatedBytes << endl;
                    }
                    ++it_d2d;
                }
            }
            // --- END Schedule D2D retransmissions --- //
        }

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " NrSchedulerGnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;
        EV << NOW << " NrSchedulerGnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return availableBlocks == 0;
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in NrSchedulerGnbUl::rtxschedule(): %s", e.what());
    }
    return false;
}

} //namespace
