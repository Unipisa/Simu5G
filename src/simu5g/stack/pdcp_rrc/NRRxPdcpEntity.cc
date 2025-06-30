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

#include "stack/pdcp_rrc/NRRxPdcpEntity.h"

namespace simu5g {

Define_Module(NRRxPdcpEntity);



void NRRxPdcpEntity::initialize()
{
    outOfOrderDelivery_ = par("outOfOrderDelivery").boolValue();
    rxWindowDesc_.windowSize_ = par("rxWindowSize");
    timeout_ = par("timeout").doubleValue();

    received_.resize(rxWindowDesc_.windowSize_, false);
    t_reordering_.setTimerId(REORDERING_T);

    LteRxPdcpEntity::initialize();
}

void NRRxPdcpEntity::handlePdcpSdu(Packet *pdcpSdu)
{
    Enter_Method("NRRxPdcpEntity::handlePdcpSdu");

    auto controlInfo = pdcpSdu->getTag<FlowControlInfo>();
    unsigned int rcvdSno = controlInfo->getSequenceNumber();

    EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - processing PDCP SDU with SN[" << rcvdSno << "]" << endl;

    if (!(pdcp_->isDualConnectivityEnabled()) || outOfOrderDelivery_) { // deliver packet to upper layer
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - Deliver SDU SN[" << rcvdSno << "] to upper layer" << endl;
        pdcp_->toDataPort(pdcpSdu);
        return;
    }
    // else dual connectivity is enabled and reordering needs to be done

    // check if already considered for reordering
    if (rcvdSno < rxWindowDesc_.rxDeliv_) {
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  was already considered for reordering. Discard the SDU" << endl;
        delete pdcpSdu;
        return;
    }

    // get the position in the buffer
    int index = rcvdSno - rxWindowDesc_.rxDeliv_;
    if (index >= rxWindowDesc_.windowSize_) {
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  is too large with respect to the window size. Advance the window and deliver out-of-sequence SDUs" << endl;
        delete pdcpSdu;
        return;
    }

    // check if already received
    if (received_.at(index)) {
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - the SN[" << rcvdSno << "] <  has already been received. Discard the SDU" << endl;
        delete pdcpSdu;
        return;
    }

    // update next expected sequence number
    if (rcvdSno >= rxWindowDesc_.rxNext_)
        rxWindowDesc_.rxNext_ = rcvdSno + 1;

    if (rcvdSno == rxWindowDesc_.rxDeliv_) {
        // this SDU is the next one to be delivered
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - Deliver SDU SN[" << rcvdSno << "] to upper layer" << endl;
        pdcp_->toDataPort(pdcpSdu);

        rxWindowDesc_.rxDeliv_++;

        // try to deliver in-order, buffered SDUs, if any
        int pos = 1;
        while (pos < rxWindowDesc_.windowSize_ && received_.at(pos)) {
            cPacket *sdu = check_and_cast<cPacket *>(sduBuffer_.remove(pos));
            received_.at(pos) = false;

            EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - Deliver SDU buffered at index[" << pos << "] to upper layer" << endl;
            pdcp_->toDataPort(sdu);

            rxWindowDesc_.rxDeliv_++;
            pos++;
        }

        // shift window by 'i' positions
        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - shifting window by " << pos << " positions" << endl;
        for (unsigned int i = pos; i < rxWindowDesc_.rxNext_ - rxWindowDesc_.rxDeliv_; ++i) {
            if (sduBuffer_.get(i) != nullptr)
                sduBuffer_.addAt(i - pos, sduBuffer_.remove(i));
            received_.at(i - pos) = received_.at(i);
            received_.at(i) = false;
        }
    }
    else {
        // else, buffer SDU

        EV << NOW << " NRRxPdcpEntity::handlePdcpSdu - SDU SN[" << rcvdSno << "] received out of sequence. Buffer at index[" << index << "]" << endl;

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

void NRRxPdcpEntity::handleMessage(cMessage *msg)
{
    if (msg->isName("timer")) {
        t_reordering_.handle();

        EV << NOW << " NRRxPdcpEntity::handleMessage : t_reordering timer has expired " << endl;

        unsigned int old = rxWindowDesc_.rxDeliv_;

        // deliver buffered SDUs
        while (rxWindowDesc_.rxDeliv_ < rxWindowDesc_.rxReord_) {
            int pos = rxWindowDesc_.rxDeliv_ - old;
            if (received_.at(pos) == true) {
                EV << NOW << " NRRxPdcpEntity::handleMessage - Deliver SDU buffered at index[" << pos << "] to upper layer" << endl;
                cPacket *sdu = check_and_cast<cPacket *>(sduBuffer_.remove(pos));
                pdcp_->toDataPort(sdu);
            }
            rxWindowDesc_.rxDeliv_++;
        }

        while (received_.at(rxWindowDesc_.rxDeliv_ - old) == true) {
            EV << NOW << " NRRxPdcpEntity::handleMessage - Deliver SDU buffered at index[" << (rxWindowDesc_.rxDeliv_ - old) << "] to upper layer" << endl;
            cPacket *sdu = check_and_cast<cPacket *>(sduBuffer_.remove(rxWindowDesc_.rxDeliv_ - old));
            pdcp_->toDataPort(sdu);

            rxWindowDesc_.rxDeliv_++;
            if (rxWindowDesc_.rxDeliv_ == rxWindowDesc_.rxNext_)
                break;
        }

        // shift window by 'i' positions
        int offset = rxWindowDesc_.rxDeliv_ - old;
        EV << NOW << " NRRxPdcpEntity::handleMessage - shifting window by " << offset << " positions" << endl;
        for (unsigned int i = offset; i < rxWindowDesc_.windowSize_; ++i) {
            if (sduBuffer_.get(i) != nullptr)
                sduBuffer_.addAt(i - offset, sduBuffer_.remove(i));
            received_.at(i - offset) = received_.at(i);
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

