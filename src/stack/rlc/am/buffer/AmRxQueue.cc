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

#include "stack/rlc/am/buffer/AmRxQueue.h"

#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/mac/LteMacBase.h"
#include "stack/rlc/am/LteRlcAm.h"

namespace simu5g {

Define_Module(AmRxQueue);

using namespace inet;
using namespace omnetpp;

unsigned int AmRxQueue::totalCellRcvdBytes_ = 0;

simsignal_t AmRxQueue::rlcCellPacketLossSignal_[2] = { registerSignal("rlcCellPacketLossDl"), registerSignal("rlcCellPacketLossUl") };
simsignal_t AmRxQueue::rlcPacketLossSignal_[2] = { registerSignal("rlcPacketLossDl"), registerSignal("rlcPacketLossUl") };
simsignal_t AmRxQueue::rlcPduPacketLossSignal_[2] = { registerSignal("rlcPduPacketLossDl"), registerSignal("rlcPduPacketLossUl") };
simsignal_t AmRxQueue::rlcDelaySignal_[2] = { registerSignal("rlcDelayDl"), registerSignal("rlcDelayUl") };
simsignal_t AmRxQueue::rlcThroughputSignal_[2] = { registerSignal("rlcThroughputDl"), registerSignal("rlcThroughputUl") };
simsignal_t AmRxQueue::rlcPduDelaySignal_[2] = { registerSignal("rlcPduDelayDl"), registerSignal("rlcPduDelayUl") };
simsignal_t AmRxQueue::rlcPduThroughputSignal_[2] = { registerSignal("rlcPduThroughputDl"), registerSignal("rlcPduThroughputUl") };
simsignal_t AmRxQueue::rlcCellThroughputSignal_[2] = { registerSignal("rlcCellThroughputDl"), registerSignal("rlcCellThroughputUl") };

AmRxQueue::AmRxQueue() :
    // In order to create a back connection (AM CTRL), a flow control
    // info for sending control messages to the transmitting entity is required
    lastSentAck_(0),  timer_(this)
{
    rxWindowDesc_.firstSeqNum_ = 0;
    rxWindowDesc_.seqNum_ = 0;
    timer_.setTimerId(BUFFERSTATUS_T);
}

void AmRxQueue::initialize()
{
    // Loading parameters from NED
    rxWindowDesc_.windowSize_ = par("rxWindowSize");
    ackReportInterval_ = par("ackReportInterval");
    statusReportInterval_ = par("statusReportInterval");

    discarded_.resize(rxWindowDesc_.windowSize_);
    received_.resize(rxWindowDesc_.windowSize_);
    totalRcvdBytes_ = 0;

    binder_.reference(this, "binderModule", true);
    lteRlc_.reference(this, "amModule", true);

    // Statistics
    LteMacBase *mac = inet::getConnectedModule<LteMacBase>(getParentModule()->gate("RLC_to_MAC"), 0);

    if (mac->getNodeType() == ENODEB || mac->getNodeType() == GNODEB) {
        dir_ = UL;
    }
    else {
        dir_ = DL;
    }
}

void AmRxQueue::handleMessage(cMessage *msg)
{
    if (!(msg->isSelfMessage()))
        throw cRuntimeError("Unexpected message received from AmRxQueue");

    EV << NOW << "AmRxQueue::handleMessage timer event received, sending status report " << endl;

    // Signal timer event handled
    timer_.handle();

    // Send status report to the AM Tx entity
    sendStatusReport();

    // Reschedule the timer if there are PDUs in the buffer

    for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; i++) {
        if (pduBuffer_.get(i) != nullptr) {
            timer_.start(statusReportInterval_);
            break;
        }
    }

    delete msg;
}

Packet *AmRxQueue::defragmentFrames(std::deque<Packet *>& fragmentFrames)
{
    EV_DEBUG << "Defragmenting " << fragmentFrames.size() << " fragments.\n";
    auto defragmentedFrame = new Packet();
    defragmentedFrame->copyTags(*(fragmentFrames.at(0)));

    std::string defragmentedName(fragmentFrames.at(0)->getName());
    auto index = defragmentedName.find("-frag");
    if (index != std::string::npos)
        defragmentedFrame->setName(defragmentedName.substr(0, index).c_str());

    for (auto fragmentFrame : fragmentFrames) {
        fragmentFrame->popAtFront<LteRlcAmPdu>();
        defragmentedFrame->insertAtBack(fragmentFrame->peekData());
        delete fragmentFrame;
    }

    fragmentFrames.clear();

    EV_TRACE << "Created " << *defragmentedFrame << ".\n";

    return defragmentedFrame;
}

void AmRxQueue::discard(const int sn)
{
    int index = sn - rxWindowDesc_.firstSeqNum_;

    if ((index < 0) || (index >= rxWindowDesc_.windowSize_))
        throw cRuntimeError("AmRxQueue::discard PDU %d out of rx window ", sn);

    int discarded = 0;

    Direction dir = UNKNOWN_DIRECTION;

    MacNodeId dstId = NODEID_NONE, srcId = NODEID_NONE;

    for (int i = 0; i <= index; ++i) {
        discarded_.at(i) = true;

        if (pduBuffer_.get(i) != nullptr) {
            auto pkt = check_and_cast<inet::Packet *>(pduBuffer_.remove(i));
            auto pdu = pkt->peekAtFront<LteRlcAmPdu>();
            auto ci = pdu->getTag<FlowControlInfo>();
            dir = (Direction)ci->getDirection();
            dstId = ci->getDestId();
            srcId = ci->getSourceId();
            // Erase element from pendingPduBuffer_ (if it is within the buffer)
            auto it = std::find(pendingPduBuffer_.begin(), pendingPduBuffer_.end(), pkt);
            if (it != pendingPduBuffer_.end())
                pendingPduBuffer_.erase(it);

            delete pkt;
            ++discarded;
        }
        else
            throw cRuntimeError("AmRxQueue::discard PDU at position %d already discarded", i);
    }

    EV << NOW << " AmRxQueue::discard , discarded " << discarded << " PDUs " << endl;

    if (dir != UNKNOWN_DIRECTION) {
        // UE module
        cModule *ue = getRlcByMacNodeId(binder_, (dir == DL ? dstId : srcId), UM);
        if (ue != nullptr)
            ue->emit(rlcPacketLossSignal_[dir_], 1.0);

        // NODEB
        cModule *nodeb = getRlcByMacNodeId(binder_, (dir == DL ? srcId : dstId), UM);
        if (nodeb != nullptr)
            nodeb->emit(rlcCellPacketLossSignal_[dir_], 1.0);
    }
}

void AmRxQueue::enque(Packet *pkt)
{
    Enter_Method("enque()");

    take(pkt);

    auto pdu = pkt->peekAtFront<LteRlcAmPdu>();

    // Check if a control PDU has been received
    if (pdu->getAmType() != DATA) {
        if ((pdu->getAmType() == ACK)) {
            EV << NOW << " AmRxQueue::enque Received ACK message" << endl;
            // Forwarding ACK to associated transmitting entity
            lteRlc_->routeControlMessage(pkt);
        }
        else if ((pdu->getAmType() == MRW)) {
            EV << NOW << " AmRxQueue::enque MRW command [" <<
                pdu->getSnoMainPacket()
               << "] received for sequence number  " << pdu->getLastSn() << endl;
            // Move the receiver window
            moveRxWindow(pdu->getLastSn());

            // ACK this message
            auto pktAux = new Packet("rlcAmPdu (MRW ACK)");

            auto mrw = makeShared<LteRlcAmPdu>();
            mrw->setAmType(MRW_ACK);
            mrw->setSnoMainPacket(pdu->getSnoMainPacket());
            mrw->setSnoFragment(rxWindowDesc_.firstSeqNum_);
            // TODO: setting AM control byte size
            mrw->setChunkLength(B(RLC_HEADER_AM));
            pktAux->insertAtFront(mrw);
            // Set flow control info
            *pktAux->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;
            // Sending control PDU
            lteRlc_->bufferControlPdu(pktAux);
            EV << NOW << " AmRxQueue::enque sending MRW_ACK message " << pdu->getSnoMainPacket() << endl;

            delete pkt;
        }
        else if ((pdu->getAmType() == MRW_ACK)) {
            EV << NOW << " MRW ACK routing to TX entity " << endl;
            // Route message to reverse link TX entity
            lteRlc_->routeControlMessage(pkt);
        }
        else {
            throw cRuntimeError("RLC AM - Unknown status PDU");
        }
        // Control PDU (not a DATA PDU) - completely processed
        return;
    }

    // If the timer has not been enabled yet, start the timer to handle status report (ACK) messages
    if (timer_.idle()) {
        EV << NOW << " AmRxQueue::enque reporting timer was idle, will fire at " << NOW.dbl() + statusReportInterval_.dbl() << endl;
        timer_.start(statusReportInterval_);
    }
    else {
        EV << NOW << " AmRxQueue::enque reporting timer was already on, will fire at " << NOW.dbl() + timer_.remaining().dbl() << endl;
    }

    // Check if we need to extract FlowControlInfo for building up the matrix
    if (flowControlInfo_ == nullptr) {
        auto orig = pkt->getTag<FlowControlInfo>();
        // Make a copy of the original control info
        flowControlInfo_ = orig->dup();
        // Swap source and destination fields
        flowControlInfo_->setSourceId(orig->getDestId());
        flowControlInfo_->setSrcAddr(orig->getDstAddr());
        flowControlInfo_->setTypeOfService(orig->getTypeOfService());
        flowControlInfo_->setDestId(orig->getSourceId());
        flowControlInfo_->setDstAddr(orig->getSrcAddr());
        // Set up other fields
        flowControlInfo_->setDirection((orig->getDirection() == DL) ? UL : DL);
    }

    // Get the RLC PDU Transmission sequence number
    int tsn = pdu->getSnoFragment();

    // Get the index of the PDU buffer
    int index = tsn - rxWindowDesc_.firstSeqNum_;

    if (index < 0) {
        EV << NOW << " AmRxQueue::enque the received PDU with " << index << " was already buffered and discarded by MRW" << endl;
        delete pkt;
    }
    else if ((index >= rxWindowDesc_.windowSize_)) {
        // The received PDU is out of the window
        throw cRuntimeError("AmRxQueue::enque(): received PDU with position %d out of the window of size %d", index, rxWindowDesc_.windowSize_);
    }
    else {
        // Check if the tsn is the next expected TSN.
        if (tsn == rxWindowDesc_.seqNum_) {
            rxWindowDesc_.seqNum_++;

            EV << NOW << "AmRxQueue::enque DATA PDU received at index [" << index << "] with fragment number [" << tsn << "] in sequence " << endl;
        }
        else {
            rxWindowDesc_.seqNum_ = tsn + 1;

            EV << NOW << "AmRxQueue::enque DATA PDU received at index [" << index << "] with fragment number ["
               << tsn << "] out of sequence, sending status report " << endl;
            sendStatusReport();
        }

        // Check if the PDU has already been received

        if (received_.at(index) == true) {
            EV << NOW << " AmRxQueue::enque the received PDU has index " << index << " which points to an already busy location" << endl;

            // Check if the received PDU points
            // to the same data structure of the PDU
            // stored in the buffer

            auto pktAux = check_and_cast<Packet *>(pduBuffer_.get(index));
            auto bufferedpdu = pktAux->peekAtFront<LteRlcAmPdu>();

            if (bufferedpdu->getSnoMainPacket() == pdu->getSnoMainPacket()) {
                // Drop the incoming RLC PDU
                EV << NOW << " AmRxQueue::enque the received PDU with " << index << " was already buffered " << endl;
                delete pkt;
            }
            else {
                throw cRuntimeError("AmRxQueue::enque(): the received PDU at position %d"
                                    "main SDU %d overlaps with an old one, main SDU %d", index, pdu->getSnoMainPacket(),
                        bufferedpdu->getSnoMainPacket());
            }
        }
        else {
            // Buffer the PDU
            pduBuffer_.addAt(index, pkt);
            received_.at(index) = true;
            // Check if this PDU forms a complete SDU
            checkCompleteSdu(index);
        }
    }
}

void AmRxQueue::passUp(const int index)
{
    Enter_Method("passUp");

    Packet *pkt = nullptr;

    auto header = check_and_cast<Packet *>(pduBuffer_.get(index))->peekAtFront<LteRlcAmPdu>();
    if (!header->isWhole()) {
        // assemble frame
        std::deque<Packet *> frameBuff;
        const auto pkId = header->getSnoMainPacket();

        // handle special case: some fragments have already been moved out of the receive window and
        // are available in the pendingPduBuffer
        if (index == 0 && !header->isFirst()) {
            for (auto& p: pendingPduBuffer_) {
                auto frgId = p->peekAtFront<LteRlcAmPdu>()->getSnoMainPacket();
                if (frgId != pkId) {
                    throw cRuntimeError("AmRxQueue::passUp(): fragment buffer has fragments for SDU %d while trying to pass up %d", frgId, pkId);
                }
                frameBuff.push_back(p);
            }
            pendingPduBuffer_.clear();
        }

        int auxIndex = index;

        for (int i = 0; i < pduBuffer_.size() && frameBuff.size() < header->getTotalFragments(); i++) {
            auto headerAux = check_and_cast<Packet *>(pduBuffer_.get(auxIndex))->peekAtFront<LteRlcAmPdu>();
            // duplicate buffered PDU. We cannot detach it from the receiver window until a move Rx command is executed.
            if (pkId == headerAux->getSnoMainPacket())
                frameBuff.push_back(check_and_cast<Packet *>(pduBuffer_.get(auxIndex))->dup());
            auxIndex++;
            if (auxIndex >= pduBuffer_.size())
                auxIndex = 0;
        }

        // now all fragments (PDUs) are available and the SDU can be defragmented
        pkt = defragmentFrames(frameBuff);
    }
    else {
        pkt = (check_and_cast<Packet *>(pduBuffer_.get(index)))->dup();
        pkt->removeAtFront<LteRlcAmPdu>();
    }

    pkt->trim();

    auto sdu = pkt->popAtFront<LteRlcAmSdu>();

    EV << NOW << " AmRxQueue::passUp passing up SDU[" << sdu->getSnoMainPacket() << "] referenced by PDU at position " << index << endl;

    auto ci = pkt->getTag<FlowControlInfo>();

    Direction dir = (Direction)ci->getDirection();
    MacNodeId dstId = ci->getDestId();
    MacNodeId srcId = ci->getSourceId();
    cModule *nodeb = nullptr;
    cModule *ue = nullptr;
    double delay = (NOW - pkt->getCreationTime()).dbl();

    if (dir == DL) {
        nodeb = getRlcByMacNodeId(binder_, srcId, UM);
        ue = getRlcByMacNodeId(binder_, dstId, UM);
    }
    else { // dir == one of UL, D2D, D2D_MULTI
        nodeb = getRlcByMacNodeId(binder_, dstId, UM);
        ue = getRlcByMacNodeId(binder_, srcId, UM);
    }

    totalRcvdBytes_ += pkt->getByteLength();
    totalCellRcvdBytes_ += pkt->getByteLength();
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double cellTputSample = (double)totalCellRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    if (nodeb != nullptr) {
        nodeb->emit(rlcCellThroughputSignal_[dir_], cellTputSample);
        nodeb->emit(rlcCellPacketLossSignal_[dir_], 0.0);
    }
    if (ue != nullptr) {
        ue->emit(rlcThroughputSignal_[dir_], tputSample);
        ue->emit(rlcDelaySignal_[dir_], delay);
        ue->emit(rlcPacketLossSignal_[dir_], 0.0);
    }

    // Get the SDU and pass it to the upper layers - PDU // SDU // PDCPPDU
    lteRlc_->sendDefragmented(pkt);

    // send buffer status report
    sendStatusReport();
}

void AmRxQueue::checkCompleteSdu(const int index)
{

    auto pkt = check_and_cast<Packet *>(pduBuffer_.get(index));
    auto pdu = pkt->peekAtFront<LteRlcAmPdu>();

    int incomingSdu = pdu->getSnoMainPacket();

    EV << NOW << " AmRxQueue::checkCompleteSdu at position " << index << " for SDU number " << incomingSdu << endl;

    if (firstSdu_ == -1) {
        // this is the first PDU in the Rx window of a new SDU
        firstSdu_ = incomingSdu;
    }

    // complete SDU found flag
    bool complete = false;
    // backward search OK flag
    bool bComplete = false;

    Ptr<LteRlcAmPdu> tempPdu = nullptr;
    int tempSdu = -1;
    // Index of the first PDU related to the SDU
    int firstIndex = -1;
    // If the RLC PDU is whole, pass the RLC PDU to the upper layer
    if (pdu->isWhole()) {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found (PDU at " << index << " was whole)" << endl;
        passUp(index);
        return;
    }
    else {
        if (!pdu->isFirst()) {
            if ((index) == 0) {
                // We are at the beginning of the buffer and PDU is in the middle of its SDU sequence.
                // Since the window has been moved, previous PDUs of this SDU have been correctly received
                // and are available in the pending PDUs buffer.
                if (firstSdu_ == incomingSdu) {
                    firstIndex = index;
                    bComplete = true;
                }
                else
                    throw cRuntimeError("AmRxQueue::checkCompleteSdu(): first SDU error : %d", firstSdu_);
            }
            else {
                // check for previous PDUs
                for (int i = index - 1; i >= 0; i--) {
                    if (received_.at(i) == false) {
                        // There is NO RLC PDU in this position
                        // The SDU is not complete
                        EV << NOW << " AmRxQueue::checkCompleteSdu: SDU cannot be reconstructed, no PDU received at positions earlier than " << i << endl;
                        return;
                    }
                    else {
                        auto tempPkt = check_and_cast<Packet *>(pduBuffer_.get(i));

                        tempPdu = constPtrCast<LteRlcAmPdu>(tempPkt->peekAtFront<LteRlcAmPdu>());
                        tempSdu = tempPdu->getSnoMainPacket();

                        if (tempSdu != incomingSdu)
                            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): backward search: fragmentation error: the receiver buffer contains parts of different SDUs, PDU seqnum %d", pdu->getSnoFragment());

                        if (tempPdu->isFirst()) {
                            // found starting PDU
                            firstIndex = i;
                            bComplete = true;
                            break;
                        }
                        else if (tempPdu->isLast() || tempPdu->isWhole()) {
                            auto auxPkt = check_and_cast<Packet *>(pduBuffer_.get(i + 1));
                            auto aux = auxPkt->peekAtFront<LteRlcAmPdu>();
                            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): backward search: sequence error, found last or whole PDU [%d] preceding a middle one [%d], belonging to SDU [%d], current SDU is [%d]", tempPdu->getSnoFragment(),
                                    aux->getSnoFragment(), aux->getSnoMainPacket(), tempSdu);
                        }
                    }
                }
            }
        }
        else {
            // since PDU is the first one, backward search is automatically completed by definition
            bComplete = true;
            firstIndex = index;
        }
    }
    if (!bComplete) {
        EV << NOW
           << " AmRxQueue::checkCompleteSdu - SDU cannot be reconstructed, backward search didn't find any predecessors to PDU at "
           << index << endl;
        return;
    }
    // search ended with current PDU, the whole SDU can be reconstructed.
    if (pdu->isLast()) {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found, backward search was successful, and current was last of its SDU"
                     " passing up " << firstIndex << endl;
        passUp(firstIndex);
        return;
    }

    EV << NOW << " AmRxQueue::checkCompleteSdu initiating forward search, starting from position " << index + 1 << endl;
    // Go forward looking for remaining PDUs

    for (int i = index + 1; i < (rxWindowDesc_.windowSize_); ++i) {
        if (received_.at(i) == false) {
            EV << NOW << " AmRxQueue::checkCompleteSdu forward search failed, no PDU at position " << i << " corresponding to"
                                                                                                           " SN  " << i + rxWindowDesc_.firstSeqNum_ << endl;

            // There is NO RLC PDU in this position
            // The SDU is not complete
            return;
        }
        else {
            auto temPkt = check_and_cast<Packet *>(pduBuffer_.get(i));
            tempPdu = constPtrCast<LteRlcAmPdu>(temPkt->peekAtFront<LteRlcAmPdu>());
            tempSdu = tempPdu->getSnoMainPacket();
            if (tempSdu != incomingSdu)
                throw cRuntimeError("AmRxQueue::checkCompleteSdu(): SDU numbers differ from position %d to %d : former SDU %d second %d", i, i - 1, incomingSdu, tempSdu);
        }
        if (tempPdu->isLast()) {
            complete = true;
            EV << NOW << " AmRxQueue::checkCompleteSdu: forward search successful, last PDU found at position "
               << i << endl;

            break;
        }
        else if (tempPdu->isFirst() || tempPdu->isWhole()) {
            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): forward search: PDU sequencer error ");
            break;
        }
    }

    // check if all PDUs for this SDU have been received
    if (complete) {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found after forward search, passing up "
           << firstIndex << endl;
        passUp(firstIndex);
        return;
    }
}

void AmRxQueue::sendStatusReport()
{
    Enter_Method("sendStatusReport()");
    EV << NOW << " AmRxQueue::sendStatusReport " << endl;

    // Check if the prohibit status report has been set.

    if ((NOW.dbl() - lastSentAck_.dbl()) < ackReportInterval_.dbl()) {
        EV << NOW
           << " AmRxQueue::sendStatusReport , minimum interval not reached "
           << ackReportInterval_.dbl() << endl;

        return;
    }

    // Compute cumulative ACK
    int cumulative = 0;
    bool hole = !received_.at(0);
    std::vector<bool> bitmap;

    for (int i = 0; i < rxWindowDesc_.windowSize_; ++i) {
        if ((received_.at(i) == true) && !hole) {
            cumulative++;
        }
        else if ((cumulative > 0) || hole) {
            hole = true;
            bitmap.push_back(received_.at(i));
        }
    }

    // The BitMap :
    // Starting from the cumulative ACK the next received PDU
    // are stored.

    EV << NOW << " AmRxQueue::sendStatusReport : cumulative ACK value "
       << cumulative << " bitmap length " << bitmap.size() << endl;

    if ((cumulative > 0) || bitmap.size() > 0) {
        // create a new RLC PDU
        auto pktPdu = new Packet("rlcAmPdu (Cum. ACK)");
        auto pdu = makeShared<LteRlcAmPdu>();

        // set RLC type descriptor
        pdu->setAmType(ACK);

        int lastSn = rxWindowDesc_.firstSeqNum_ + cumulative - 1;

        pdu->setLastSn(lastSn);
        // set bitmap
        EV << NOW << " AmRxQueue::sendStatusReport : sending the cumulative ACK for  "
           << lastSn << endl;

        // Note that, FSN could be out of the receiver windows, in this case
        // no ACK BITMAP message is sent.
        if (cumulative < rxWindowDesc_.windowSize_) {
            pdu->setBitmapVec(bitmap);
            // Start the BITMAP ACK report at the end of the cumulative ACK
            int fsn = rxWindowDesc_.firstSeqNum_ + cumulative;
            // We set the first sequence number of the BITMAP starting
            // from the end of the cumulative ACK message.
            pdu->setFirstSn(fsn);
        }
        // todo setting byte size
        pdu->setChunkLength(B(RLC_HEADER_AM));
        // set flowcontrolinfo
        *pktPdu->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;
        // sending control PDU
        pktPdu->insertAtFront(pdu);
        lteRlc_->bufferControlPdu(pktPdu);
        lastSentAck_ = NOW;
    }
    else {
        EV << NOW
           << " AmRxQueue::sendStatusReport : NOT sending the cumulative ACK for  "
           << cumulative << endl;
    }
}

int AmRxQueue::computeWindowShift() const
{
    EV << NOW << "AmRxQueue::computeWindowShift" << endl;
    int shift = 0;
    for ( int i = 0; i < rxWindowDesc_.windowSize_; ++i) {
        if (received_.at(i) == true || discarded_.at(i) == true) {
            ++shift;
        }
        else {
            break;
        }
    }
    return shift;
}

void AmRxQueue::moveRxWindow(const int seqNum)
{
    EV << NOW << " AmRxQueue::moveRxWindow moving forth to match first seqnum " << seqNum << endl;

    int pos = seqNum - rxWindowDesc_.firstSeqNum_;

    if (pos <= 0)
        return;                   // ignore the shift, it is ineffective.

    if (pos > rxWindowDesc_.windowSize_)
        throw cRuntimeError("AmRxQueue::moveRxWindow(): positions %d win size %d, seq num %d", pos, rxWindowDesc_.windowSize_, seqNum);

    // Shift the window and check if a complete SDU is shifted or if
    // part of an SDU is still in the buffer

    int currentSdu = firstSdu_;

    EV << NOW << " AmRxQueue::moveRxWindow current SDU is " << firstSdu_ << endl;

    for ( int i = 0; i < pos; ++i) {
        if (pduBuffer_.get(i) != nullptr) {

            auto pktPdu = check_and_cast<Packet *>(pduBuffer_.remove(i));
            auto pdu = pktPdu->peekAtFront<LteRlcAmPdu>();
            currentSdu = (pdu->getSnoMainPacket());

            if (pdu->isLast() || pdu->isWhole()) {
                // Reset the last PDU seen.
                currentSdu = -1;
                // Remove all PDUs from pending PDU buffer.
                for (auto& p: pendingPduBuffer_) {
                    delete p;
                }
                pendingPduBuffer_.clear();
                delete pktPdu;
            }
            else {
                // temporarily store PDU until the window is shifted to last PDU of its SDU
                pendingPduBuffer_.push_back(pktPdu);
            }
        }
        else {
            currentSdu = -1;
        }
    }

    for ( int i = pos; i < rxWindowDesc_.windowSize_; ++i) {
        if (pduBuffer_.get(i) != nullptr) {
            pduBuffer_.addAt(i - pos, pduBuffer_.remove(i));
        }
        else {
            pduBuffer_.remove(i);
        }
        received_.at(i - pos) = received_.at(i);
        discarded_.at(i - pos) = discarded_.at(i);
        received_.at(i) = false;
        discarded_.at(i) = false;
    }

    rxWindowDesc_.firstSeqNum_ += pos;

    EV << NOW << " AmRxQueue::moveRxWindow first sequence number updated to "
       << rxWindowDesc_.firstSeqNum_ << endl;

    firstSdu_ = currentSdu;
    EV << NOW << " AmRxQueue::moveRxWindow current SDU updated to "
       << firstSdu_ << endl;
}

AmRxQueue::~AmRxQueue()
{
    // clear buffered PDUs
    for (int i = 0; i < pduBuffer_.size(); i++) {
        if (pduBuffer_.get(i) != nullptr) {
            auto pktPdu = check_and_cast<Packet *>(pduBuffer_.remove(i));
            delete pktPdu;
        }
    }

    // Remove all PDUs from the pending PDU buffer.
    for (auto& p: pendingPduBuffer_) {
        delete p;
    }
    pendingPduBuffer_.clear();

    delete flowControlInfo_;
}

} //namespace

