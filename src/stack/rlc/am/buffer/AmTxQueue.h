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

#ifndef _LTE_AMTXBUFFER_H_
#define _LTE_AMTXBUFFER_H_

#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "common/timer/TTimer.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/am/packet/LteRlcAmPdu.h"
#include "stack/rlc/am/packet/LteRlcAmSdu_m.h"
#include "stack/rlc/am/LteRlcAm.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "inet/common/packet/Packet.h"

/*
 * RLC AM Mode Transmission Entity
 *
 * it is created by the RLC AM entity on new LCID detection
 * (and eventually destroyed on hand-over /connection close)
 *
 * enqueues upper layer PDUs (RLC SDUs) into a TX waiting buffer,
 * then fragments them and send them to MAC layer by using a
 * moving transmission window enabled with ARQ and drop-timer mechanisms.
 */

using namespace inet;
class AmTxQueue : public cSimpleModule
{
  protected:

    /*
     * reference to corresponding RLC AM module
     */
    LteRlcAm* lteRlc_;

    /*
     * SDU (upper layer PDU) currently being processed
     */
    Packet * currentSdu_ = nullptr;
    std::deque<Packet *> *fragmentList_ = nullptr;
    std::deque<int> txWindowIndexList_;

    /*
     * SDU Fragmentation descriptor
     */

    RlcFragDesc fragDesc_;
    /*
     * copy of LTE control info - used for sending down PDUs and control packets.
     */

    FlowControlInfo* lteInfo_ = nullptr;

    //--------------------------------------------------------------------------------------
    //        Buffers
    //--------------------------------------------------------------------------------------

    /*
     * The SDU enqueue buffer.
     */
    cPacketQueue sduQueue_;

    /*
     * The PDU (fragments) buffer.
     */
    cArray pduRtxQueue_;

    /*
     * The MRW PDU retransmission buffer.
     */
    cArray mrwRtxQueue_;

    /*
     * The buffer for PDUs waiting to be requested from MAC
     */
    inet::cPacketQueue pduBuffer_;

    //----------------------------------------------------------------------------------------

    // Received status variable
    std::vector<bool> received_;

    // Discarded status variable
    std::vector<bool> discarded_;

    // Transmission window descriptor
    RlcWindowDesc txWindowDesc_;

    // Move receive window command descriptor
    MrwDesc mrwDesc_;

    //-------------------------------------------------------------------------
    //                Timers and Timeouts
    //-------------------------------------------------------------------------
    // A multi-timer is kept to manage retransmissions and PDUs discard
    TMultiTimer pduTimer_;

    // A multi-timer is kept to manage MRW control messages
    TMultiTimer mrwTimer_;

    // A Generic timer is used to analyze the buffer status
    TTimer bufferStatusTimer_;

    // maximum AM retransmissions
    int maxRtx_;

    // PDU retransmission timeout
    omnetpp::simtime_t pduRtxTimeout_;

    // control PDU retransmission timeout
    omnetpp::simtime_t ctrlPduRtxTimeout_;

    // Buffer analyze timeout
    omnetpp::simtime_t bufferStatusTimeout_;

    //-------------------------------------------------------------------------

    // map of RLC Control PDU that are waiting for ACK
    std::map<int, LteRlcAmPdu *> unackedMrw_;

  public:
    AmTxQueue();
    virtual ~AmTxQueue();

    /*
     * Enqueues an upper layer packet into the transmission buffer
     * @param sdu the packet to be enqueued
     */
    void enque(Packet* sdu);

    /*
     *     Fragments current SDU (or next one, if current is completed) and adds PDUs (fragments) to the transmission buffer
     */
    void addPdus();

    /*
     * Send buffered PDUs until the specified size has been reached
     * or the buffer is empty
     *
     * @param size   maximum amount of data to be sent down
     */
    void sendPdus(int size);

    /*
     * Buffers a control pdu within the corresponding TxQueue
     */
    void bufferControlPdu(cPacket* pkt);

    /*
     * Receives a control message from the AM receiver buffer
     * @param pkt
     */
    virtual void handleControlPacket(cPacket* pkt);

  protected:

    /**
     * Initialize
     */
    virtual void initialize();
    /*
     * Analyze gate of incoming packet and call proper handler
     * @param msg
     */
    virtual void handleMessage(cMessage* msg);

    /* Discards a given RLC PDU and all the PDUs related to the same SDU
     *
     * @param seqNum the sequence number of the PDU that triggers discarding
     */
    void discard(int seqNum);

    /* buffers a PDU to be sent down to MAC when a new SDU is requested
     *
     * @param pdu PDU to be sent
     */
    void bufferPdu(cPacket *pdu);

    /* Move the transmitter window based upon reception of ACK control message
     *
     * @param seqNum
     */
    void moveTxWindow(const int seqNum);

    /* Send a MRW control message
     *
     * @param seqNum
     */
    void sendMrw(const int seqNum);

    /* Checks if receiver window has to be shifted
     */
    void checkForMrw();

    /* Receive a cumulative ACK from the transmitter ACK entity
     *
     * @param seqNum
     */
    void recvCumulativeAck(const int seqNum);

    /* Receive an ACK from the transmitter ACK entity
     *
     * @param seqNum
     */
    void recvAck(const int seqNum);

    /* Receive a MRW from the AM Receiver
     *
     * @param seqNum
     */
    void recvMrwAck(const int seqNum);

    /* timer events handlers*/
    void pduTimerHandle(const int sn);
    void mrwTimerHandle(const int sn);

    std::deque<Packet *> * fragmentFrame(Packet *frame, std::deque<int>& windowsIndex, RlcFragDesc rlcFragDesc);
};

#endif
