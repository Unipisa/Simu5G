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

#include "simu5g/stack/pdcp/NrPdcpRxEntity.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include <inet/common/ProtocolTag_m.h>
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(NrPdcpRxEntity);



void NrPdcpRxEntity::initialize(int stage)
{
    LtePdcpRxEntity::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        reorderingEnabled_ = par("reorderingEnabled").boolValue();
        outOfOrderDelivery_ = par("outOfOrderDelivery").boolValue();
        rxWindowDesc_.windowSize_ = par("rxWindowSize");
        timeout_ = par("timeout").doubleValue();

        received_.resize(rxWindowDesc_.windowSize_, false);
        t_reordering_.setTimerId(REORDERING_T);
    }
}

void NrPdcpRxEntity::handlePdcpSdu(Packet *pdcpSdu, unsigned int sequenceNumber)
{
    Enter_Method("NrPdcpRxEntity::handlePdcpSdu");

    unsigned int rcvdSno = sequenceNumber;

    EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - processing PDCP SDU with SN[" << rcvdSno << "]" << endl;

    if (!reorderingEnabled_ || outOfOrderDelivery_) { // deliver packet to upper layer
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - Deliver SDU SN[" << rcvdSno << "] to upper layer" << endl;
        pdcpSdu->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
        deliverSduToUpperLayer(pdcpSdu);
        return;
    }
    // else dual connectivity is enabled and reordering needs to be done

    // check if already considered for reordering
    if (rcvdSno < rxWindowDesc_.rxDeliv_) {
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  was already considered for reordering. Discard the SDU" << endl;
        delete pdcpSdu;
        return;
    }

    // get the position in the buffer
    int index = rcvdSno - rxWindowDesc_.rxDeliv_;
    if (index >= rxWindowDesc_.windowSize_) {
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  is too large with respect to the window size. Advance the window and deliver out-of-sequence SDUs" << endl;
        delete pdcpSdu;
        return;
    }

    // check if already received
    if (received_.at(index)) {
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  has already been received. Discard the SDU" << endl;
        delete pdcpSdu;
        return;
    }

    // update next expected sequence number
    if (rcvdSno >= rxWindowDesc_.rxNext_)
        rxWindowDesc_.rxNext_ = rcvdSno + 1;

    if (rcvdSno == rxWindowDesc_.rxDeliv_) {
        unsigned int oldRxDeliv = rxWindowDesc_.rxDeliv_;

        // this SDU is the next one to be delivered
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - Deliver SDU SN[" << rcvdSno << "] to upper layer" << endl;
        pdcpSdu->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
        deliverSduToUpperLayer(pdcpSdu);

        rxWindowDesc_.rxDeliv_++;

        // try to deliver in-order, buffered SDUs, if any
        int pos = 1;
        while (pos < rxWindowDesc_.windowSize_ && received_.at(pos)) {
            auto *sdu = check_and_cast<Packet *>(sduBuffer_.remove(pos));
            received_.at(pos) = false;

            EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - Deliver SDU buffered at index[" << pos << "] to upper layer" << endl;
            sdu->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
            deliverSduToUpperLayer(sdu);

            rxWindowDesc_.rxDeliv_++;
            pos++;
        }

        // Shift the window down by 'pos'. The slots still holding state are those of the
        // SNs below rxNext_, i.e. old indices [pos, rxNext_ - oldRxDeliv): the bound is
        // relative to rxDeliv_ as it was before the deliveries above advanced it. Slots at
        // or above that bound are empty already -- nothing beyond rxNext_ has been received
        // -- so the vacated span needs no separate clearing pass.
        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - shifting window by " << pos << " positions" << endl;
        ASSERT(rxWindowDesc_.rxNext_ >= rxWindowDesc_.rxDeliv_);
        for (unsigned int i = pos; i < rxWindowDesc_.rxNext_ - oldRxDeliv; ++i) {
            if (sduBuffer_.get(i) != nullptr)
                sduBuffer_.addAt(i - pos, sduBuffer_.remove(i));
            received_.at(i - pos) = received_.at(i);
            received_.at(i) = false;
        }
    }
    else {
        // else, buffer SDU

        EV << NOW << " NrPdcpRxEntity::handlePdcpSdu - SDU SN[" << rcvdSno << "] received out of sequence. Buffer at index[" << index << "]" << endl;

        sduBuffer_.addAt(index, pdcpSdu);
        received_.at(index) = true;
    }

    // handle t-reordering

    // if t_reordering is running
    if (t_reordering_.busy()) {
        if (rxWindowDesc_.rxDeliv_ >= rxWindowDesc_.rxReord_)
            t_reordering_.stop();
    }
    // if t_reordering is not running
    if (!t_reordering_.busy()) {
        if (rxWindowDesc_.rxDeliv_ < rxWindowDesc_.rxNext_) {
            t_reordering_.start(timeout_);
            rxWindowDesc_.rxReord_ = rxWindowDesc_.rxNext_;
        }
    }
}

void NrPdcpRxEntity::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage()) {
        // Packet from gate — delegate to base class handler
        PdcpRxEntityBase::handleMessage(msg);
        return;
    }
    if (msg->isName("timer")) {
        t_reordering_.handle();

        EV << NOW << " NrPdcpRxEntity::handleMessage : t_reordering timer has expired " << endl;

        unsigned int old = rxWindowDesc_.rxDeliv_;

        // Every buffer index below is rxDeliv_ - old, and a valid reordering window keeps
        // it addressable: rxDeliv_ <= rxReord_ <= rxNext_ <= old + windowSize_, so the two
        // loops walk rxDeliv_ from old towards at most old + windowSize_. The bounds below
        // are the algorithm's own (stop at rxReord_, then at rxNext_ -- there is no SDU past
        // the highest one received); the ASSERTs state the window invariant those rely on,
        // so a window an upstream defect has driven out of range (e.g. two SN streams feeding
        // one entity) fails loudly here instead of silently misbehaving or tripping an opaque
        // vector range error.

        // deliver buffered SDUs up to rxReord_
        while (rxWindowDesc_.rxDeliv_ < rxWindowDesc_.rxReord_) {
            unsigned int pos = rxWindowDesc_.rxDeliv_ - old;
            ASSERT(pos < rxWindowDesc_.windowSize_);
            if (received_.at(pos) == true) {
                EV << NOW << " NrPdcpRxEntity::handleMessage - Deliver SDU buffered at index[" << pos << "] to upper layer" << endl;
                auto *sdu = check_and_cast<Packet *>(sduBuffer_.remove(pos));
                sdu->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
                deliverSduToUpperLayer(sdu);
            }
            rxWindowDesc_.rxDeliv_++;
        }

        // then any further in-sequence SDUs above rxReord_, up to rxNext_
        while (rxWindowDesc_.rxDeliv_ < rxWindowDesc_.rxNext_) {
            unsigned int pos = rxWindowDesc_.rxDeliv_ - old;
            ASSERT(pos < rxWindowDesc_.windowSize_);
            if (received_.at(pos) != true)
                break;
            EV << NOW << " NrPdcpRxEntity::handleMessage - Deliver SDU buffered at index[" << pos << "] to upper layer" << endl;
            auto *sdu = check_and_cast<Packet *>(sduBuffer_.remove(pos));
            sdu->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
            deliverSduToUpperLayer(sdu);

            rxWindowDesc_.rxDeliv_++;
        }

        // Slide the window down by 'offset', so the slot of the new rxDeliv_ becomes index 0.
        // Every slot is rewritten: those above the vacated span shift down, the rest clear.
        // Iterating over the whole window (rather than only the shifted span) is what clears
        // a window that drained completely -- offset == windowSize_ -- instead of leaving its
        // received-flags set for the next round to misread as already-received.
        unsigned int offset = rxWindowDesc_.rxDeliv_ - old;
        EV << NOW << " NrPdcpRxEntity::handleMessage - shifting window by " << offset << " positions" << endl;
        for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; ++i) {
            unsigned int src = i + offset;
            if (src < rxWindowDesc_.windowSize_) {
                if (sduBuffer_.get(src) != nullptr)
                    sduBuffer_.addAt(i, sduBuffer_.remove(src));
                received_.at(i) = received_.at(src);
            }
            else
                received_.at(i) = false;
        }

        if (rxWindowDesc_.rxNext_ > rxWindowDesc_.rxDeliv_) {
            rxWindowDesc_.rxReord_ = rxWindowDesc_.rxNext_;
            t_reordering_.start(timeout_);
        }

        delete msg;
    }
}

} //namespace

