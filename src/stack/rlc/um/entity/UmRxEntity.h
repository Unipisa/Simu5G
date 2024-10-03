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
#include <inet/common/ModuleRefByPar.h>

#include "stack/rlc/um/LteRlcUm.h"
#include "common/timer/TTimer.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"

namespace simu5g {

using namespace omnetpp;

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
class UmRxEntity : public cSimpleModule
{
  public:
    UmRxEntity();
    ~UmRxEntity() override;

    /*
     * Enqueues a lower layer packet into the PDU buffer
     * @param pdu the packet to be enqueued
     */
    void enque(cPacket *pkt);

    void setFlowControlInfo(FlowControlInfo *lteInfo) { flowControlInfo_ = lteInfo->dup(); }
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    // returns true if this entity is for a D2D_MULTI connection
    bool isD2DMultiConnection() { return flowControlInfo_->getDirection() == D2D_MULTI; }

    // called when a D2D mode switch is triggered
    void rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer = true);

    // returns if the entity contains RLC pdus
    bool isEmpty() const { return buffered_.pkt == nullptr && pduBuffer_.size() == 0; }

  protected:

    /**
     * Initialize watches
     */
    void initialize() override;
    void handleMessage(cMessage *msg) override;

    //Statistics
    static unsigned int totalCellPduRcvdBytes_;
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalPduRcvdBytes_;
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

    static simsignal_t rlcPacketLossTotalSignal_;

    // statistics for D2D
    static simsignal_t rlcPacketLossD2DSignal_;
    static simsignal_t rlcPduPacketLossD2DSignal_;
    static simsignal_t rlcDelayD2DSignal_;
    static simsignal_t rlcPduDelayD2DSignal_;
    static simsignal_t rlcThroughputD2DSignal_;
    static simsignal_t rlcPduThroughputD2DSignal_;

  private:

    inet::ModuleRefByPar<Binder> binder_;

    // reference to eNB for statistic purpose
    opp_component_ptr<cModule> nodeB_;

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    inet::ModuleRefByPar<LteRlcUm> rlc_;

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo *flowControlInfo_ = nullptr;

    // The PDU enqueue buffer.
    cArray pduBuffer_;

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
        inet::Packet *pkt = nullptr;
        size_t size;
        unsigned int currentPduSno;   // next PDU sequence number expected
    } buffered_;

    // Sequence number of the last SDU delivered to the upper layer
    unsigned int lastSnoDelivered_ = 0;

    // Sequence number of the last correctly reassembled PDU
    unsigned int lastPduReassembled_ = 0;

    bool init_ = false;

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

    bool isBurst_ = false; // a burst has started last TTI
    unsigned int totalBits_ = 0; // total bytes during the burst
    unsigned int ttiBits_ = 0; // bytes during this TTI
    simtime_t t2_; // point in time the burst begins
    simtime_t t1_; // point in time last packet sent during burst

    /*
     * This method is used to manage a burst and calculate the UL throughput of a UE
     * It is called at the end of each TTI period and at the end of a t_reordering
     * period. Only the eNB needs to manage the buffer, since only it has to
     * calculate UL throughput.
     *
     * @param event specifies when it is called, i.e. after TTI or after timer reordering
     */
    void handleBurst(BurstCheck event);

    // move forward the reordering window
    void moveRxWindow(int pos);

    // consider the PDU at position 'index' for reassembly
    void reassemble(unsigned int index);

    // deliver a PDCP PDU to the PDCP layer
    void toPdcp(inet::Packet *rlcSdu);

    // clear buffered SDU
    void clearBufferedSdu();

};

} //namespace

#endif

