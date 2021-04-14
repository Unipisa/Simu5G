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
#include "common/binder/Binder.h"
#include "common/LteCommon.h"
#include "stack/pdcp_rrc/ConnectionsTable.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/layer/entity/LteTxPdcpEntity.h"
#include "stack/pdcp_rrc/layer/entity/LteRxPdcpEntity.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"

class LteTxPdcpEntity;
class LteRxPdcpEntity;


#define LTE_PDCP_HEADER_COMPRESSION_DISABLED B(-1)

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * TODO REVIEW COMMENTS
 *
 * This is the PDCP/RRC layer of LTE Stack.
 *
 * PDCP part performs the following tasks:
 * - Header compression/decompression
 * - association of the terminal with its eNodeB, thus storing its MacNodeId.
 *
 * The PDCP layer attaches an header to the packet. The size
 * of this header is fixed to 2 bytes.
 *
 * RRC part performs the following actions:
 * - Binding the Local IP Address of this Terminal with
 *   its module id (MacNodeId) by telling these informations
 *   to the oracle
 * - Assign a Logical Connection IDentifier (LCID)
 *   for each connection request (coming from PDCP)
 *
 * The couple < MacNodeId, LogicalCID > constitute the CID,
 * that uniquely identifies a connection in the whole network.
 *
 */
class LtePdcpRrcBase : public omnetpp::cSimpleModule
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
    LtePdcpRrcBase();

    /**
     * Cleans the connection table
     */
    virtual ~LtePdcpRrcBase();

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
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Statistics recording
     */
    virtual void finish() override;

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
    void headerCompress(inet::Packet* pkt);

    /**
     * headerDecompress(): Performs header decompression.
     * At the moment, if header compression is enabled,
     * simply restores original packet size
     *
     * @param Packet packet to decompress
     */
    void headerDecompress(inet::Packet* pkt);

    /*
     * Functions to be implemented from derived classes
     */

    /**
     * getDestId() retrieves the id of destination node according
     * to the following rules:
     * - On UE use masterId
     * - On ENODEB:
     *   - Use source Ip for directly attached UEs
     *
     * @param lteInfo Control Info
     */
    virtual MacNodeId getDestId(FlowControlInfo* lteInfo) = 0;

    /**
     * getDirection() is used only on UEs and ENODEBs:
     * - direction is downlink for ENODEB
     * - direction is uplink for UE
     *
     * @return Direction of traffic
     */
    virtual Direction getDirection() = 0;
    void setTrafficInformation(omnetpp::cPacket* pkt, FlowControlInfo* lteInfo);

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
    virtual void fromDataPort(omnetpp::cPacket *pkt);

    /**
     * handler for eutran port
     *
     * fromEutranRrcSap() receives data packets from eutran
     * and sends it on a special LCID, over TM
     *
     * @param pkt incoming packet
     */
    void fromEutranRrcSap(omnetpp::cPacket *pkt);

    /*
     * Lower Layer Handlers
     */

    /**
     * handler for um/am sap
     *
     * toDataPort() performs the following steps:
     * - decompresses the header, restoring original packet
     * - decapsulates the packet
     * - sends the packet to the application layer
     *
     * @param pkt incoming packet
     */
    virtual void fromLowerLayer(omnetpp::cPacket *pkt);

    /**
     * toDataPort() performs the following steps:
     * - decompresses the header, restoring original packet
     * - decapsulates the packet
     * - sends the PDCP SDU to the IP layer
     *
     * @param pkt incoming packet
     */
    void toDataPort(omnetpp::cPacket *pkt);

    /**
     * handler for tm sap
     *
     * toEutranRrcSap() decapsulates packet and sends it
     * over the eutran port
     *
     * @param pkt incoming packet
     */
    void toEutranRrcSap(omnetpp::cPacket *pkt);

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
    void sendToUpperLayer(omnetpp::cPacket *pkt);

    /*
     * Data structures
     */

    /// Header size after ROHC (RObust Header Compression)
    inet::B headerCompressedSize_;

    /// Binder reference
    Binder *binder_;

    /// Connection Identifier
    LogicalCid lcid_;

    /// Hash Table used for CID <-> Connection mapping
    ConnectionsTable* ht_;

    /// Identifier for this node
    MacNodeId nodeId_;

    omnetpp::cGate* dataPort_[2];
    omnetpp::cGate* eutranRrcSap_[2];
    omnetpp::cGate* tmSap_[2];
    omnetpp::cGate* umSap_[2];
    omnetpp::cGate* amSap_[2];

    /**
     * The entities map associate each CID with a PDCP Entity, identified by its ID
     */
    typedef std::map<MacCid, LteTxPdcpEntity*> PdcpTxEntities;
    typedef std::map<MacCid, LteRxPdcpEntity*> PdcpRxEntities;
    PdcpTxEntities txEntities_;
    PdcpRxEntities rxEntities_;

    /**
     * getTxEntity() and getRxEntity() are used to gather the PDCP entity
     * for that LCID. If entity was already present, a reference
     * is returned, otherwise a new entity is created,
     * added to the entities map and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the CID of the flow
     *
     */
    virtual LteTxPdcpEntity* getTxEntity(MacCid cid);

    virtual LteRxPdcpEntity* getRxEntity(MacCid cid);

    /*
     * Dual Connectivity support
     */
    virtual bool isDualConnectivityEnabled() { return false; }

    virtual void forwardDataToTargetNode(inet::Packet* pkt, MacNodeId targetNode) {}

    virtual void receiveDataFromSourceNode(inet::Packet* pkt, MacNodeId sourceNode) {}

    // statistics
    omnetpp::simsignal_t receivedPacketFromUpperLayer;
    omnetpp::simsignal_t receivedPacketFromLowerLayer;
    omnetpp::simsignal_t sentPacketToUpperLayer;
    omnetpp::simsignal_t sentPacketToLowerLayer;
};

class LtePdcpRrcUe : public LtePdcpRrcBase
{
  protected:

    MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // UE is subject to handovers: master may change
        return binder_->getNextHop(nodeId_);
    }

    Direction getDirection()
    {
        // Data coming from Dataport on UE are always Uplink
        return UL;
    }

  public:
    virtual void initialize(int stage);
    virtual void deleteEntities(MacNodeId nodeId);
};

class LtePdcpRrcEnb : public LtePdcpRrcBase
{
  protected:
   void handleControlInfo(omnetpp::cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        delete lteInfo;
    }

    virtual MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // dest id
        MacNodeId destId = binder_->getMacNodeId(inet::Ipv4Address(lteInfo->getDstAddr()));
        // master of this ue (myself)
        MacNodeId master = binder_->getNextHop(destId);
        if (master != nodeId_)
        {
            destId = master;
        }
        else
        {
            // for dual connectivity
            master = binder_->getMasterNode(master);
            if (master != nodeId_)
            {
                destId = master;
            }
        }
        // else ue is directly attached
        return destId;
    }

    Direction getDirection()
    {
        // Data coming from Dataport on ENB are always Downlink
        return DL;
    }
  public:
    virtual void initialize(int stage);
    virtual void deleteEntities(MacNodeId nodeId);
};

#endif
