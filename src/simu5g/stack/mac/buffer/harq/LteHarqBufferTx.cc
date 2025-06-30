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

#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

namespace simu5g {

using namespace omnetpp;

LteHarqBufferTx::LteHarqBufferTx(Binder *binder, unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac)
    : macOwner_(owner), processes_(numProc, nullptr), numProc_(numProc), numEmptyProc_(numProc), selectedAcid_(HARQ_NONE), nodeId_(dstMac->getMacNodeId())
{
    for (unsigned int i = 0; i < numProc_; i++) {
        processes_[i] = new LteHarqProcessTx(binder, i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
    }
}

LteHarqBufferTx::LteHarqBufferTx(Binder *binder, unsigned int numProc, LteMacBase *owner)
    : macOwner_(owner), processes_(numProc, nullptr), numProc_(numProc), numEmptyProc_(numProc), selectedAcid_(HARQ_NONE), nodeId_((MacNodeId)-1)
{
}

UnitList LteHarqBufferTx::firstReadyForRtx()
{
    unsigned char oldestProcessAcid = HARQ_NONE;
    simtime_t oldestTxTime = NOW + 1;
    simtime_t currentTxTime = 0;

    for (unsigned int i = 0; i < numProc_; i++) {
        if (processes_[i]->hasReadyUnits()) {
            currentTxTime = processes_[i]->getOldestUnitTxTime();
            if (currentTxTime < oldestTxTime) {
                oldestTxTime = currentTxTime;
                oldestProcessAcid = i;
            }
        }
    }
    UnitList ret;
    ret.first = oldestProcessAcid;
    if (oldestProcessAcid != HARQ_NONE) {
        ret.second = processes_[oldestProcessAcid]->readyUnitsIds();
    }
    return ret;
}

int64_t LteHarqBufferTx::pduLength(unsigned char acid, Codeword cw)
{
    return processes_[acid]->getPduLength(cw);
}

void LteHarqBufferTx::markSelected(UnitList unitIds, unsigned char availableTbs)
{
    if (unitIds.second.size() == 0) {
        EV << "H-ARQ TX buffer: markSelected(): empty unitIds list" << endl;
        return;
    }

    if (availableTbs == 0) {
        EV << "H-ARQ TX buffer: markSelected(): no available TBs" << endl;
        return;
    }

    unsigned char acid = unitIds.first;
    CwList cwList = unitIds.second;

    if (cwList.size() > availableTbs) {
        //eg: tx mode siso trying to rtx 2 TBs
        // this is the codeword which will contain the jumbo TB
        Codeword cw = cwList.front();
        cwList.pop_front();
        auto pkt = processes_[acid]->getPdu(cw);
        auto basePdu = pkt->removeAtFront<LteMacPdu>();
        while (cwList.size() > 0) {
            Codeword cw = cwList.front();
            cwList.pop_front();
            auto pkt2 = processes_[acid]->getPdu(cw);
            auto guestPdu = pkt2->removeAtFront<LteMacPdu>();
            while (guestPdu->hasSdu())
                basePdu->pushSdu(guestPdu->popSdu());
            while (guestPdu->hasCe())
                basePdu->pushCe(guestPdu->popCe());
            pkt2->insertAtFront(guestPdu);
            processes_[acid]->dropPdu(cw);
        }
        pkt->insertAtFront(basePdu);
        processes_[acid]->markSelected(cw);
    }
    else {
        // all units are marked
        for (const auto& cw : cwList) {
            processes_[acid]->markSelected(cw);
        }
    }

    selectedAcid_ = acid;

    // debug output
    EV << "H-ARQ TX: process " << (int)selectedAcid_ << " has been selected for retransmission" << endl;
}

void LteHarqBufferTx::insertPdu(unsigned char acid, Codeword cw, Packet *pkt)
{
    auto pdu = pkt->peekAtFront<LteMacPdu>();

    if (selectedAcid_ == HARQ_NONE) {
        // the process has not been used for rtx, or it is the first TB inserted, it must be completely empty
        if (!processes_[acid]->isEmpty())
            throw cRuntimeError("H-ARQ TX buffer: new process selected for tx is not completely empty");
    }

    if (!processes_[acid]->isUnitEmpty(cw))
        throw cRuntimeError("LteHarqBufferTx::insertPdu(): unit is not empty");

    selectedAcid_ = acid;
    numEmptyProc_--;
    processes_[acid]->insertPdu(pkt, cw);

    auto tag = pkt->getTag<UserControlInfo>();
    // debug output
    EV << "H-ARQ TX: new pdu (id " << pdu->getId() << " ) inserted into process " << (int)acid << " "
                                                                                                  "codeword id: " << (int)cw << " "
                                                                                                                                "for node with id " << tag->getDestId() << endl;

    macOwner_->insertMacPdu(pkt);
}

UnitList LteHarqBufferTx::firstAvailable()
{
    UnitList ret;

    unsigned char acid = HARQ_NONE;

    if (selectedAcid_ == HARQ_NONE) {
        for (unsigned int i = 0; i < numProc_; i++) {
            if (processes_[i]->isEmpty()) {
                acid = i;
                break;
            }
        }
    }
    else {
        acid = selectedAcid_;
    }
    ret.first = acid;

    if (acid != HARQ_NONE) {
        // if there is any free process, return empty list
        ret.second = processes_[acid]->emptyUnitsIds();
    }

    return ret;
}

UnitList LteHarqBufferTx::getEmptyUnits(unsigned char acid)
{
    // TODO add multi CW check and retransmission checks
    UnitList ret;
    ret.first = acid;
    ret.second = processes_[acid]->emptyUnitsIds();
    return ret;
}

void LteHarqBufferTx::receiveHarqFeedback(Packet *pkt)
{
    EV << "LteHarqBufferTx::receiveHarqFeedback - start" << endl;

    auto fbpkt = pkt->peekAtFront<LteHarqFeedback>();
    bool result = fbpkt->getResult();
    HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
    Codeword cw = fbpkt->getCw();
    unsigned char acid = fbpkt->getAcid();
    long fbPduId = fbpkt->getFbMacPduId(); // id of the pdu that should receive this feedback
    long unitPduId = processes_[acid]->getPduId(cw);

    // After handover or a D2D mode switch, the process may have been dropped. The received feedback must be ignored.
    if (processes_[acid]->isDropped()) {
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << ". The corresponding unit has been "
                                                                        " reset after handover or a D2D mode switch (the contained pdu was dropped). Ignore feedback." << endl;
        delete pkt;
        return;
    }

    if (fbPduId != unitPduId) {
        // feedback is not for the pdu in this unit, maybe the addressed one was dropped
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << " Codeword " << cw << " not addressed"
                                                                                             " to the actually contained pdu (maybe it was dropped)" << endl;
        EV << "Received id: " << fbPduId << endl;
        EV << "PDU id: " << unitPduId << endl;
        // todo: comment endsim after tests
        throw cRuntimeError("H-ARQ TX: feedback is not for the pdu in this unit, maybe the addressed one was dropped");
    }

    /*
     * @author Alessandro Noferi
     *
     * place this piece of code before:
     * processes_[acid]->pduFeedback(harqResult, cw);
     * since it deletes the pdu
     */
    if (harqResult == HARQACK) {
        auto macPdu = processes_[acid]->getPdu(cw)->peekAtFront<LteMacPdu>();
        auto userInfo = pkt->getTag<UserControlInfo>();
        macOwner_->harqAckToFlowManager(userInfo, macPdu);
    }

    bool reset = processes_[acid]->pduFeedback(harqResult, cw);
    if (reset)
        numEmptyProc_++;

    // debug output
    const char *ack = result ? "ACK" : "NACK";
    EV << "H-ARQ TX: feedback received for process " << (int)acid << " codeword " << (int)cw << ""
                                                                                                " result is " << ack << endl;

    ASSERT(pkt->getOwner() == this->macOwner_);
    delete pkt;
}

void LteHarqBufferTx::sendSelectedDown()
{
    if (selectedAcid_ == HARQ_NONE) {
        EV << "LteHarqBufferTx::sendSelectedDown - no process selected in H-ARQ buffer, nothing to do" << endl;
        return;
    }

    CwList ul = processes_[selectedAcid_]->selectedUnitsIds();
    for (const auto& id : ul) {
        auto pkt = processes_[selectedAcid_]->extractPdu(id);
        auto pduToSend = pkt->peekAtFront<LteMacPdu>();
        auto cinfo = pkt->getTag<UserControlInfo>();
        macOwner_->sendLowerPackets(pkt);

        // debug output
        EV << "\t H-ARQ TX: pdu (id " << pduToSend->getId() << " ) extracted from process " << (int)selectedAcid_ << " "
                "codeword " << (int)id << " for node with id " << cinfo->getDestId() << endl;
    }
    selectedAcid_ = HARQ_NONE;
}

void LteHarqBufferTx::dropProcess(unsigned char acid)
{
    // pdus can be dropped only if the unit is in BUFFERED state.
    CwList ul = processes_[acid]->readyUnitsIds();

    for (auto& id : ul) {
        processes_[acid]->dropPdu(id);
    }
    // if a process contains units in BUFFERED state, then all units of this
    // process are either empty or in BUFFERED state (ready).
    numEmptyProc_++;
}

void LteHarqBufferTx::selfNack(unsigned char acid, Codeword cw)
{
    bool reset = false;
    CwList ul = processes_[acid]->readyUnitsIds();

    for (const auto& unitId : ul) {
        reset = processes_[acid]->selfNack(unitId);
    }
    if (reset)
        numEmptyProc_++;
}

void LteHarqBufferTx::forceDropProcess(unsigned char acid)
{
    processes_[acid]->forceDropProcess();
    if (acid == selectedAcid_)
        selectedAcid_ = HARQ_NONE;
    numEmptyProc_++;
}

void LteHarqBufferTx::forceDropUnit(unsigned char acid, Codeword cw)
{
    bool reset = processes_[acid]->forceDropUnit(cw);
    if (reset) {
        if (acid == selectedAcid_)
            selectedAcid_ = HARQ_NONE;
        numEmptyProc_++;
    }
}

BufferStatus LteHarqBufferTx::getBufferStatus()
{
    BufferStatus bs(numProc_);
    unsigned int numHarqUnits = 0;
    for (unsigned int i = 0; i < numProc_; i++) {
        numHarqUnits = processes_[i]->getNumHarqUnits();
        std::vector<UnitStatus> vus(numHarqUnits);
        vus = processes_[i]->getProcessStatus();
        bs[i] = vus;
    }
    return bs;
}

LteHarqProcessTx *LteHarqBufferTx::getProcess(unsigned char acid)
{
    try {
        return processes_.at(acid);
    }
    catch (std::out_of_range& x) {
        throw cRuntimeError("LteHarqBufferTx::getProcess(): acid %i out of bounds", int(acid));
    }
}

LteHarqProcessTx *LteHarqBufferTx::getSelectedProcess()
{
    return getProcess(selectedAcid_);
}

// @author Alessandro Noferi

bool LteHarqBufferTx::isHarqBufferActive() const {
    for (auto process : processes_) {
        if (process->isHarqProcessActive()) {
            return true;
        }
    }
    return false;
}

LteHarqBufferTx::~LteHarqBufferTx()
{
    for (auto process : processes_)
        delete process;

    processes_.clear();
}

bool LteHarqBufferTx::isInUnitList(unsigned char acid, Codeword cw, UnitList unitIds)
{
    if (acid != unitIds.first)
        return false;

    for (const auto& codeword : unitIds.second) {
        if (cw == codeword) {
            return true;
        }
    }
    return false;
}

} //namespace

