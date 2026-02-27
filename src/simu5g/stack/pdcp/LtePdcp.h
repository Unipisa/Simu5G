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

#ifndef _LTE_LTEPDCP_H_
#define _LTE_LTEPDCP_H_

#include <unordered_map>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/IPdcpGateway.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"

namespace simu5g {

using namespace omnetpp;

class PdcpTxEntityBase;
class PdcpRxEntityBase;

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
 * - Assign a Data Radio Bearer IDentifier (DRB ID)
 *   for each connection request (coming from PDCP).
 *
 * The couple < MacNodeId, DRB ID > constitutes the CID,
 * that uniquely identifies a connection in the whole network.
 *
 */
class LtePdcp : public cSimpleModule, public IPdcpGateway
{
  protected:
    // Modules references
    inet::ModuleRefByPar<Binder> binder_;

    // Identifier for this node
    MacNodeId nodeId_;

    // Flags characterizing the subclass type (set during initialize)
    bool isNR_ = false;
    bool hasD2DSupport_ = false;

    // Dual Connectivity support
    bool dualConnectivityEnabled_ = false;

    // NR node ID (for UEs with dual connectivity)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // Gate for NR RLC (NrPdcpUe only, for dual connectivity)
    cGate *nrRlcOutGate_ = nullptr;

    // Module type for creating RX/TX PDCP entities
    cModuleType *rxEntityModuleType_ = nullptr;
    cModuleType *txEntityModuleType_ = nullptr;
    cModuleType *bypassRxEntityModuleType_ = nullptr;
    cModuleType *bypassTxEntityModuleType_ = nullptr;

    cGate *upperLayerInGate_ = nullptr;
    cGate *upperLayerOutGate_ = nullptr;
    cGate *rlcInGate_ = nullptr;
    cGate *rlcOutGate_ = nullptr;

    /**
     * The entities map associates each DrbKey with a PDCP Entity, identified by its ID
     */
    typedef std::map<DrbKey, PdcpTxEntityBase *> PdcpTxEntities;
    typedef std::map<DrbKey, PdcpRxEntityBase *> PdcpRxEntities;
    PdcpTxEntities txEntities_;
    PdcpRxEntities rxEntities_;


  public:

    /**
     * lookupTxEntity() searches for an existing TX PDCP entity for the given node+DRB ID.
     *
     * @param id Node + DRB ID
     * @return pointer to the existing TX PDCP entity, or nullptr if not found
     */
    virtual PdcpTxEntityBase *lookupTxEntity(DrbKey id);

    /**
     * createTxEntity() creates a new TX PDCP entity for the given node+DRB ID and adds it to the entities map.
     *
     * @param id Node + DRB ID
     * @return pointer to the newly created TX PDCP entity
     */
    virtual PdcpTxEntityBase *createTxEntity(DrbKey id);

    /**
     * lookupRxEntity() searches for an existing RX PDCP entity for the given node+DRB ID.
     *
     * @param id Node + DRB ID
     * @return pointer to the existing RX PDCP entity, or nullptr if not found
     */
    virtual PdcpRxEntityBase *lookupRxEntity(DrbKey id);

    /**
     * createRxEntity() creates a new RX PDCP entity for the given node+DRB ID and adds it to the entities map.
     *
     * @param id Node + DRB ID
     * @return pointer to the newly created RX PDCP entity
     */
    virtual PdcpRxEntityBase *createRxEntity(DrbKey id);

    /**
     * createBypassTxEntity() creates a bypass TX entity for DC secondary nodes.
     */
    virtual PdcpTxEntityBase *createBypassTxEntity(DrbKey id);

    /**
     * createBypassRxEntity() creates a bypass RX entity for DC secondary nodes.
     */
    virtual PdcpRxEntityBase *createBypassRxEntity(DrbKey id);

  protected:
    /*
     * Dual Connectivity support
     */
    bool isDualConnectivityEnabled() { return dualConnectivityEnabled_; }

    void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode);

    // D2D mode switch handler (stub — subclasses may override for specific behavior)
    virtual void pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode);

  public:
    /**
     * Cleans the connection table
     */
    ~LtePdcp() override;

    /*
     * Delete TX/RX entities
     */
    void deleteEntities(MacNodeId nodeId);

    /*
     * Collect UE node IDs that have pending UL data in RX entities (NR ENB use)
     */
    void activeUeUL(std::set<MacNodeId> *ueSet);

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
    MacNodeId getNrNodeId() { return nrNodeId_ != NODEID_NONE ? nrNodeId_ : nodeId_; }

    /*
     * Upper Layer Handlers
     */

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

  public:
    /*
     * Thin wrappers for entities to send packets via root module gates.
     * These are pure take+send operations; signal emissions are done by entities.
     */
    void sendToUpperLayer(inet::Packet *pkt) override;
    void sendToRlc(inet::Packet *pkt) override;
    void sendToNrRlc(inet::Packet *pkt) override;
    void sendToX2(inet::Packet *pkt) override;


};

} //namespace

#endif
