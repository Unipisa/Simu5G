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

#ifndef __SIMU5G_NRUMTXENTITY_H_
#define __SIMU5G_NRUMTXENTITY_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/packetFlowManager/PacketFlowManagerBase.h"
#include "simu5g/stack/rlc/um/NrRlcUm.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/mec/utils/MecCommon.h"
#include "RlcUmTransmitterBuffer.h"

using namespace omnetpp;

namespace simu5g {
class NrRlcUm;
class PacketFlowManagerBase;
/**
 * TODO - Generated class
 */
class NrUmTxEntity : public cSimpleModule
{

public:

    ~NrUmTxEntity() override;


    /*
     * Enqueues an upper layer packet into the SDU buffer
     * @param pkt the packet to be enqueued
     *
     * @return TRUE if the packet was enqueued in the SDU buffer
     */
    bool enque(inet::Packet *pkt);

    /**
     * rlcPduMake() creates a PDU having the specified size
     * and sends it to the lower layer
     *
     * @param size of a PDU
     */
    void rlcPduMake(int pduSize);

    void setFlowControlInfo(FlowControlInfo *lteInfo, MacCid cid) ;
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    // force the sequence number to assume the sno passed as an argument
    //void setNextSequenceNumber(unsigned int nextSno) { sno_ = nextSno; }




    // set holdingDownstreamInPackets_
    void startHoldingDownstreamInPackets() { holdingDownstreamInPackets_ = true; }

    // return true if the entity is not buffering in the TX queue
    bool isHoldingDownstreamInPackets();

    // store the packet in the holding buffer
    void enqueHoldingPackets(inet::cPacket *pkt);

    // resume sending packets downstream
    void resumeDownstreamInPackets();

    // return the value of notifyEmptyBuffer_
    bool isEmptyingBuffer() { return notifyEmptyBuffer_; }

    // returns true if this entity is for a D2D_MULTI connection
    bool isD2DMultiConnection() { return flowControlInfo_->getDirection() == D2D_MULTI; }

    // called when a D2D mode switch is triggered
    void rlcHandleD2DModeSwitch(bool oldConnection, bool clearBuffer = true);

protected:

    // reference to the parent's RLC layer
    inet::ModuleRefByPar<NrRlcUm> lteRlc_;

    /*
     * @author Alessandro Noferi
     *
     * reference to packetFlowManager in order to be able
     * to count discarded packets and packet delay
     *
     * Be sure to control every time if it is null, this module
     * is not mandatory for a correct network simulation.
     * It is useful, e.g., for RNI service within MEC
     */
    inet::ModuleRefByPar<PacketFlowManagerBase> packetFlowManager_;

    RlcBurstStatus burstStatus_;

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo *flowControlInfo_ = nullptr;
    MacCid infoCid_;
    /*
     * The SDU enqueue buffer.
     */



    RlcUmTransmitterBuffer* sduBuffer;

    int sn_FieldLength;

    //For debug
    std::string name_entity;

    /*
     * If true, the entity checks when the queue becomes empty
     */
    bool notifyEmptyBuffer_;

    /*
     * If true, the entity temporarily stores incoming SDUs in the holding queue (useful at D2D mode switching)
     */
    bool holdingDownstreamInPackets_;

    /*
     * The SDU holding buffer.
     */
    inet::cPacketQueue sduHoldingQueue_;



    static simsignal_t wastedGrantedBytes;
    static simsignal_t requestedPDUSizeSignal;
    static simsignal_t  sentPDUSizeSignal;
    LteMacBase* mac;

    /**
     * Initialize fragmentSize and
     * watches
     */
    void initialize() override;
    void reportBufferStatus(inet::Packet* pkt);

private:

    // Node id of the owner module
    MacNodeId ownerNodeId_;



};

} //namespace

#endif
