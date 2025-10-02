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

#ifndef _LTE_LTEPDCP_H_
#define _LTE_LTEPDCP_H_

#include <unordered_map>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/LteTxPdcpEntity.h"
#include "simu5g/stack/pdcp/LteRxPdcpEntity.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

namespace simu5g {

using namespace omnetpp;

class LteTxPdcpEntity;
class LteRxPdcpEntity;

class PacketFlowManagerBase;

struct ConnectionKey {
    inet::Ipv4Address srcAddr;
    inet::Ipv4Address dstAddr;
    uint16_t typeOfService;
    uint16_t direction;

    bool operator==(const ConnectionKey& other) const {
        return srcAddr == other.srcAddr &&
               dstAddr == other.dstAddr &&
               typeOfService == other.typeOfService &&
               direction == other.direction;
    }
};

struct ConnectionKeyHash {
    std::size_t operator()(const ConnectionKey& key) const {
        std::size_t h1 = std::hash<uint32_t>{}(key.srcAddr.getInt());
        std::size_t h2 = std::hash<uint32_t>{}(key.dstAddr.getInt());
        std::size_t h3 = std::hash<uint16_t>{}(key.typeOfService);
        std::size_t h4 = std::hash<uint16_t>{}(key.direction);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

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
class LtePdcpBase : public cSimpleModule
{
    friend class LteTxPdcpEntity;
    friend class LteRxPdcpEntity;
    friend class NrTxPdcpEntity;
    friend class NrRxPdcpEntity;
    friend class DualConnectivityManager;

  protected:
    // Modules references
    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<PacketFlowManagerBase> packetFlowManager_;
    inet::ModuleRefByPar<PacketFlowManagerBase> NRpacketFlowManager_;

    // Connection Identifier
    LogicalCid lcid_ = 1;

    // Hash Table used for CID <-> Connection mapping
    std::unordered_map<ConnectionKey, LogicalCid, ConnectionKeyHash> lcidTable_;

    // Identifier for this node
    MacNodeId nodeId_;

    // Module type for creating RX/TX PDCP entities
    cModuleType *rxEntityModuleType_ = nullptr;
    cModuleType *txEntityModuleType_ = nullptr;

    cGate *dataPortInGate_ = nullptr;
    cGate *dataPortOutGate_ = nullptr;
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

    // statistics
    static simsignal_t receivedPacketFromUpperLayerSignal_;
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;
    static simsignal_t sentPacketToLowerLayerSignal_;

  protected:

    /**
     * lookupTxEntity() searches for an existing TX PDCP entity for the given CID.
     *
     * @param cid Connection ID
     * @return pointer to the existing TX PDCP entity, or nullptr if not found
     */
    virtual LteTxPdcpEntity *lookupTxEntity(MacCid cid);

    /**
     * createTxEntity() creates a new TX PDCP entity for the given CID and adds it to the entities map.
     *
     * @param cid Connection ID
     * @return pointer to the newly created TX PDCP entity
     */
    virtual LteTxPdcpEntity *createTxEntity(MacCid cid);

    /**
     * lookupRxEntity() searches for an existing RX PDCP entity for the given CID.
     *
     * @param cid Connection ID
     * @return pointer to the existing RX PDCP entity, or nullptr if not found
     */
    virtual LteRxPdcpEntity *lookupRxEntity(MacCid cid);

    /**
     * createRxEntity() creates a new RX PDCP entity for the given CID and adds it to the entities map.
     *
     * @param cid Connection ID
     * @return pointer to the newly created RX PDCP entity
     */
    virtual LteRxPdcpEntity *createRxEntity(MacCid cid);

    /*
     * Dual Connectivity support
     */
    virtual bool isDualConnectivityEnabled() { return false; }

    virtual void forwardDataToTargetNode(inet::Packet *pkt, MacNodeId targetNode) { throw cRuntimeError("Illegal operation forwardDataToTargetNode"); }

    virtual void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode) { throw cRuntimeError("Illegal operation receiveDataFromSourceNode"); }

  public:
    /**
     * Cleans the connection table
     */
    ~LtePdcpBase() override;

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
     * getApplication(): determines the application type based on packet name
     */
    ApplicationType getApplication(cPacket *pkt);

    /**
     * getTrafficCategory(): determines the traffic category based on packet name
     */
    LteTrafficClass getTrafficCategory(cPacket *pkt);

    /**
     * getRlcType(): maps traffic category to appropriate RLC type
     */
    LteRlcType getRlcType(LteTrafficClass trafficCategory);

    /**
     * getNrNodeId(): returns the ID of this node
     */
    virtual MacNodeId getNrNodeId() { return nodeId_; }

    /**
     * lookupOrAssignLcid(): Looks up an existing LCID for the given connection key,
     * or assigns a new one if not found.
     *
     * @param key Connection key containing source/destination addresses, ToS, and direction
     * @return LogicalCid for the connection
     */
    LogicalCid lookupOrAssignLcid(const ConnectionKey& key);

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
    void setTrafficInformation(cPacket *pkt, inet::Ptr<FlowControlInfo> lteInfo);

    /*
     * Upper Layer Handlers
     */

    /**
     * Analyze the metadata of the higher-layer packet (source and destination
     * IP addresses, for example), and fills in several fields of lteInfo:
     * - sourceId, destID
     * - d2dTxDestId, d2dRxSrcId
     * - multicastGroupId (if destAddr is a multicast address)
     * - direction
     * - application, traffic
     * - rlcType
     * - LCID
     *
     * Returns the (nodeId,LCID) pair that identifies the connection for the packet.
     */
    virtual void analyzePacket(inet::Packet *pkt);

    /**
     * Process data packets from higher layers.
     *
     * @param pkt incoming packet
     */
    virtual void fromDataPort(cPacket *pkt);

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
    virtual void toDataPort(cPacket *pkt);

    /*
     * Forwarding Handlers
     */

    /*
     * sendToLowerLayer() forwards a PDCP PDU to the RLC layer
     */
    virtual void sendToLowerLayer(inet::Packet *pkt);

    virtual PacketFlowManagerBase *getPacketFlowManager() { return packetFlowManager_.getNullable(); }

};

class LtePdcpUe : public LtePdcpBase
{
  protected:

    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override
    {
        // UE is subject to handovers: master may change
        return binder_->getNextHop(nodeId_);
    }

  public:
    void initialize(int stage) override;
    void deleteEntities(MacNodeId nodeId) override;
};

class LtePdcpEnb : public LtePdcpBase
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
            master = binder_->getMasterNodeOrSelf(master);
            if (master != nodeId_) {
                destId = master;
            }
        }
        // else UE is directly attached
        return destId;
    }

  public:
    void initialize(int stage) override;
    void deleteEntities(MacNodeId nodeId) override;
};

} //namespace

#endif
