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

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/LtePdcpUeD2D.h"

namespace simu5g {

/**
 * @class NrPdcpUe
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NrPdcpUe : public LtePdcpUeD2D
{
  private:
    /// Identifier for this node
    MacNodeId nrNodeId_;

    // flag for enabling Dual Connectivity
    bool dualConnectivityEnabled_;

    cGate *nrTmSapOutGate_ = nullptr;
    cGate *nrUmSapOutGate_ = nullptr;
    cGate *nrAmSapOutGate_ = nullptr;

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
     * Analyze the packet and fill out its lteInfo.
     * @param pkt incoming packet
     */
    void analyzePacket(inet::Packet *pkt) override;

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
