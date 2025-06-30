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

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/LteMacBase.h"
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

unsigned int LteHarqBufferRx::totalCellRcvdBytes_ = 0;

using namespace omnetpp;

simsignal_t LteHarqBufferRx::macCellThroughputSignal_[2] = { cComponent::registerSignal("macCellThroughputDl"), cComponent::registerSignal("macCellThroughputUl") };
simsignal_t LteHarqBufferRx::macDelaySignal_[2] = { cComponent::registerSignal("macDelayDl"), cComponent::registerSignal("macDelayUl") };
simsignal_t LteHarqBufferRx::macThroughputSignal_[2] = { cComponent::registerSignal("macThroughputDl"), cComponent::registerSignal("macThroughputUl") };

LteHarqBufferRx::LteHarqBufferRx(unsigned int num, LteMacBase *owner, Binder *binder, MacNodeId srcId)
    : binder_(binder), macOwner_(owner), numHarqProcesses_(num), srcId_(srcId), processes_(num, nullptr), isMulticast_(false)
{
    initMacUe();

    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        processes_[i] = new LteHarqProcessRx(i, macOwner_, binder);
    }

    // Signals initialization: these are used to gather statistics
    if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB) {
        nodeB_ = macOwner_;
        dir = UL;
    }
    else { // this is a UE
        nodeB_ = getMacByMacNodeId(binder, macUe_->getMacCellId());
        dir = DL;
    }
}

LteHarqBufferRx::LteHarqBufferRx(Binder *binder, LteMacBase *owner, unsigned int num, MacNodeId srcId)
    : binder_(binder), macOwner_(owner), numHarqProcesses_(num), srcId_(srcId), processes_(num, nullptr), isMulticast_(false)
{
}

void LteHarqBufferRx::insertPdu(Codeword cw, inet::Packet *pkt)
{

    auto pdu = pkt->peekAtFront<LteMacPdu>();
    auto uInfo = pkt->getTag<UserControlInfo>();

    MacNodeId srcId = uInfo->getSourceId();
    if (macOwner_->isHarqReset(srcId)) {
        // if the HARQ processes have been aborted during this TTI (e.g., due to a D2D mode switch),
        // incoming packets should not be accepted
        delete pkt;
        return;
    }
    unsigned char acid = uInfo->getAcid();
    // TODO add codeword to insertPdu
    processes_[acid]->insertPdu(cw, pkt);
    // debug output
    EV << "H-ARQ RX: new PDU (id " << pdu->getId()
       << " ) inserted into process " << (int)acid << endl;
}

void LteHarqBufferRx::sendFeedback()
{
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw) {
            if (processes_[i]->isEvaluated(cw)) {
                auto pkt = processes_[i]->createFeedback(cw);
                auto hfb = pkt->peekAtFront<LteHarqFeedback>();

                // debug output:
                auto uInfo = pkt->getTag<UserControlInfo>();
                const char *r = hfb->getResult() ? "ACK" : "NACK";
                EV << "H-ARQ RX: feedback sent to TX process "
                   << (int)hfb->getAcid() << " Codeword  " << (int)cw
                   << " of node with id "
                   << uInfo->getDestId()
                   << " result: " << r << endl;

                macOwner_->takeObj(pkt);
                macOwner_->sendLowerPackets(pkt);
            }
        }
    }
}

unsigned int LteHarqBufferRx::purgeCorruptedPdus()
{
    unsigned int purged = 0;

    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw) {
            if (processes_[i]->getUnitStatus(cw) == RXHARQ_PDU_CORRUPTED) {
                EV << "LteHarqBufferRx::purgeCorruptedPdus - purged PDU with acid " << i << endl;
                // purge PDU
                processes_[i]->purgeCorruptedPdu(cw);
                processes_[i]->resetCodeword(cw);
                // increment purged PDUs counter
                ++purged;
            }
        }
    }
    return purged;
}

std::list<Packet *> LteHarqBufferRx::extractCorrectPdus()
{
    this->sendFeedback();
    std::list<Packet *> ret;
    unsigned char acid = 0;
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw) {
            if (processes_[i]->isCorrect(cw)) {
                auto pktTemp = processes_[i]->extractPdu(cw);
                auto temp = pktTemp->peekAtFront<LteMacPdu>();
                auto uInfo = pktTemp->getTag<UserControlInfo>();

                unsigned int size = pktTemp->getByteLength();

                // emit delay statistic
                macUe_emit(macDelaySignal_[dir], (NOW - pktTemp->getCreationTime()).dbl());

                // Calculate Throughput by sending the number of bits for this packet
                totalCellRcvdBytes_ += size;
                totalRcvdBytes_ += size;
                double den = (NOW - getSimulation()->getWarmupPeriod()).dbl();

                // emit throughput statistics
                if (den > 0) {
                    double tputSample = (double)totalRcvdBytes_ / den;
                    double cellTputSample = (double)totalCellRcvdBytes_ / den;

                    nodeB_->emit(macCellThroughputSignal_[dir], cellTputSample);
                    macUe_emit(macThroughputSignal_[dir], tputSample);
                }

                macOwner_->dropObj(pktTemp);
                ret.push_back(pktTemp);
                acid = i;

                EV << "LteHarqBufferRx::extractCorrectPdus H-ARQ RX: PDU (id " << ret.back()->getId()
                   << " ) extracted from process " << (int)acid
                   << " to be sent upper" << endl;
            }
        }
    }

    return ret;
}

UnitList LteHarqBufferRx::firstAvailable()
{
    UnitList ret;

    unsigned char acid = HARQ_NONE;
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        if (processes_[i]->isEmpty()) {
            acid = i;
            break;
        }
    }

    ret.first = acid;

    if (acid != HARQ_NONE) {
        // if there is any free process, return empty list
        ret.second = processes_[acid]->emptyUnitsIds();
    }

    return ret;
}

UnitList LteHarqBufferRx::getEmptyUnits(unsigned char acid)
{
    // TODO add multi CW check and reTx checks
    UnitList ret;
    ret.first = acid;
    ret.second = processes_[acid]->emptyUnitsIds();
    return ret;
}

RxBufferStatus LteHarqBufferRx::getBufferStatus()
{
    RxBufferStatus bs(numHarqProcesses_);
    unsigned int numHarqUnits = 0;
    for (unsigned int i = 0; i < numHarqProcesses_; i++) {
        numHarqUnits = (processes_)[i]->getNumHarqUnits();
        std::vector<RxUnitStatus> vus(numHarqUnits);
        vus = (processes_)[i]->getProcessStatus();
        bs[i] = vus;
    }
    return bs;
}

LteHarqBufferRx::~LteHarqBufferRx()
{
    for (auto* process : processes_)
        delete process;
    macOwner_ = nullptr;
}

// @author Alessandro Noferi

bool LteHarqBufferRx::isHarqBufferActive() const {
    for (const auto& process : processes_) {
        if (process->isHarqProcessActive()) {
            return true;
        }
    }
    return false;
}

} //namespace

