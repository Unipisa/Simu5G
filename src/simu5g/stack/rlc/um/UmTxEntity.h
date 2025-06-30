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

#ifndef _LTE_UMTXENTITY_H_
#define _LTE_UMTXENTITY_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/rlc/LteRlcDefs.h"
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

class LteRlcUm;
class PacketFlowManagerBase;

/**
 * @class UmTxEntity
 * @brief Transmission entity for UM
 *
 * This module is used to segment and/or concatenate RLC SDUs
 * in UM mode at RLC layer of the LTE stack. It operates in
 * the following way:
 *
 * - When an RLC SDU is received from the upper layer:
 *    a) the RLC SDU is buffered;
 *    b) the arrival of new data is notified to the lower layer.
 *
 * - When the lower layer requests an RLC PDU, this module invokes
 *   the rlcPduMake() function that builds a new SDU by segmenting
 *   and/or concatenating original SDUs stored in the buffer.
 *   Additional information is added to the SDU in order to allow
 *   the receiving RLC entity to rebuild the original SDUs.
 *
 * - The newly created SDU is encapsulated into an RLC PDU and sent
 *   to the lower layer.
 *
 * The size of PDUs is signaled by the lower layer.
 */
class UmTxEntity : public cSimpleModule
{
    struct FragmentInfo {
        inet::Packet *pkt = nullptr;
        int size = 0;
    };

    FragmentInfo *fragmentInfo = nullptr;

  public:

    ~UmTxEntity() override
    {
        delete fragmentInfo;
        delete flowControlInfo_;
    }

    /*
     * Enqueues an upper layer packet into the SDU buffer
     * @param pkt the packet to be enqueued
     *
     * @return TRUE if the packet was enqueued in the SDU buffer
     */
    bool enque(cPacket *pkt);

    /**
     * rlcPduMake() creates a PDU having the specified size
     * and sends it to the lower layer
     *
     * @param size of a PDU
     */
    void rlcPduMake(int pduSize);

    void setFlowControlInfo(FlowControlInfo *lteInfo) { flowControlInfo_ = lteInfo->dup(); }
    FlowControlInfo *getFlowControlInfo() { return flowControlInfo_; }

    // force the sequence number to assume the sno passed as an argument
    void setNextSequenceNumber(unsigned int nextSno) { sno_ = nextSno; }

    // remove the last SDU from the queue
    void removeDataFromQueue();

    // clear the TX buffer
    void clearQueue();

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
    inet::ModuleRefByPar<LteRlcUm> lteRlc_;

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

    /*
     * The SDU enqueue buffer.
     */
    inet::cPacketQueue sduQueue_;

    /*
     * Determine whether the first item in the queue is a fragment or a whole SDU
     */
    bool firstIsFragment_;

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

    /*
     * The maximum available queue size (in bytes)
     * (amount of data in sduQueue_ must not exceed this value)
     */
    unsigned int queueSize_;

    /*
     * The currently stored amount of data in the SDU queue (in bytes)
     */
    unsigned int queueLength_;

    /**
     * Initialize fragmentSize and
     * watches
     */
    void initialize() override;

  private:

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    /// Next PDU sequence number to be assigned
    unsigned int sno_;
};

} //namespace

#endif

