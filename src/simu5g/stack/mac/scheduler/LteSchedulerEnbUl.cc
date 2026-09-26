//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/allocator/LteAllocationModule.h"
#include "simu5g/stack/phy/PhyBase.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(LteSchedulerEnbUl);

void LteSchedulerEnbUl::initialize(int stage)
{
    LteSchedulerEnb::initialize(stage);
    if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        WATCH(harqStatus_);
        WATCH(racStatus_);
    }
}

bool LteSchedulerEnbUl::checkEligibility(MacNodeId id, Codeword& cw, GHz carrierFrequency)
{
    HarqRxBuffers *harqRxBuff = mac_->getHarqRxBuffers(carrierFrequency);
    if (harqRxBuff == nullptr)                              // a new HARQ buffer will be created at reception
        return true;

    // check if harq buffer has already been created for this node
    if (harqRxBuff->find(id) != harqRxBuff->end()) {
        LteHarqBufferRx *ulHarq = harqRxBuff->at(id);

        // get current Harq Process for nodeId
        unsigned char currentAcid = harqStatus_[carrierFrequency].at(id);
        // get current Harq Process status
        std::vector<RxUnitStatus> status = ulHarq->getProcess(currentAcid)->getProcessStatus();
        // check if at least one codeword buffer is available for reception
        for ( ; cw < MAX_CODEWORDS; ++cw) {
            if (status.at(cw).second == RXHARQ_PDU_EMPTY) {
                return true;
            }
        }
    }
    return false;
}

void LteSchedulerEnbUl::updateHarqDescs()
{
    EV << NOW << "LteSchedulerEnbUl::updateHarqDescs  cell " << mac_->getMacCellId() << endl;

    for (const auto &[harqKey, harqBuffers] : *harqRxBuffers_) {
        for (const auto &[ueKey, ueBuffer] : harqBuffers) {
            auto currentStatus = harqStatus_[harqKey].find(ueKey);
            if (currentStatus != harqStatus_[harqKey].end()) {
                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " OLD Current Process is  " << (unsigned int)currentStatus->second << endl;
                // updating current acid id
                currentStatus->second = (currentStatus->second + 1) % (ueBuffer->getProcesses());

                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " NEW Current Process is " << (unsigned int)currentStatus->second << "(total harq processes " << ueBuffer->getProcesses() << ")" << endl;
            }
            else {
                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " initialized the H-ARQ status " << endl;
                harqStatus_[harqKey][ueKey] = 0;
            }
        }
    }
}

bool LteSchedulerEnbUl::racschedule(GHz carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();
    unsigned int racAllocatedBlocks = 0;

    auto map_it = racStatus_.find(carrierFrequency);
    if (map_it != racStatus_.end()) {
        RacStatus& racStatus = map_it->second;
        for (const auto& [nodeId, _] : racStatus) {
            EV << NOW << " LteSchedulerEnbUl::racschedule handling RAC for node " << nodeId << endl;

            // apply the allowed-band restriction for this user
            // (NOTE: tempBandLim is deliberately loop-local, matching the
            // historical code, which carried a "FIXME: bandLim is never
            // deleted" at this spot -- although tempBandLim is a stack local
            // and bandLim only ever points at it or at a caller-owned vector)
            BandLimitVector tempBandLim;
            bandLim = applyAllowedBandLimits(nodeId, UL, carrierFrequency, bandLim, tempBandLim);

            // FIXME default behavior
            // try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

            const unsigned int cw = 0;
            const unsigned int blocks = 1;

            bool allocation = false;

            // Minimum RAC grant size: enough for a short BSR + MAC header. Ensures a UE
            // with a poor UL channel still gets a usable Msg3-style grant (see below).
            const unsigned int minRacBytes = 56;

            // Number of RBs actually granted for the RAC (recorded for the schedule list).
            unsigned int grantedBlocks = 0;
            unsigned int grantedBytes = 0;

            unsigned int size = bandLim->size();
            // Allocate RBs across bands until the RAC grant is large enough to carry at
            // least a short BSR + MAC header. Each logical band holds a single RB, so we
            // spread the allocation over multiple bands. The grant size the UE receives is
            // recomputed in LteMacEnb*::sendGrants() as (allocated blocks x bytes-per-block),
            // so the allocation must be widened in *RBs* here -- inflating a byte count has
            // no effect. Without this, a UE far from the gNB with a poor UL channel gets a
            // useless few-byte grant (e.g. 1 RB = 4 B), can never send a BSR or even a
            // 3-byte RLC-AM STATUS PDU, and stalls forever: STATUS PDUs are generated but
            // never transmitted.
            for (Band b = 0; b < size && grantedBytes < minRacBytes; ++b) {
                // if the limit flag is set to skip, jump off
                int limit = bandLim->at(b).limit_.at(cw);
                if (limit == -2) {
                    EV << "LteSchedulerEnbUl::racschedule - skipping logical band according to limit value" << endl;
                    continue;
                }

                if (allocator_->availableBlocks(nodeId, MACRO, b) == 0)
                    continue;

                // bytes this band's RB carries at the UE's current UL CQI/MCS
                unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocks, UL, carrierFrequency);
                if (bytes == 0)
                    continue; // RB cannot carry data at the current CQI -- try another band

                allocator_->addBlocks(MACRO, b, nodeId, blocks, bytes);
                racAllocatedBlocks += blocks;
                grantedBlocks += blocks;
                grantedBytes += bytes;
                allocation = true;
            }

            if (allocation) {
                EV << NOW << " LteSchedulerEnbUl::racschedule UE: " << nodeId << " Handled RAC ("
                   << grantedBlocks << " RBs, " << grantedBytes << " B)" << endl;
            }

            // Fallback: if no band can carry data at the current UL CQI (CQI=0), still
            // allocate a minimum grant on the first available band. This mimics real NR
            // Msg3 grants which use a predefined robust low MCS.
            if (!allocation) {
                for (Band b = 0; b < size; ++b) {
                    int limit = bandLim->at(b).limit_.at(cw);
                    if (limit == -2)
                        continue;
                    if (allocator_->availableBlocks(nodeId, MACRO, b) > 0) {
                        grantedBlocks = blocks;
                        allocator_->addBlocks(MACRO, b, nodeId, grantedBlocks, minRacBytes);
                        racAllocatedBlocks += grantedBlocks;
                        EV << NOW << "LteSchedulerEnbUl::racschedule UE: " << nodeId << " Handled RAC (fallback) on band: " << b << endl;
                        allocation = true;
                        break;
                    }
                }
            }
            if (allocation) {
                // create scList id for current cid/codeword
                MacCid cid = MacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                             // we use the LCID corresponding to the SHORT_BSR
                std::pair<MacCid, Codeword> scListId = {cid, cw};
                scheduleList_[carrierFrequency][scListId] = grantedBlocks;
            }
        }

        // clean up all requests
        racStatus.clear();
    }

    if (racAllocatedBlocks < numBands) {
        // serve RAC for background UEs
        racscheduleBackground(racAllocatedBlocks, carrierFrequency, bandLim);
    }

    // update available blocks
    unsigned int availableBlocks = numBands - racAllocatedBlocks;

    EV << NOW << " LteSchedulerEnbUl::racschedule racAllocatedBlocks: " << racAllocatedBlocks << " availableBlocks after rac schedule: " << availableBlocks << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

    return availableBlocks == 0;
}

void LteSchedulerEnbUl::racscheduleBackground(unsigned int& racAllocatedBlocks, GHz carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbUl::racscheduleBackground - scheduling RAC for background UEs" << endl;

    std::list<MacNodeId> servedRac;

    IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);

    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();

    for (auto it = bgTrafficManager->getWaitingForRacUesBegin(), et = bgTrafficManager->getWaitingForRacUesEnd(); it != et; ++it) {
        // get current nodeId
        MacNodeId bgUeId = MacNodeId(BGUE_MIN_ID + *it);

        EV << NOW << " LteSchedulerEnbUl::racscheduleBackground handling RAC for node " << bgUeId << endl;

        BandLimitVector tempBandLim;
        if (bandLim == nullptr) {
            // Create a vector of band limit using all bands
            makeUniformBandLimits(tempBandLim, numBands, -1);
            bandLim = &tempBandLim;
        }

        // FIXME default behavior
        // try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

        const unsigned int cw = 0;
        const unsigned int blocks = 1;

        unsigned int size = bandLim->size();
        for (Band b = 0; b < size; ++b) {
            // if the limit flag is set to skip, jump off
            int limit = bandLim->at(b).limit_.at(cw);
            if (limit == -2) {
                EV << "LteSchedulerEnbUl::racscheduleBackground - skipping logical band according to limit value" << endl;
                continue;
            }

            if (allocator_->availableBlocks(bgUeId, MACRO, b) > 0) {
                unsigned int bytes = blocks * (bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, UL));
                if (bytes > 0) {
                    allocator_->addBlocks(MACRO, b, bgUeId, 1, bytes);
                    racAllocatedBlocks++;

                    servedRac.push_back(bgUeId);

                    EV << NOW << "LteSchedulerEnbUl::racscheduleBackground UE: " << bgUeId << "Handled RAC on band: " << b << endl;

                    break;
                }
            }
        }
    }

    while (!servedRac.empty()) {
        // notify the traffic manager that the RAC for this UE has been served
        bgTrafficManager->racHandled(servedRac.front());
        servedRac.pop_front();
    }
}

bool LteSchedulerEnbUl::rtxschedule(GHz carrierFrequency, BandLimitVector *bandLim)
{
    try {
        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
        EV << NOW << " LteSchedulerEnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        auto freqIt = harqRxBuffers_->find(carrierFrequency);
        if (freqIt != harqRxBuffers_->end()) {
            auto& rxBufferForCarrierFrequency = freqIt->second;
            for (auto it = rxBufferForCarrierFrequency.begin(); it != rxBufferForCarrierFrequency.end(); ) {
                // get current nodeId and buffer
                auto& [nodeId, harqBuffer] = *it;

                if (nodeId == NODEID_NONE || !binder_->nodeExists(nodeId)) {
                    // UE has left the simulation - erase queue and continue
                    it = rxBufferForCarrierFrequency.erase(it);
                    continue;
                }

                // get current Harq Process for nodeId
                unsigned char currentAcid = harqStatus_[carrierFrequency].at(nodeId);

                // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
                bool skip = true;
                unsigned char acid = (currentAcid + 2) % (harqBuffer->getProcesses());
                LteHarqProcessRx *currentProcess = harqBuffer->getProcess(acid);
                std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
                for (const auto& status : procStatus) {
                    if (status.second == RXHARQ_PDU_CORRUPTED) {
                        skip = false;
                        break;
                    }
                }
                if (skip) {
                    ++it;
                    continue;
                }

                // Get user transmission parameters
                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);// get the user info

                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                unsigned int allocatedBytes = 0;

                // TODO handle the codewords join case (sizeof(cw0+cw1) < currentTbs && currentLayers ==1)

                for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                    // FIXME PERFORMANCE: check for rtx status before calling rtxAcid
                    // perform a retransmission on available codewords for the selected acid
                    unsigned int rtxBytes = LteSchedulerEnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim);
                    if (rtxBytes > 0) {
                        --codewords;
                        allocatedBytes += rtxBytes;

                        mac_->signalProcessForRtx(nodeId, carrierFrequency, UL, false);
                    }
                }
                EV << NOW << "LteSchedulerEnbUl::rtxschedule UE " << nodeId << " allocated bytes : " << allocatedBytes << endl;
                ++it;
            }
        }
        scheduleAdditionalRetransmissions(carrierFrequency, bandLim);

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " LteSchedulerEnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return availableBlocks == 0;
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxschedule(): %s", e.what());
    }
    return false;
}

unsigned int LteSchedulerEnbUl::scheduleAdditionalRetransmissions(GHz carrierFrequency, BandLimitVector *bandLim)
{
    // No additional retransmissions in the clean uplink scheduler. The D2D-mirror
    // HARQ retransmissions are scheduled by the D2D subclasses' override (see
    // LteSchedulerEnbUlD2D / NrSchedulerGnbUlD2D).
    return 0;
}


unsigned int LteSchedulerEnbUl::schedulePerAcidRtx(MacNodeId nodeId, GHz carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    try {
        // apply the allowed-band restriction for this user
        BandLimitVector tempBandLim;
        bandLim = applyAllowedBandLimits(nodeId, direction_, carrierFrequency, bandLim, tempBandLim);

        EV << NOW << "LteSchedulerEnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        LteHarqProcessRx *currentProcess = harqRxBuffers_->at(carrierFrequency).at(nodeId)->getProcess(acid);

        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " LteSchedulerEnbUl::rtxAcid User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        // serve the buffered PDU across the allowed bands and record the allocation
        bool served;
        return allocateRtxBytes(nodeId, direction_, SHORT_BSR, currentProcess->getByteLength(cw), carrierFrequency, cw, acid, bandLim, antenna, served);
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}


void LteSchedulerEnbUl::removePendingRac(MacNodeId nodeId)
{
    for (auto& item : racStatus_) {
        RacStatus::iterator elem_it = item.second.find(nodeId);
        if (elem_it != item.second.end())
            item.second.erase(nodeId);
    }
}

} //namespace
