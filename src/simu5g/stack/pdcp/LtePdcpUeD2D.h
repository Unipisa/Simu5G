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

#ifndef _LTE_LTEPDCPRRCUED2D_H_
#define _LTE_LTEPDCPRRCUED2D_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/LtePdcp.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of the LTE Stack (with D2D support).
 *
 */
class LtePdcpUeD2D : public LtePdcpUe
{
  protected:
    // initialization flag for each D2D peer
    // it is set to true when the first IP datagram for that peer reaches the PDCP layer
    std::map<inet::L3Address, bool> d2dPeeringInit_;

  protected:

    void handleMessage(cMessage *msg) override;

    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override;

    // additional getDirection method determining if D2D communication is available to a specific destination
    Direction getDirection(MacNodeId destId)
    {
        if (binder_->getD2DCapability(nodeId_, destId) && binder_->getD2DMode(nodeId_, destId) == DM)
            return D2D;
        return UL;
    }

    /**
     * Analyze the packet and fill out its lteInfo.
     * @param pkt incoming packet
     */
    void analyzePacket(inet::Packet *pkt) override;

    // handler for mode switch signal
    void pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode);
};

} //namespace

#endif
