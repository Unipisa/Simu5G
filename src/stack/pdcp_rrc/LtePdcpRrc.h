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

#ifndef _LTE_LTEPDCPRRC_H_
#define _LTE_LTEPDCPRRC_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "common/binder/Binder.h"
#include "common/LteCommon.h"
#include "stack/pdcp_rrc/ConnectionsTable.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/LteTxPdcpEntity.h"
#include "stack/pdcp_rrc/LteRxPdcpEntity.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"

namespace simu5g {

using namespace omnetpp;

class LteTxPdcpEntity;
class LteRxPdcpEntity;

class PacketFlowManagerBase;

#define LTE_PDCP_HEADER_COMPRESSION_DISABLED    B(-1)

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * TODO REVIEW COMMENTS
 *
 * This is the PDCP/RRC layer of the LTE Stack.
 *
 * The PDCP part performs the following tasks:
 * - Header compression/decompression
 * - association of the terminal with its eNodeB, thus storing its MacNodeId.
 *
 * The PDCP layer attaches a header to the packet. The size
 * of this header is fixed at 2 bytes.
 *
 * The RRC part performs the following actions:
 * - Binding the Local IP Address of this Terminal with
 *   its module id (MacNodeId) by informing these details
 *   to the oracle.
 * - Assign a Logical Connection IDentifier (LCID)
 *   for each connection request (coming from PDCP).
 *
 * The couple < MacNodeId, LogicalCID > constitutes the CID,
 * that uniquely identifies a connection in the whole network.
 *
 */
class LtePdcpRrcBase : public cSimpleModule
{
    friend class LteTxPdcpEntity;
    friend class LteRxPdcpEntity;
    friend class NRTxPdcpEntity;
    friend class NRRxPdcpEntity;
    friend class DualConnectivityManager;

  public:
    /**
     * Initializes the connection table
     */

    /**
     * Cleans the connection table
     */
    ~LtePdcpRrcBase() override;

    /*
     * Delete TX/RX entities (redefine this function)
     */
    virtual void deleteEntities(MacNodeId nodeId) {}

  protected:

    /**
     * Initialize class structures
     * gates, delay, compression
     * and watches
     */
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Statistics recording
     */
    void finish() override;

    /*
     * Internal functions
     */

    /**
     * getNodeId(): returns the ID of this node
     */
    MacNodeId getNodeId() { return nodeId_; }

    /**
     * getNrNodeId(): returns the ID of this node
     */
    virtual MacNodeId getNrNodeId() { return nodeId_; }

    /**
     * headerCompress(): Performs header compression.
     * At the moment, if header compression is enabled,
     * simply decrements the HEADER size by the configured
     * number of bytes
     *
     * @param Packet packet to compress
     */
    void headerCompress(inet::Packet *pkt);

    /**
     * headerDecompress(): Performs header decompression.
     * At the moment, if header compression is enabled,
     * simply restores the original packet size
     *
     * @param Packet packet to decompress
     */
    void headerDecompress(inet::Packet *pkt);

    /*
     * Functions to be implemented from derived classes
     */

    /**
     * getDestId() retrieves the id of the destination node according
     * to the following rules:
     * - On UE use masterId
     * - On ENODEB:
     *   - Use source Ip for directly attached UEs
     *
     * @param lteInfo Control Info
     */
    virtual MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) = 0;

    /**
     * getDirection() is used only on UEs and ENODEBs:
     * - direction is downlink for ENODEB
     * - direction is uplink for UE
     *
     * @return Direction of traffic
     */
    virtual Direction getDirection() = 0;
    void setTrafficInformation(cPacket *pkt, inet::Ptr<FlowControlInfo> lteInfo);

    bool isCompressionEnabled();

    /*
     * Upper Layer Handlers
     */

    /**
     * handler for data port
     *
     * fromDataPort() receives data packets from applications
     * and performs the following steps:
     * - If compression is enabled, header is compressed
     * - Reads the source port to determine if a
     *   connection for that application was already established
     *    - If it was established, sends the packet with the proper CID
     *    - Otherwise, encapsulates packet in a sap message and sends it
     *      to the RRC layer: it will find a proper CID and send the
     *      packet back to the PDCP layer
     *
     * @param pkt incoming packet
     */
    virtual void fromDataPort(cPacket *pkt);

    /**
     * handler for eutran port
     *
     * fromEutranRrcSap() receives data packets from eutran
     * and sends them on a special LCID, over TM
     *
     * @param pkt incoming packet
     */
    void fromEutranRrcSap(cPacket *pkt);

    /*
     * Lower Layer Handlers
     */

    /**
     * handler for um/am sap
     *
     * toDataPort() performs the following steps:
     * - decompresses the header, restoring the original packet
     * - decapsulates the packet
     * - sends the packet to the application layer
     *
     * @param pkt incoming packet
     */
    virtual void fromLowerLayer(cPacket *pkt);

    /**
     * toDataPort() performs the following steps:
     * - decompresses the header, restoring the original packet
     * - decapsulates the packet
     * - sends the PDCP SDU to the IP layer
     *
     * @param pkt incoming packet
     */
    void toDataPort(cPacket *pkt);

    /**
     * handler for tm sap
     *
     * toEutranRrcSap() decapsulates packet and sends it
     * over the eutran port
     *
     * @param pkt incoming packet
     */
    void toEutranRrcSap(cPacket *pkt);

    /*
     * Forwarding Handlers
     */

    /*
     * sendToLowerLayer() forwards a PDCP PDU to the RLC layer
     */
    virtual void sendToLowerLayer(inet::Packet *pkt);

    /*
     * sendToUpperLayer() forwards a PDCP SDU to the IP layer
     */
    void sendToUpperLayer(cPacket *pkt);

    /*
     * @author Alessandro Noferi
     *
     * reference to the PacketFlowManager to do notifications
     * about PDCP packets
     */
    inet::ModuleRefByPar<PacketFlowManagerBase> packetFlowManager_;
    inet::ModuleRefByPar<PacketFlowManagerBase> NRpacketFlowManager_;

    virtual PacketFlowManagerBase *getPacketFlowManager() { return packetFlowManager_.getNullable(); }

    /*
     * Data structures
     */

    /// Header size after ROHC (RObust Header Compression)
    inet::B headerCompressedSize_;

    /// Binder reference
    inet::ModuleRefByPar<Binder> binder_;

    /// Connection Identifier
    LogicalCid lcid_ = 1;

    /// Hash Table used for CID <-> Connection mapping
    ConnectionsTable ht_;

    /// Identifier for this node
    MacNodeId nodeId_;

    cGate *dataPortInGate_ = nullptr;
    cGate *dataPortOutGate_ = nullptr;
    cGate *eutranRrcSapInGate_ = nullptr;
    cGate *eutranRrcSapOutGate_ = nullptr;
    cGate *tmSapInGate_ = nullptr;
    cGate *tmSapOutGate_ = nullptr;
    cGate *umSapInGate_ = nullptr;
    cGate *umSapOutGate_ = nullptr;
    cGate *amSapInGate_ = nullptr;
    cGate *amSapOutGate_ = nullptr;

    LteRlcType conversationalRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType streamingRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType interactiveRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType backgroundRlc_ = UNKNOWN_RLC_TYPE;

    /**
     * The entities map associates each CID with a PDCP Entity, identified by its ID
     */
    typedef std::map<MacCid, LteTxPdcpEntity *> PdcpTxEntities;
    typedef std::map<MacCid, LteRxPdcpEntity *> PdcpRxEntities;
    PdcpTxEntities txEntities_;
    PdcpRxEntities rxEntities_;

    /**
     * getTxEntity() and getRxEntity() are used to gather the PDCP entity
     * for that LCID. If the entity was already present, a reference
     * is returned; otherwise, a new entity is created,
     * added to the entities map, and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the CID of the flow
     *
     */
    virtual LteTxPdcpEntity *getTxEntity(MacCid cid);

    virtual LteRxPdcpEntity *getRxEntity(MacCid cid);

    /*
     * Dual Connectivity support
     */
    virtual bool isDualConnectivityEnabled() { return false; }

    virtual void forwardDataToTargetNode(inet::Packet *pkt, MacNodeId targetNode) {}

    virtual void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode) {}

    // statistics
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;
};

class LtePdcpRrcUe : public LtePdcpRrcBase
{
  protected:

    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override
    {
        // UE is subject to handovers: master may change
        return binder_->getNextHop(nodeId_);
    }

    Direction getDirection() override
    {
        // Data coming from DataPort on UE are always Uplink
        return UL;
    }

  public:
    void initialize(int stage) override;
    void deleteEntities(MacNodeId nodeId) override;
};

class LtePdcpRrcEnb : public LtePdcpRrcBase
{
  protected:
    void handleControlInfo(cPacket *upPkt, FlowControlInfo *lteInfo)
    {
        delete lteInfo;
    }

    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override
    {
        // destination id
        MacNodeId destId = binder_->getMacNodeId(inet::Ipv4Address(lteInfo->getDstAddr()));
        // master of this UE (myself)
        MacNodeId master = binder_->getNextHop(destId);
        if (master != nodeId_) {
            destId = master;
        }
        else {
            // for dual connectivity
            master = binder_->getMasterNode(master);
            if (master != nodeId_) {
                destId = master;
            }
        }
        // else UE is directly attached
        return destId;
    }

    Direction getDirection() override
    {
        // Data coming from DataPort on ENB are always Downlink
        return DL;
    }

  public:
    void initialize(int stage) override;
    void deleteEntities(MacNodeId nodeId) override;
};

} //namespace

#endif

