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

#ifndef _LTE_LTERLCUM_H_
#define _LTE_LTERLCUM_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/rlc/packet/LteRlcSdu_m.h"
#include "stack/rlc/um/UmTxEntity.h"
#include "stack/rlc/um/UmRxEntity.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/LteMacBase.h"
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

class UmTxEntity;
class UmRxEntity;

/**
 * @class LteRlcUm
 * @brief UM Module
 *
 * This is the UM Module of RLC.
 * It implements the unacknowledged mode (UM):
 *
 * - Unacknowledged mode (UM):
 *   This mode is used for data traffic. Packets arriving on
 *   this port have already been assigned a CID.
 *   UM implements fragmentation and reassembly of packets.
 *   To perform this task there is a TxEntity module for
 *   every CID = <NODE_ID,LCID>. RLC PDUs are created by the
 *   sender and reassembly is performed at the receiver by
 *   simply returning the original packet to him.
 *   Traffic on this port is then forwarded on ports
 *
 *   UM mode attaches a header to the packet. The size
 *   of this header is fixed at 2 bytes.
 *
 */
class LteRlcUm : public cSimpleModule
{
  public:

    /**
     * sendFragmented() is invoked by the TXBuffer as a direct method
     * call and is used to forward fragments to lower layers. This is needed
     * since the TXBuffer itself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendFragmented(cPacket *pkt);

    /**
     * sendDefragmented() is invoked by the RXBuffer as a direct method
     * call and is used to forward fragments to upper layers. This is needed
     * since the RXBuffer itself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendDefragmented(cPacket *pkt);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

    /**
     * sendToLowerLayer() is invoked by the TXEntity as a direct method
     * call and is used to forward fragments to lower layers. This is needed
     * since the TXBuffer itself has no output gates
     *
     * @param pkt packet to forward
     */
    virtual void sendToLowerLayer(cPacket *pkt);

    /**
     * dropBufferOverflow() is invoked by the TXEntity as a direct method
     * call and is used to drop fragments if the queue is full.
     *
     * @param pkt packet to be dropped
     */
    virtual void dropBufferOverflow(cPacket *pkt);

    virtual void resumeDownstreamInPackets(MacNodeId peerId) {}

    virtual bool isEmptyingTxBuffer(MacNodeId peerId) { return false; }

    /**
     * @author Alessandro Noferi
     * It fills the ueSet argument with the MacNodeIds that have
     * RLC data in the entities
     *
     */
    void activeUeUL(std::set<MacNodeId> *ueSet);

    /**
     * @author Alessandro Noferi
     * Methods used by RlcEntity and EnodeBCollector respectively
     * in order to manage UL throughput stats.
     */

    void addUeThroughput(MacNodeId nodeId, Throughput throughput);
    double getUeThroughput(MacNodeId nodeId);
    void resetThroughputStats(MacNodeId nodeId);

  protected:

    cGate *upInGate_ = nullptr;
    cGate *upOutGate_ = nullptr;
    cGate *downInGate_ = nullptr;
    cGate *downOutGate_ = nullptr;

    RanNodeType nodeType;

    // statistics
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;
    static simsignal_t rlcPacketLossDlSignal_;
    static simsignal_t rlcPacketLossUlSignal_;

    /**
     * Initialize watches
     */
    void initialize(int stage) override;

    void finish() override
    {
    }

    /**
     * Analyze the gate of the incoming packet
     * and call the proper handler
     */
    void handleMessage(cMessage *msg) override;

    // parameters
    bool mapAllLcidsToSingleBearer_;

    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If the TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned as well.
     *
     * @param lteInfo flow-related info
     * @return pointer to the TXBuffer for the CID of the flow
     *
     */
    virtual UmTxEntity *getTxBuffer(inet::Ptr<FlowControlInfo> lteInfo);

    /**
     * getRxBuffer() is used by the receiver to gather the RXBuffer
     * for that CID. If the RXBuffer was already present, a reference
     * is returned, otherwise a new RXBuffer is created,
     * added to the rx_buffers map and a reference is returned as well.
     *
     * @param lteInfo flow-related info
     * @return pointer to the RXBuffer for that CID
     *
     */
    virtual UmRxEntity *getRxBuffer(inet::Ptr<FlowControlInfo> lteInfo);

    /**
     * handler for traffic coming
     * from the upper layer (PDCP)
     *
     * handleUpperMessage() performs the following tasks:
     * - Adds the RLC-UM header to the packet, containing
     *   the CID, the Traffic Type and the Sequence Number
     *   of the packet (extracted from the IP Datagram)
     * - Searches (or adds) the proper TXBuffer, depending
     *   on the packet CID
     * - Calls the TXBuffer, which from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleUpperMessage(cPacket *pkt);

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * the lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following task:
     *
     * - Searches (or adds) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, which from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleLowerMessage(cPacket *pkt);

    /*
     * Data structures
     */

    /**
     * The entities map associates each CID with
     * a TX/RX Entity , identified by its ID
     */
    typedef std::map<MacCid, UmTxEntity *> UmTxEntities;
    typedef std::map<MacCid, UmRxEntity *> UmRxEntities;
    UmTxEntities txEntities_;
    UmRxEntities rxEntities_;

    /**
     * @author Alessandro Noferi
     * Holds the throughput stats for each UE
     * identified by the srcId of the
     * FlowControlInfo in each entity
     *
     */
    typedef std::map<MacNodeId, Throughput> ULThroughputPerUE;
    ULThroughputPerUE ulThroughput_;

};

} //namespace

#endif

