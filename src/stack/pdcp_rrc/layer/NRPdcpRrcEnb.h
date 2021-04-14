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

#ifndef _NRPDCPRRCENB_H_
#define _NRPDCPRRCENB_H_

#include <omnetpp.h>

#include "entity/NRTxPdcpEntity.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrcEnbD2D.h"
#include "stack/dualConnectivityManager/DualConnectivityManager.h"

/**
 * @class NRPdcpRrcEnb
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NRPdcpRrcEnb : public LtePdcpRrcEnbD2D
{

  protected:

    // flag for enabling Dual Connectivity
    bool dualConnectivityEnabled_;

    // reference to the Dual Connectivity Manager
    DualConnectivityManager* dualConnectivityManager_;

    virtual void initialize(int stage);

    /**
     * handler for data port
     * @param pkt incoming packet
     */
    virtual void fromDataPort(cPacket *pktAux);

    /**
     * handler for um/am sap
     *
     * it performs the following steps:
     * - decompresses the header, restoring original packet
     * - decapsulates the packet
     * - sends the packet to the application layer
     *
     * @param pkt incoming packet
     */
    virtual void fromLowerLayer(cPacket *pkt);

    virtual MacNodeId getDestId(FlowControlInfo* lteInfo);

    /**
     * getEntity() is used to gather the NR PDCP entity
     * for that LCID. If entity was already present, a reference
     * is returned, otherwise a new entity is created,
     * added to the entities map and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the LCID of the flow
     *
     */
    virtual LteTxPdcpEntity* getTxEntity(MacCid lcid);
    virtual LteRxPdcpEntity* getRxEntity(MacCid cid);

    /*
     * Dual Connectivity support
     */
    virtual bool isDualConnectivityEnabled() { return dualConnectivityEnabled_; }

    // send packet to the target node by invoking the Dual Connectivity manager
    virtual void forwardDataToTargetNode(Packet* pkt, MacNodeId targetNode);

    // receive packet from the source node. Called by the Dual Connectivity manager
    virtual void receiveDataFromSourceNode(Packet* pkt, MacNodeId sourceNode);

  public:

};

#endif
