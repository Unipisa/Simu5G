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

#include "simu5g/stack/mac/NrMacUe.h"

#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
#include "simu5g/stack/mac/buffer/LteMacQueue.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/mac/packet/LteSchedulingGrant.h"
#include "simu5g/stack/mac/scheduler/LteSchedulerUeUl.h"

namespace simu5g {

Define_Module(NrMacUe);

NrMacUe::NrMacUe()
{
    isNr_ = true;
}

void NrMacUe::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& [carrierFreq, harqRxMap] : harqRxBuffers_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        std::list<Packet *> pduList;
        for (auto [macNodeId, rxBuf] : harqRxMap) {
            pduList = rxBuf->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "NrMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no HARQ counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    for (auto& [carrierFreq, grant] : schedulingGrant_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (grant != nullptr)
            noSchedulingGrants = false;
    }

    if (noSchedulingGrants) {
        EV << NOW << " NrMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
        checkRAC();
        // TODO ensure all operations done before return (i.e. move H-ARQ RX purge before this point)
    }
    else {
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (auto& [carrierFreq, grant] : schedulingGrant_) {
            if (grant != nullptr && grant->getPeriodic()) {
                periodicGrant = true;

                // Periodic checks
                if (--expirationCounter_[carrierFreq] < 0) {
                    // Periodic grant is expired
                    grant = nullptr;
                    checkRac = true;
                }
                else if (--periodCounter_[carrierFreq] > 0) {
                    skip = true;
                }
                else {
                    // resetting grant period
                    periodCounter_[carrierFreq] = grant->getPeriod();
                    // this is periodic grant TTI - continue with frame sending
                    checkRac = false;
                    skip = false;
                    break;
                }
            }
        }
        if (periodicGrant) {
            if (checkRac)
                checkRAC();
            else {
                if (skip)
                    return;
            }
        }
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) { // if a grant is configured
        EV << NOW << " NrMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        if (!firstTx) {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx = true;
            // the gNb will receive the first PDU in 2 TTI, thus initializing acid to 0
            currentHarq_ = harqProcesses_ - 2;
        }

        for (auto& [carrierFrequency, harqTxBufferMap] : harqTxBuffers_) {
            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq((carrierFrequency))) > 0)
                continue;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                continue;

            for (auto [it2Key, currHarq] : harqTxBufferMap) {
                unsigned int numProcesses = currHarq->getNumProcesses();

                for (unsigned int proc = 0; proc < numProcesses; proc++) {
                    LteHarqProcessTx *currProc = currHarq->getProcess(proc);

                    // check if the current process has unit ready for retransmission
                    bool ready = currProc->hasReadyUnits();
                    CwList cwListRetx = currProc->readyUnitsIds();

                    EV << "\t [process=" << proc << "] , [retx=" << (ready ? "true" : "false") << "] , [n=" << cwListRetx.size() << "]" << endl;

                    // check if one 'ready' unit has the same direction of the grant
                    bool checkDir = false;
                    for (Codeword cw : cwListRetx) {
                        auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                        if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection()) {
                            checkDir = true;
                            break;
                        }
                    }

                    // if a retransmission is needed
                    if (ready && checkDir) {
                        UnitList signal;
                        signal.first = proc;
                        signal.second = cwListRetx;
                        currHarq->markSelected(signal, schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                        retx = true;
                        break;
                    }
                }
            }
        }
        // if no retransmission is needed, proceed with normal scheduling
        if (!retx) {
            emptyScheduleList_ = true;
            bool harqBlocked = false;
            std::map<GHz, LteSchedulerUeUl *>::iterator sit;
            for (auto [carrierFrequency, carrierLcgScheduler] : lcgScheduler_) {
                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq((carrierFrequency))) > 0)
                    continue;

                if (isUlGrantBlockedByHarq(carrierFrequency)) {
                    EV << "NrMacUe::handleSelfMessage - no free H-ARQ process for the grant on carrier [" << carrierFrequency << "], the grant is not used" << endl;
                    harqBlocked = true;
                    continue;
                }

                EV << "NrMacUe::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "NrMacUe::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            // a standalone BSR also needs an H-ARQ process, and buildStandaloneBsr()
            // may pick a blocked carrier's grant: with one blocked, it waits
            if (isBsrPending() && emptyScheduleList_ && !harqBlocked) {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else {
                requestedSdus_ = macSduRequest(); // returns an integer
            }
        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    if (debugHarq_) {
        for (auto& [carrierFreq, harqTxBufferMap] : harqTxBuffers_) {
            EV << "\n carrier[ " << carrierFreq << "] htxbuf.size " << harqTxBufferMap.size() << endl;
            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for (auto [currId, currHarq] : harqTxBufferMap) {
                BufferStatus harqStatus = currHarq->getBufferStatus();
                EV << "\t cycleOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for (const auto& jt : harqStatus) {
                    EV << "\t\t cycleInner " << cntInner << " - jt->size=" << jt.size()
                       << " - statusCw(0/1)=" << jt.at(0).second << "/" << jt.at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    // update current HARQ process id, if needed
    if (requestedSdus_ == 0) {
        EV << NOW << " NrMacUe::handleSelfMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_ + 1) % harqProcesses_ << endl;
        currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
    }

    decreaseNumerologyPeriodCounter();

    EV << "--- END UE MAIN LOOP ---" << endl;
}

UnitList NrMacUe::reserveTxHarqUnits(LteHarqBufferTx *txBuf, Direction dir)
{
    // asynchronous H-ARQ: search for an empty unit within the first available process
    return txBuf->firstAvailable();
}

Packet *NrMacUe::createUlMacPdu(MacCid destCid, GHz carrierFreq, MacNodeId destId)
{
    auto macPkt = new Packet("LteMacPdu");
    auto header = makeShared<LteMacPdu>();
    header->setHeaderLength(MAC_HEADER);
    macPkt->insertAtFront(header);

    // the direction is the flow's (UL, or D2D on a D2D-capable subclass), not a constant
    auto info = macPkt->addTagIfAbsent<UserControlInfo>();
    info->setSourceId(getMacNodeId());
    info->setDestId(destId);
    info->setDirection(connDescOut_.at(destCid).flowInfo.getDirection());
    info->setPacketLcid(SHORT_BSR);
    info->setCarrierFrequency(carrierFreq);
    info->setGrantId(schedulingGrant_[carrierFreq]->getGrantId());
    info->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

    return macPkt;
}

} //namespace
