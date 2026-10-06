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

#ifndef __IP2NICD2D_H_
#define __IP2NICD2D_H_

#include "simu5g/stack/ip2nic/Ip2Nic.h"

namespace simu5g {

using namespace omnetpp;

/**
 * Device-to-Device (D2D) aware version of ~Ip2Nic. It overrides the packet
 * analysis and next-hop resolution with the D2D-capable behavior formerly
 * gated by the hasD2DSupport_ flag: IP-multicast becomes D2D_MULTI with a
 * group-id assignment, unicast destinations get a D2D peer lookup, and the
 * next hop is routed directly to the peer when the peering is in DM (Direct
 * Mode). Used by the D2D-capable NICs (LteNicUeD2D, LteNicEnbD2D,
 * NrNicUeD2D, NrNicEnbD2D).
 */
class D2dBinder;

class Ip2NicD2D : public Ip2Nic
{
  protected:
    // holder of the global D2D state
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

    void initialize(int stage) override;
    /// D2D classification: multicast group, peer bookkeeping, DM/IM direction
    void classifyConnection(inet::Packet *pkt, FlowControlInfo *lteInfo, const inet::L3Address& destAddr, MacNodeId localNodeId, bool isEnb) override;

    /// all D2D-capable stacks key DRBs by the actual flow direction
    Direction bindingDirection(FlowControlInfo *lteInfo) override { return (Direction)lteInfo->getDirection(); }
    MacNodeId getNextHopNodeId(const inet::L3Address& destAddr, MacNodeId sourceId) override;
};

} //namespace

#endif
