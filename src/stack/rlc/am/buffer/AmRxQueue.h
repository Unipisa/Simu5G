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

#ifndef _LTE_AMRXBUFFER_H_
#define _LTE_AMRXBUFFER_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/Packet.h>

#include "common/timer/TTimer.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/am/LteRlcAm.h"
#include "stack/rlc/am/packet/LteRlcAmPdu.h"
#include "stack/rlc/am/packet/LteRlcAmSdu_m.h"

namespace simu5g {

using namespace omnetpp;

class AmRxQueue : public cSimpleModule
{
  protected:

    // parent RLC AM module
    inet::ModuleRefByPar<LteRlcAm> lteRlc_;

    // Binder module
    inet::ModuleRefByPar<Binder> binder_;

    //! Receiver window descriptor
    RlcWindowDesc rxWindowDesc_;

    //! Minimum time between two consecutive ACK messages
    simtime_t ackReportInterval_;

    //! The time when the last ACK message was sent.
    simtime_t lastSentAck_;

    //! Buffer status report interval
    simtime_t statusReportInterval_;

    //! SDU reconstructed at the beginning of the receiver buffer
    int firstSdu_ = 0;

    //! Timer to manage the buffer status report
    TTimer timer_;

    //! AM PDU buffer
    cArray pduBuffer_;

    //! AM PDU fragment buffer
    //  (stores PDUs of the next SDU if they are shifted out of the PDU buffer before the SDU is completely
    //   received and can be passed to the upper layer)
    std::deque<inet::Packet *> pendingPduBuffer_;

    //! AM PDU received vector
    /** For each AM PDU a received status variable is kept.
     */
    std::vector<bool> received_;

    //! AM PDU discarded vector
    /** For each AM PDU a discarded status variable is kept.
     */
    std::vector<bool> discarded_;

    /*
     * FlowControlInfo matrix: used for CTRL messages generation
     */
    FlowControlInfo *flowControlInfo_ = nullptr;

    //Statistics
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalRcvdBytes_;
    Direction dir_ = UNKNOWN_DIRECTION;
    static simsignal_t rlcCellPacketLossSignal_[2];
    static simsignal_t rlcPacketLossSignal_[2];
    static simsignal_t rlcPduPacketLossSignal_[2];
    static simsignal_t rlcDelaySignal_[2];
    static simsignal_t rlcPduDelaySignal_[2];
    static simsignal_t rlcCellThroughputSignal_[2];
    static simsignal_t rlcThroughputSignal_[2];
    static simsignal_t rlcPduThroughputSignal_[2];

  public:

    AmRxQueue();

    ~AmRxQueue() override;

    //! Receive an RLC PDU from the lower layer
    void enque(inet::Packet *pdu);

    //! Send a buffer status report to the ACK manager
    void handleMessage(cMessage *msg) override;

    //initialize
    void initialize() override;

  protected:

    //! Send the RLC SDU stored in the buffer to the upper layer
    /** Note that, the buffer contains a set of RLC PDUs. At most,
     *  one RLC SDU can be in the buffer!
     */
    void deque();

    //! Pass an SDU to the upper layer (RLC receiver)
    /** @param <index> index The index of the first PDU related to
     *  the target SDU (i.e. the SDU that has been completely received)
     */
    void passUp(const int index);

    //! Check if the SDU carried in the index PDU has been
    //! completely received
    void checkCompleteSdu(const int index);

    //! Send buffer status report to the ACK manager
    void sendStatusReport();

    //! Compute the shift of the RX window
    int computeWindowShift() const;

    //! Move the RX window
    /** Shift the RX window. The number of positions to shift is
     *  given by seqNum - current RX first seqnum.
     */
    void moveRxWindow(const int seqNum);

    //! Discard out of MRW PDUs
    void discard(const int sn);

    //! Defragment received frame
    inet::Packet *defragmentFrames(std::deque<inet::Packet *>& fragmentFrames);
};

} //namespace

#endif

