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

#ifndef _LTE_UMRXENTITY_H_
#define _LTE_UMRXENTITY_H_

#include <omnetpp.h>
#include "stack/rlc/um/LteRlcUm.h"
#include "common/timer/TTimer.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"

class LteMacBase;
class LteRlcUm;
class LteRlcUmDataPdu;


/**
 * @class UmRxEntity
 * @brief Receiver entity for UM
 *
 * This module is used to buffer RLC PDUs and to reassemble
 * RLC SDUs in UM mode at RLC layer of the LTE stack.
 *
 * It implements the procedures described in 3GPP TS 36.322
 */
class UmRxEntity : public omnetpp::cSimpleModule
{
  public:
    UmRxEntity();
    virtual ~UmRxEntity();

    /*
     * Enqueues a lower layer packet into the PDU buffer
     * @param pdu the packet to be enqueued
     */
    void enque(omnetpp::cPacket* pkt);

    void setFlowControlInfo(FlowControlInfo* lteInfo) { flowControlInfo_ = lteInfo; }
    FlowControlInfo* getFlowControlInfo() { return flowControlInfo_; }

    // returns true if this entity is for a D2D_MULTI connection
    bool isD2DMultiConnection() { return (flowControlInfo_->getDirection() == D2D_MULTI); }

    // called when a D2D mode switch is triggered
    void rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer=true);

    // returns if the entity contains RLC pdus
    bool isEmpty() const { return (buffered_.pkt == nullptr && pduBuffer_.size() == 0);}

  protected:

    /**
     * Initialize watches
     */
    virtual void initialize();
    virtual void handleMessage(omnetpp::cMessage* msg);

    //Statistics
    static unsigned int totalCellPduRcvdBytes_;
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalPduRcvdBytes_;
    unsigned int totalRcvdBytes_;
    omnetpp::simsignal_t rlcCellPacketLoss_;
    omnetpp::simsignal_t rlcPacketLoss_;
    omnetpp::simsignal_t rlcPduPacketLoss_;
    omnetpp::simsignal_t rlcDelay_;
    omnetpp::simsignal_t rlcPduDelay_;
    omnetpp::simsignal_t rlcCellThroughput_;
    omnetpp::simsignal_t rlcThroughput_;
    omnetpp::simsignal_t rlcPduThroughput_;
    omnetpp::simsignal_t rlcPacketLossTotal_;

    // statistics for D2D
    omnetpp::simsignal_t rlcPacketLossD2D_;
    omnetpp::simsignal_t rlcPduPacketLossD2D_;
    omnetpp::simsignal_t rlcDelayD2D_;
    omnetpp::simsignal_t rlcPduDelayD2D_;
    omnetpp::simsignal_t rlcThroughputD2D_;
    omnetpp::simsignal_t rlcPduThroughputD2D_;

    // buffered fragments
    std::deque<inet::Packet *> *fragments = nullptr;
  private:

    Binder* binder_;

    // reference to eNB for statistic purpose
    omnetpp::cModule* nodeB_;

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    LteRlcUm *rlc_;

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo* flowControlInfo_;

    // The PDU enqueue buffer.
    omnetpp::cArray pduBuffer_;

    // State variables
    RlcUmRxWindowDesc rxWindowDesc_;

    // Timer to manage reordering of the PDUs
    TTimer t_reordering_;

    // Timeout for above timer
    double timeout_;

    // For each PDU a received status variable is kept.
    std::vector<bool> received_;

    // The SDU waiting for the missing portion
    struct Buffered {
         inet::Packet* pkt = nullptr;
         size_t size;
         unsigned int currentPduSno;   // next PDU sequence number expected
    } buffered_;

    // Sequence number of the last SDU delivered to the upper layer
    unsigned int lastSnoDelivered_;

    // Sequence number of the last correctly reassembled PDU
    unsigned int lastPduReassembled_;

    bool init_;

    // If true, the next PDU and the corresponding SDUs are considered in order
    // (modify the lastPduReassembled_ and lastSnoDelivered_ counters)
    // useful for D2D after a mode switch
    bool resetFlag_;


    /**
     *  @author Alessandro Noferi
     * UL throughput variables
     * From TS 136 314
     * UL data burst is the collective data received while the eNB
     * estimate of the UE buffer size is continuously above zero by
     * excluding transmission of the last piece of data.
     */

    enum BurstCheck
    {
        ENQUE, REORDERING
    };

    bool isBurst_; // a burst h started last TTI
    bool t2Set_; // used to save t2
    unsigned int totalBits_; // total bytes during the burst
    unsigned int ttiBits_; // bytes during this tti
    omnetpp::simtime_t t2_; // point in time the burst begins
    omnetpp::simtime_t t1_; // point in time last pkt sent during burst


    /*
    * This method is used to manage a burst and calculate the UL tput of a UE
    * It is called at the end of each TTI period and at the end of a t_reordering
    * period. Only the EnodeB needs to manage the buffer, since only it has to
    * calculate UL tput.
    *
    * @param event specifies when it is called, i.e after TTI or after timer reoridering
    */
    void handleBurst(BurstCheck event);





    // move forward the reordering window
    void moveRxWindow(const int pos);

    // consider the PDU at position 'index' for reassembly
    void reassemble(unsigned int index);

    // deliver a PDCP PDU to the PDCP layer
    void toPdcp(inet::Packet* rlcSdu);

    // clear buffered SDU
    void clearBufferedSdu();

};

#endif

