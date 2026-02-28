//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUM_H_
#define _LTE_LTERLCUM_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/utils/utils.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"
#include "simu5g/stack/rlc/um/UmRxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/mec/utils/MecCommon.h"

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
  protected:
    RanNodeType nodeType;
    bool hasD2DSupport_ = false;

    // statistics
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;
    static simsignal_t rlcPacketLossDlSignal_;
    static simsignal_t rlcPacketLossUlSignal_;

    // parameters
    cModuleType *txEntityModuleType_;
    cModuleType *rxEntityModuleType_;

    /*
    * Data structures
    */

    /**
    * The entities map associates each DrbKey with
    * a TX/RX Entity , identified by its ID
    */
    typedef std::map<DrbKey, UmTxEntity *> UmTxEntities;
    typedef std::map<DrbKey, UmRxEntity *> UmRxEntities;
    UmTxEntities txEntities_;
    UmRxEntities rxEntities_;

    // D2D: per-peer TX entity tracking (only used when hasD2DSupport_ is true)
    std::map<MacNodeId, std::set<UmTxEntity *, simu5g::utils::cModule_LessId>> perPeerTxEntities_;

    /**
    * @author Alessandro Noferi
    * Holds the throughput stats for each UE
    * identified by the srcId of the
    * FlowControlInfo in each entity
    *
    */
    typedef std::map<MacNodeId, Throughput> ULThroughputPerUE;
    ULThroughputPerUE ulThroughput_;

    cGate *upInGate_ = nullptr;
    cGate *upOutGate_ = nullptr;
    cGate *downInGate_ = nullptr;
    cGate *downOutGate_ = nullptr;

  public:

    /**
     * sendToUpperLayer() is invoked by the RXBuffer as a direct method
     * call and is used to forward fragments to upper layers. This is needed
     * since the RXBuffer itself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendToUpperLayer(cPacket *pkt);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    void deleteQueues(MacNodeId nodeId);

    /**
     * sendToLowerLayer() is invoked by the TXEntity as a direct method
     * call and is used to forward fragments to lower layers. This is needed
     * since the TXBuffer itself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendToLowerLayer(cPacket *pkt);

    /**
     * sendNewDataIndication() sends a new-data notification to the MAC layer.
     * Called by TXEntities after enqueuing a new SDU.
     */
    void sendNewDataIndication(cPacket *pkt);

    void resumeDownstreamInPackets(MacNodeId peerId);

    bool isEmptyingTxBuffer(MacNodeId peerId);

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

  public:
    /**
     * lookupTxBuffer() searches for an existing TXBuffer for the given DrbKey.
     *
     * @param id DrbKey to lookup
     * @return pointer to the TXBuffer if found, nullptr otherwise
     */
    UmTxEntity *lookupTxBuffer(DrbKey id);

    /**
     * createTxBuffer() creates a new TXBuffer for the given DrbKey and flow info.
     *
     * @param id DrbKey for the new buffer
     * @param lteInfo flow-related info
     * @return pointer to the newly created TXBuffer
     */
    UmTxEntity *createTxBuffer(DrbKey id, FlowControlInfo *lteInfo);


    /**
     * lookupRxBuffer() searches for an existing RXBuffer for the given DrbKey.
     *
     * @param id DrbKey to lookup
     * @return pointer to the RXBuffer if found, nullptr otherwise
     */
    UmRxEntity *lookupRxBuffer(DrbKey id);

    /**
     * createRxBuffer() creates a new RXBuffer for the given DrbKey and flow info.
     *
     * @param id DrbKey for the new buffer
     * @param lteInfo flow-related info
     * @return pointer to the newly created RXBuffer
     */
    UmRxEntity *createRxBuffer(DrbKey id, FlowControlInfo *lteInfo);

  protected:
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
    void handleUpperMessage(cPacket *pkt);

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
    void handleLowerMessage(cPacket *pkt);

};

} //namespace

#endif
