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

#ifndef _NRPDCPRRCUE_H_
#define _NRPDCPRRCUE_H_

#include <omnetpp.h>

#include "stack/pdcp_rrc/LtePdcpRrcUeD2D.h"

namespace simu5g {

/**
 * @class NRPdcpRrcUe
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NRPdcpRrcUe : public LtePdcpRrcUeD2D
{
    cGate *nrTmSapOutGate_ = nullptr;
    cGate *nrUmSapOutGate_ = nullptr;
    cGate *nrAmSapOutGate_ = nullptr;

    /// Identifier for this node
    MacNodeId nrNodeId_;

    // flag for enabling Dual Connectivity
    bool dualConnectivityEnabled_;

  protected:

    void initialize(int stage) override;

    /**
     * getNrNodeId(): returns the ID of this node
     */
    MacNodeId getNrNodeId() override { return nrNodeId_; }

    // this function overrides the one in the base class because we need to distinguish the nodeId of the sender
    // i.e. whether to use the NR nodeId or the LTE one
    Direction getDirection(MacNodeId srcId, MacNodeId destId)
    {
        if (binder_->getD2DCapability(srcId, destId) && binder_->getD2DMode(srcId, destId) == DM)
            return D2D;
        return UL;
    }

    // this function was redefined so as to use the getDirection() function implemented above
    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override;

    /**
     * handler for data port
     * @param pkt incoming packet
     */
    void fromDataPort(cPacket *pkt) override;

    /**
     * getEntity() is used to gather the NR PDCP entity
     * for that LCID. If the entity was already present, a reference
     * is returned, otherwise a new entity is created,
     * added to the entities map and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the LCID of the flow
     *
     */
    LteTxPdcpEntity *getTxEntity(MacCid lcid) override;
    LteRxPdcpEntity *getRxEntity(MacCid lcid) override;

    /*
     * sendToLowerLayer() forwards a PDCP PDU to the RLC layer
     */
    void sendToLowerLayer(Packet *pkt) override;

    /*
     * Dual Connectivity support
     */
    bool isDualConnectivityEnabled() override { return dualConnectivityEnabled_; }

  public:

    void deleteEntities(MacNodeId nodeId) override;
};

} //namespace

#endif

