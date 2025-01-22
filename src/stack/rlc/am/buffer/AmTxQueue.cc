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

#include "stack/rlc/am/buffer/AmTxQueue.h"
#include "stack/rlc/am/LteRlcAm.h"
#include "stack/mac/LteMacBase.h"

namespace simu5g {

Define_Module(AmTxQueue);

AmTxQueue::AmTxQueue() :
     pduTimer_(this), mrwTimer_(this), bufferStatusTimer_(this)
{

    // Initialize timer IDs
    pduTimer_.setTimerId(PDU_T);
    mrwTimer_.setTimerId(MRW_T);
    bufferStatusTimer_.setTimerId(BUFFER_T);
}

void AmTxQueue::initialize()
{
    // Initialize all parameters from NED.
    maxRtx_ = par("maxRtx");
    fragDesc_.fragUnit_ = par("fragmentSize");
    pduRtxTimeout_ = par("pduRtxTimeout");
    ctrlPduRtxTimeout_ = par("ctrlPduRtxTimeout");
    bufferStatusTimeout_ = par("bufferStatusTimeout");
    txWindowDesc_.windowSize_ = par("txWindowSize");
    // Resize status vectors
    received_.resize(txWindowDesc_.windowSize_, false);
    discarded_.resize(txWindowDesc_.windowSize_ + 1, false);

    // Reference to corresponding RLC AM module
    lteRlc_.reference(this, "amModule", true);
}

AmTxQueue::~AmTxQueue()
{
    // Clear buffered PDUs
    while (!pduBuffer_.isEmpty()) {
        auto pktPdu = check_and_cast<Packet *>(pduBuffer_.pop());
        delete pktPdu;
    }

    // Clear buffered SDUs
    while (!sduQueue_.isEmpty()) {
        auto pktSdu = check_and_cast<Packet *>(sduQueue_.pop());
        delete pktSdu;
    }

    // Clear buffered PDU fragments
    for (int i = 0; i < pduRtxQueue_.size(); i++) {
        if (pduRtxQueue_.get(i) != nullptr) {
            auto pktPdu = check_and_cast<Packet *>(pduRtxQueue_.remove(i));
            delete pktPdu;
        }
    }

    // Clear retransmission buffer
    for (int i = 0; i < mrwRtxQueue_.size(); i++) {
        if (mrwRtxQueue_.get(i) != nullptr) {
            auto pktPdu = check_and_cast<Packet *>(mrwRtxQueue_.remove(i));
            delete pktPdu;
        }
    }

    delete lteInfo_;
    delete currentSdu_;
}

void AmTxQueue::enque(Packet *pkt)
{
    EV << NOW << " AmTxQueue::enque - inserting new SDU  " << endl;
    // Buffer the SDU
    auto sdu = pkt->peekAtFront<LteRlcAmSdu>();
    sduQueue_.insert(pkt);

    // Check if there are waiting SDUs
    if (currentSdu_ == nullptr) {
        // Add AM-PDU to the transmission buffer
        if (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_ < txWindowDesc_.windowSize_) {
            // RLC AM PDUs can be added to the buffer
            addPdus();
        }
    }
}

std::deque<Packet *> *AmTxQueue::fragmentFrame(Packet *frame, std::deque<int>& windowsIndex, RlcFragDesc rlcFragDesc)
{

    EV_DEBUG << "Fragmenting " << *frame << " into " << rlcFragDesc.totalFragments_ << " fragments.\n";
    B offset = B(0);
    std::deque<Packet *> *fragments = new std::deque<Packet *>();
    const auto& frameHeader = frame->peekAtFront<LteRlcAmSdu>();
    windowsIndex.clear();
    RlcWindowDesc tmp = txWindowDesc_;
    B fragUnit = B(rlcFragDesc.fragUnit_);

    for (size_t i = 0; i < rlcFragDesc.totalFragments_; i++) {
        std::string name = std::string(frame->getName()) + "-frag" + std::to_string(i);
        auto fragment = new Packet(name.c_str());
        // Length is equal to fragmentation unit except for the last fragment
        B length = (i == rlcFragDesc.totalFragments_ - 1) ? B(frame->getTotalLength()) - offset : fragUnit;
        fragment->insertAtBack(frame->peekDataAt(offset, length));
        offset += length;
        // fragment->insertAtFront(frameHeader);
        auto pdu = makeShared<LteRlcAmPdu>();
        // Set RLC type descriptor
        pdu->setAmType(DATA);
        // Set fragmentation info
        pdu->setTotalFragments(rlcFragDesc.totalFragments_);
        pdu->setSnoFragment(tmp.seqNum_);
        pdu->setFirstSn(rlcFragDesc.firstSn_);
        pdu->setLastSn(rlcFragDesc.firstSn_ + rlcFragDesc.totalFragments_ - 1);
        pdu->setSnoMainPacket(frameHeader->getSnoMainPacket());
        pdu->setTxNumber(0);
        fragment->insertAtFront(pdu);
        EV_TRACE << "Created " << *fragment << " fragment.\n";
        // Prepare list of tx window indices
        int txWindowIndex = tmp.seqNum_ - tmp.firstSeqNum_;
        if (txWindowIndex >= 200)
            throw cRuntimeError("Illegal index");
        windowsIndex.push_back(txWindowIndex);
        fragment->copyTags(*frame);
        fragments->push_back(fragment);
        tmp.seqNum_++;
    }
    delete frame;
    EV_TRACE << "Created " << fragments->size() << " fragments.\n";
    return fragments;
}

void AmTxQueue::addPdus()
{
    Enter_Method("addPdus()");

    // Add PDUs to the AM transmission buffer until the transmission
    // window is full or until the SDU buffer is empty
    unsigned int addedPdus = 0;

    while ((txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_) < txWindowDesc_.windowSize_) {
        if (currentSdu_ == nullptr && sduQueue_.isEmpty() && (fragmentList_ == nullptr || fragmentList_->size() == 0)) {
            // No data to send
            EV << NOW << " AmTxQueue::addPdus - No data to send " << endl;
            break;
        }

        // Check if we can start to fragment a new SDU
        if (currentSdu_ == nullptr) {
            EV << NOW << " AmTxQueue::addPdus - No pending SDU has been found" << endl;
            // Get the first available SDU (buffer has already been checked to be non-empty)
            auto pkt = check_and_cast<Packet *>(sduQueue_.pop());
            auto header = pkt->peekAtFront<LteRlcAmSdu>();

            int nrFragments = ceil((double)pkt->getByteLength() / (double)fragDesc_.fragUnit_);

            if (txWindowDesc_.seqNum_ + nrFragments < txWindowDesc_.firstSeqNum_ + txWindowDesc_.windowSize_) {
                fragDesc_.startFragmentation(pkt->getByteLength(), txWindowDesc_.seqNum_);

                currentSdu_ = pkt;
                fragmentList_ = fragmentFrame(pkt->dup(), txWindowIndexList_, fragDesc_);

                // Starting Fragmentation
                EV << NOW << " AmTxQueue::addPdus current SDU size "
                   << currentSdu_->getByteLength() << " will be fragmented in "
                   << fragDesc_.totalFragments_ << " PDUs, each  of size "
                   << fragDesc_.fragUnit_ << endl;

                if (lteInfo_ != nullptr)
                    delete lteInfo_;

                lteInfo_ = currentSdu_->getTag<FlowControlInfo>()->dup();
            }
            else {
                // EV << NOW << " AmTxQueue::addPdus   cannot fragment new SDU since fragments do not fit - tx window is full" << std::endl;
            }
        }

        if (fragmentList_ == nullptr) {
            // Nothing more to do
            break;
        }

        EV << NOW << " AmTxQueue::addPdus - prepare new RLC PDU" << endl;

        auto pdu = fragmentList_->front();
        fragmentList_->pop_front();
        auto txWindowIndex = txWindowIndexList_.front();
        txWindowIndexList_.pop_front();

        auto pduHeader = pdu->peekAtFront<LteRlcAmPdu>();

        if (pduHeader->getSnoFragment() != txWindowDesc_.seqNum_)
            throw cRuntimeError("PDU sequence numbers must be checked");

        if (pduRtxQueue_.get(txWindowIndex) == nullptr) {
            // Store a copy of the current PDU
            auto pduCopy = pdu->dup();
            //pduCopy->setControlInfo(lteInfo->dup());
            pduRtxQueue_.addAt(txWindowIndex, pduCopy);

            if (txWindowIndex >= 200)
                throw cRuntimeError("Illegal index");

            if (received_.at(txWindowIndex) || discarded_.at(txWindowIndex)) {
                delete pdu;
                throw cRuntimeError("AmTxQueue::addPdus(): trying to add a PDU to a position marked received [%d] discarded [%d]",
                        (int)(received_.at(txWindowIndex)), (int)(discarded_.at(txWindowIndex)));
            }
        }
        else {
            delete pdu;
            throw cRuntimeError("AmTxQueue::addPdus(): trying to add a PDU to a busy position [%d]", txWindowIndex);
        }
        // Start the PDU timer
        pduTimer_.add(pduRtxTimeout_, txWindowDesc_.seqNum_);

        // Update number of added PDUs for the current SDU and check if all fragments have been transmitted
        if (fragDesc_.addFragment() || (fragmentList_ && fragmentList_->empty())) {
            delete fragmentList_;
            fragmentList_ = nullptr;
            txWindowIndexList_.clear();
            fragDesc_.resetFragmentation();
            delete currentSdu_;
            currentSdu_ = nullptr;
        }
        // Update Sequence Number
        txWindowDesc_.seqNum_++;
        addedPdus++;

        // Activate buffer checking timer
        if (bufferStatusTimer_.busy() == false) {
            bufferStatusTimer_.start(bufferStatusTimeout_);
        }

        // Buffer (and send down) the PDU
        bufferPdu(pdu);
    }
    ASSERT(fragmentList_ == nullptr);
    ASSERT(txWindowIndexList_.empty());
    EV << NOW << " AmTxQueue::addPdus - added " << addedPdus << " PDUs" << endl;
}

void AmTxQueue::discard(const int seqNum)
{
    int txWindowIndex = seqNum - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxQueue::discard sequence number [" << seqNum
       << "] window index [" << txWindowIndex << "]" << endl;

    if ((txWindowIndex < 0) || (txWindowIndex >= txWindowDesc_.windowSize_)) {
        throw cRuntimeError(" AmTxQueue::discard(): requested to discard an out of window PDU :"
                            " sequence number %d , window first sequence is %d",
                seqNum, txWindowDesc_.firstSeqNum_);
    }

    if (discarded_.at(txWindowIndex) == true) {
        EV << " AmTxQueue::discard requested to discard an already discarded PDU :"
              " sequence number" << seqNum << " , window first sequence is " << txWindowDesc_.firstSeqNum_ << endl;
    }
    else {
        // Mark current PDU for discard
        discarded_.at(txWindowIndex) = true;
    }

    auto pkt = check_and_cast<Packet *>(pduRtxQueue_.get(txWindowIndex));
    auto pdu = pkt->peekAtFront<LteRlcAmPdu>();

    if (pduTimer_.busy(seqNum))
        pduTimer_.remove(seqNum);

    // Check forward in the buffer if there are other PDUs related to the same SDU
    for (int i = (txWindowIndex + 1);
         i < (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); ++i)
    {
        if (pduRtxQueue_.get(i) != nullptr) {
            auto nextPdu = check_and_cast<Packet *>(pduRtxQueue_.get(i))->peekAtFront<LteRlcAmPdu>();
            if (pdu->getSnoMainPacket() == nextPdu->getSnoMainPacket()) {
                // Mark the PDU to be discarded
                if (!discarded_.at(i)) {
                    discarded_.at(i) = true;
                    // Stop the timer
                    if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                        pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                }
            }
            else {
                // PDU belonging to different SDUs found. Stopping forward search
                break;
            }
        }
        else
            break; // Last PDU in buffer found, stopping forward search
    }
    // Check backward in the buffer if there are other PDUs related to the same SDU
    for (int i = txWindowIndex - 1; i >= 0; i--) {
        if (pduRtxQueue_.get(i) == nullptr)
            throw cRuntimeError("AmTxBuffer::discard(): trying to get access to missing PDU %d", i);

        auto nextPdu = check_and_cast<Packet *>(pduRtxQueue_.get(i))->peekAtFront<LteRlcAmPdu>();

        if (pdu->getSnoMainPacket() == nextPdu->getSnoMainPacket()) {
            if (!discarded_.at(i)) {
                // Mark the PDU to be discarded
                discarded_.at(i) = true;
            }
            // Stop the timer
            if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
        }
        else {
            break;
        }
    }
    // Check if a move receiver window command can be issued.
    checkForMrw();
}

void AmTxQueue::checkForMrw()
{
    EV << NOW << " AmTxQueue::checkForMrw " << endl;

    // If there is a discarded RLC PDU at the beginning of the buffer, try
    // to move the transmitter window
    int lastPdu = 0;
    bool toMove = false;

    for (int i = 0; i < (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); ++i) {
        if ((discarded_.at(i) == true) || (received_.at(i) == true)) {
            lastPdu = i;
            toMove = true;
        }
        else {
            break;
        }
    }

    if (toMove) {
        int lastSn = txWindowDesc_.firstSeqNum_ + lastPdu;

        EV << NOW << " AmTxQueue::checkForMrw  detected a shift from " << lastSn << endl;

        sendMrw(lastSn + 1);
    }
}

void AmTxQueue::moveTxWindow(const int seqNum)
{
    int pos = seqNum - txWindowDesc_.firstSeqNum_;

    // ignore the shift; it is ineffective.
    if (pos <= 0)
        return;

    // Shift the tx window; pos is the location after the last PDU to be removed (pointing to the new firstSeqNum_).

    EV << NOW << " AmTxQueue::moveTxWindow sequence number " << seqNum
       << " corresponding index " << pos << endl;

    // Delete both discarded and received RLC PDUs
    for (int i = 0; i < pos; ++i) {
        if (pduRtxQueue_.get(i) != nullptr) {
            EV << NOW << " AmTxQueue::moveTxWindow deleting PDU ["
               << i + txWindowDesc_.firstSeqNum_
               << "] corresponding index " << i << endl;

            auto pdu = check_and_cast<Packet *>(pduRtxQueue_.remove(i));
            delete pdu;

            // Stop the rtx timer event
            if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_)) {
                pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                EV << NOW << " AmTxQueue::moveTxWindow canceling PDU timer ["
                   << i + txWindowDesc_.firstSeqNum_
                   << "] corresponding index " << i << endl;
            }
            received_.at(i) = false;
            discarded_.at(i) = false;
        }
        else
            throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered empty PDU at location %d, shift position %d", i, pos);
    }

    for (int i = pos; i < (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); ++i) {
        if (pduRtxQueue_.get(i) != nullptr) {
            auto pdu = check_and_cast<Packet *>(pduRtxQueue_.remove(i));
            pduRtxQueue_.addAt(i - pos, pdu);

            EV << NOW << " AmTxQueue::moveTxWindow  PDU ["
               << i + txWindowDesc_.firstSeqNum_
               << "] corresponding index " << i
               << " being moved at position " << i - pos << endl;
        }
        else {
            throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered empty PDU at location %d, shift position %d", i, pos);
        }

        EV << NOW << " AmTxQueue::moveTxWindow  PDU ["
           << i + txWindowDesc_.firstSeqNum_
           << "] corresponding new index " << i - pos
           << " marked as received [" << (received_.at(i - pos))
           << "] , discarded [" << (discarded_.at(i - pos)) << "]" << endl;

        received_.at(i - pos) = received_.at(i);
        discarded_.at(i - pos) = discarded_.at(i);

        received_.at(i) = false;
        discarded_.at(i) = false;

        EV << NOW << " AmTxQueue::moveTxWindow  location [" << i
           << "] corresponding new PDU "
           << i + pos + txWindowDesc_.firstSeqNum_
           << " marked as received [" << (received_.at(i))
           << "] , discarded [" << (discarded_.at(i)) << "]" << endl;
    }

    // cleanup
    for (int i = (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); i < txWindowDesc_.windowSize_; ++i) {
        if (pduRtxQueue_.get(i) != nullptr)
            throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered busy PDU at location %d, shift position %d", i, pos);

        EV << NOW << " AmTxQueue::moveTxWindow  empty location [" << i
           << "] marked as received [" << (received_.at(i))
           << "] , discarded [" << (discarded_.at(i)) << "]" << endl;

        received_.at(i) = false;
        discarded_.at(i) = false;
    }

    txWindowDesc_.firstSeqNum_ += pos;

    EV << NOW << " AmTxQueue::moveTxWindow completed. First sequence number "
       << txWindowDesc_.firstSeqNum_ << " current sequence number "
       << txWindowDesc_.seqNum_ << " cleaned up locations from "
       << txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_ << " to "
       << txWindowDesc_.windowSize_ - 1 << endl;

    // Try to add more PDUs to the buffer
    addPdus();
}

void AmTxQueue::sendMrw(const int seqNum)
{
    EV << NOW << " AmTxQueue::sendMrw sending MRW PDU [" << mrwDesc_.mrwSeqNum_
       << "] for sequence number " << seqNum << endl;

    // create a new RLC PDU
    auto pktPdu = new Packet("rlcAmPdu (MRW)");
    auto pdu = makeShared<LteRlcAmPdu>();
    // set RLC type descriptor
    pdu->setAmType(MRW);
    // set fragmentation info
    pdu->setTotalFragments(1);
    pdu->setSnoFragment(mrwDesc_.mrwSeqNum_);
    pdu->setSnoMainPacket(mrwDesc_.mrwSeqNum_);
    // set fragment size
    pdu->setChunkLength(B(fragDesc_.fragUnit_));
    // prepare MRW control data
    pdu->setFirstSn(0);
    pdu->setLastSn(seqNum);
    // set control info
    pktPdu->insertAtFront(pdu);
    *(pktPdu->addTagIfAbsent<FlowControlInfo>()) = *lteInfo_;

    auto pduCopy = pktPdu->dup();
    // save copy for retransmission
    mrwRtxQueue_.addAt(mrwDesc_.mrwSeqNum_, pduCopy);
    // update MRW descriptor
    mrwDesc_.lastMrw_ = mrwDesc_.mrwSeqNum_;
    // Start a timer for MRW message
    mrwTimer_.add(ctrlPduRtxTimeout_, mrwDesc_.mrwSeqNum_);
    // Increment mrwSn_
    mrwDesc_.mrwSeqNum_++;
    // Send the MRW message
    bufferControlPdu(pktPdu);
}

void AmTxQueue::bufferControlPdu(cPacket *pkt) {
    bufferPdu(pkt);
}

void AmTxQueue::bufferPdu(cPacket *pktAux)
{
    Enter_Method("bufferFragmented()"); // Direct Method Call
    take(pktAux); // Take ownership

    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    EV << NOW << " AmTxQueue : Enqueuing " << pkt->getName() << " of size "
       << pkt->getByteLength() << "  for port AM_Sap_down$o\n";

    // notify MAC that new data is available
    bool needToTriggerMac = pduBuffer_.isEmpty();

    // pdu is not sent directly but queued - will be sent upon mac request
    pduBuffer_.insert(pkt);

    if (needToTriggerMac) {
        lteRlc_->indicateNewDataToMac(pkt);
    }
}

void AmTxQueue::sendPdus(int size) {
    auto pkt = pduBuffer_.front();
    if (pkt->getByteLength() <= size) {
        // next PDU does fit - pop it
        pkt = pduBuffer_.pop();

        EV << "AmTxQueue::sendPdus sending a PDU of size "
           << pkt->getByteLength() << " (total requested: " << size
           << ")" << std::endl;
    }
    else {
        // throw cRuntimeError("AmTxQueue::sendPdus cannot return current head of line PDU - size too small.");
        EV << NOW
           << " AmTxQueue::sendPdus: Cannot send PDU - PDU is larger than requested size (size == "
           << size << endl;

        // send an empty (1-bit) message to notify the MAC that there is not enough space to send RLC PDU
        auto pktCopy = check_and_cast<Packet *>(pkt->dup());
        pktCopy->setName("lteRlcFragment (empty)");
        auto rlcPdu = pktCopy->removeAtFront<LteRlcAmPdu>();
        rlcPdu->markMutableIfExclusivelyOwned();
        rlcPdu->setChunkLength(inet::b(1)); // send only a bit, minimum size
        pktCopy->insertAtFront(rlcPdu);
        pkt = pktCopy; // send modified copy; the original packet will be sent when it fits
    }

    lteRlc_->sendFragmented(pkt);

    if (!pduBuffer_.isEmpty()) {
        lteRlc_->indicateNewDataToMac(pduBuffer_.front());
    }
}

void AmTxQueue::handleControlPacket(cPacket *pkt) {
    Enter_Method("handleControlPacket()");

    take(pkt);

    auto pktPdu = check_and_cast<Packet *>(pkt);
    auto pdu = pktPdu->peekAtFront<LteRlcAmPdu>();

    // get RLC type descriptor
    short type = pdu->getAmType();

    switch (type) {
        case MRW_ACK:
            EV << NOW << " AmTxQueue::handleControlPacket , received MRW ACK ["
               << pdu->getSnoMainPacket() << "]: window new first SN  "
               << pdu->getSnoFragment() << endl;
            // move tx window
            moveTxWindow(pdu->getSnoFragment());
            // signal ACK reception
            recvMrwAck(pdu->getSnoMainPacket());
            break;

        case ACK:
            EV << NOW << " AmTxQueue::handleControlPacket , received ACK " << endl;
            recvCumulativeAck(pdu->getLastSn());

            int bSize = pdu->getBitmapArraySize();

            if (bSize > 0) {
                EV << NOW
                   << " AmTxQueue::handleControlPacket , received BITMAP ACK of size "
                   << bSize << endl;

                for (int i = 0; i < bSize; ++i) {
                    if (pdu->getBitmap(i)) {
                        recvAck(pdu->getFirstSn() + i);
                    }
                }
            }

            break;
    }

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

void AmTxQueue::recvAck(const int seqNum)
{
    int index = seqNum - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxBuffer::recvAck ACK received for sequence number "
       << seqNum << " first sequence n. [" << txWindowDesc_.firstSeqNum_
       << "] index [" << index << "] " << endl;

    if (index < 0) {
        EV << NOW
           << " AmTxBuffer::recvAck ACK already received - ignoring: index "
           << index << " first sequence number"
           << txWindowDesc_.firstSeqNum_ << endl;

        return;
    }

    if (index >= txWindowDesc_.windowSize_)
        throw cRuntimeError("AmTxBuffer::recvAck(): ACK greater than window size %d", txWindowDesc_.windowSize_);

    if (!(received_.at(index))) {
        EV << NOW << " AmTxBuffer::recvAck canceling timer for PDU "
           << (index + txWindowDesc_.firstSeqNum_) << " index " << index << endl;
        // Stop the timer
        if (pduTimer_.busy(index + txWindowDesc_.firstSeqNum_))
            pduTimer_.remove(index + txWindowDesc_.firstSeqNum_);
        // Received status variable is set to true after the
        received_.at(index) = true;
        ASSERT(pduRtxQueue_.get(index) != nullptr);
    }
}

void AmTxQueue::recvCumulativeAck(const int seqNum)
{
    // Mark the AM PDUs as received and shift the window
    if ((seqNum < txWindowDesc_.firstSeqNum_) || (seqNum < 0)) {
        // Ignore the cumulative ACK; it is out of the transmitter window (the MRW command has not yet been received by the AM rx entity)
        return;
    }
    else if ((unsigned int)seqNum > (txWindowDesc_.firstSeqNum_ + txWindowDesc_.windowSize_)) {
        throw cRuntimeError("AmTxQueue::recvCumulativeAck(): SN %d exceeds window size %d", seqNum, txWindowDesc_.windowSize_);
    }
    else {
        // The ACK is inside the window

        for (int i = 0; i <= (seqNum - txWindowDesc_.firstSeqNum_); ++i) {
            EV << NOW
               << " AmTxBuffer::recvCumulativeAck ACK received for sequence number "
               << (i + txWindowDesc_.firstSeqNum_)
               << " first sequence n. [" << txWindowDesc_.firstSeqNum_
               << "] "
               "index [" << i << "] " << endl;

            // the ACK could have already been received
            if (!(received_.at(i))) {
                // canceling timer for PDU
                EV << NOW
                   << " AmTxBuffer::recvCumulativeAck canceling timer for PDU "
                   << (i + txWindowDesc_.firstSeqNum_) << " index " << i << endl;
                // Stop the timer
                if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                    pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                // Received status variable is set to true after the
                received_.at(i) = true;
            }
        }
        checkForMrw();
    }
}

void AmTxQueue::recvMrwAck(const int seqNum)
{
    EV << NOW << " AmTxQueue::recvMrwAck for MRW command number " << seqNum << endl;

    if (mrwRtxQueue_.get(seqNum) == nullptr) {
        // The message is related to an MRW which has been discarded by the handle function because it was obsolete.
        return;
    }

    // Remove the MRW PDU from the retransmission buffer
    auto mrwPdu = check_and_cast<Packet *>(mrwRtxQueue_.remove(seqNum));

    // Stop the related timer
    if (mrwTimer_.busy(seqNum)) {
        mrwTimer_.remove(seqNum);
    }
    // deallocate the MRW PDU
    delete mrwPdu;
}

void AmTxQueue::pduTimerHandle(const int sn)
{
    Enter_Method("pduTimerHandle");
    // A timer has elapsed for the RLC PDU.
    // This function checks if the PDU has been correctly received.
    // If not, the handler checks if another transmission is possible, and
    // in this case, the PDU is placed in the retransmission buffer.
    // If no more attempts can be made, the PDU is dropped.

    int index = sn - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxQueue::pduTimerHandle - sequence number " << sn << endl;

    // signal event handling to timer
    pduTimer_.handle(sn);

    // Some debug checks
    if ((index < 0) || (index >= txWindowDesc_.windowSize_))
        throw cRuntimeError("AmTxQueue::pduTimerHandle(): The PDU [%d] for which the timer elapsed is out of the window: index [%d]", sn, index);

    if (pduRtxQueue_.get(index) == nullptr)
        throw cRuntimeError("AmTxQueue::pduTimerHandle(): PDU %d not found", index);

    // Check if the PDU has been correctly received; if so, the
    // timer should have been previously stopped.
    if (received_.at(index) == true)
        throw cRuntimeError(" AmTxQueue::pduTimerHandle(): The PDU %d [index %d] has already been received", sn, index);

    // Get the PDU information
    auto pduPkt = check_and_cast<Packet *>(pduRtxQueue_.get(index));
    auto pdu = pduPkt->peekAtFront<LteRlcAmPdu>();

    int nextTxNumber = pdu->getTxNumber() + 1;

    if (nextTxNumber > maxRtx_) {
        EV << NOW << " AmTxQueue::pduTimerHandle maximum transmissions reached; discard the PDU" << endl;
        // The maximum number of transmissions for this PDU has been
        // reached. Discard the PDU and all the PDUs related to the
        // same RLC SDU.
        discard(sn);
    }
    else {
        EV << NOW << " AmTxQueue::pduTimerHandle starting new transmission" << endl;
        // extract PDU from buffer
        auto pduPkt = check_and_cast<Packet *>(pduRtxQueue_.remove(index));
        auto pduUpd = pduPkt->removeAtFront<LteRlcAmPdu>();
        pduUpd->markMutableIfExclusivelyOwned();

        // A new transmission can be started
        pduUpd->setTxNumber(nextTxNumber);
        // The RLC PDU is added to the retransmission buffer
        pduPkt->insertAtFront(pduUpd);
        // add copy of the PDU to the rtx queue
        pduRtxQueue_.add(pduPkt->dup());
        // Reschedule the timer
        pduTimer_.add(pduRtxTimeout_, sn);
        // send down the PDU
        bufferPdu(pduPkt);
    }
}

void AmTxQueue::mrwTimerHandle(const int sn)
{
    EV << NOW << " AmTxQueue::mrwTimerHandle MRW_ACK sn: " << sn << endl;

    mrwTimer_.handle(sn);

    if (mrwRtxQueue_.get(sn) == nullptr)
        throw cRuntimeError("MRW handler: MRW of SN %d not found in MRW message queue", sn);

    // Check if a newer message has been sent
    if (mrwDesc_.lastMrw_ > sn) {
        EV << NOW << "AmTxQueue::mrwTimerHandle newer MRW has been sent - no action has to be taken" << endl;

        // A newer message has been sent
        // Delete the RLC PDU
        delete (mrwRtxQueue_.remove(sn));
    }
    else {
        EV << NOW << "AmTxQueue::mrwTimerHandle retransmitting MRW" << endl;

        auto pktPdu = check_and_cast<Packet *>(mrwRtxQueue_.remove(sn));
        auto pdu = pktPdu->peekAtFront<LteRlcAmPdu>();
        // Retransmit the MRW message
        Packet *pduCopy = pktPdu->dup();
        // Enqueue the PDU into the retransmission buffer
        mrwRtxQueue_.addAt(sn, pduCopy);
        // Retransmit the MRW control message
        mrwTimer_.add(ctrlPduRtxTimeout_, sn);
        bufferPdu(pktPdu);
    }
}

void AmTxQueue::handleMessage(cMessage *msg)
{
    if (msg->isName("timer")) {
        // message received from a timer
        TTimerMsg *tmsg = check_and_cast<TTimerMsg *>(msg);
        // check timer id
        RlcAmTimerType amType = static_cast<RlcAmTimerType>(tmsg->getTimerId());
        TTimerType type = static_cast<TTimerType>(tmsg->getType());

        switch (type) {
            case TTSIMPLE:
                // Check the buffer status and eventually send an MRW command.
                checkForMrw();
                // signal the timer the event has been handled
                bufferStatusTimer_.handle();
                break;
            case TTMULTI:

                TMultiTimerMsg *tmtmsg = check_and_cast<TMultiTimerMsg *>(tmsg);

                if (amType == PDU_T) {
                    pduTimerHandle(tmtmsg->getEvent());
                }
                else if (amType == MRW_T) {
                    mrwTimerHandle(tmtmsg->getEvent());
                }
                else
                    throw cRuntimeError("AmTxQueue::handleMessage(): unexpected timer event received");
                break;
        }
        // delete timer event message.
        delete tmsg;
    }
}

} //namespace

