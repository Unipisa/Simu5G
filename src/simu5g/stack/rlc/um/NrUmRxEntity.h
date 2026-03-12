//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __SIMU5G_NRUMRXENTITY_H_
#define __SIMU5G_NRUMRXENTITY_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/rlc/um/NrRlcUm.h"
#include "NrRlcUmDataPdu.h"

#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
using namespace omnetpp;

namespace simu5g {
using namespace omnetpp;

class LteMacBase;
class NrRlcUm;
class LteRlcUmDataPdu;
/**
 * TODO - Generated class
 */
class NrUmRxEntity : public cSimpleModule
{
public:
    NrUmRxEntity();
    ~NrUmRxEntity() override;

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
    bool isEmpty() const {
        //The entity only holds PDU with segments
        return sduMap.empty();
    }


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

    static omnetpp::simsignal_t rlcThroughputSampleSignal_[2];
    omnetpp::simtime_t lastTputSample;
    unsigned int tpsample;




    inet::ModuleRefByPar<Binder> binder_;

    // reference to eNB for statistic purpose
    opp_component_ptr<cModule> nodeB_;

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    inet::ModuleRefByPar<NrRlcUm> rlc_;

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo *flowControlInfo_ = nullptr;
    //Debug
    std::string name_entity;
    // State variables

    int RX_Next_Highest;
    int RX_Next_Reassembly;
    int UM_Window_Size;
    int Rx_Timer_Trigger;
    //Map for SDU reassembly
    std::map<unsigned int, std::vector<std::pair<unsigned int, unsigned int>>> sduMap;
    cMessage* t_ReassemblyTimer;
    simtime_t t_Reassembly;

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




    // deliver a PDCP PDU to the PDCP layer
    void toPdcp(inet::Packet *rlcSdu);
    bool restartReassemblyTimer();
    void discard(bool notInWindow);
    bool completeSdu(unsigned int sduSequenceNumber, unsigned int size, bool removeIfComplete);
    bool inReassemblyWindow( int sn);
    void handlePDUInReceivedBuffer(inet::Ptr<NrRlcUmDataPdu> pdu,  int tsn);
    //Debug
    void showStateVariables();


};

} //namespace

#endif
