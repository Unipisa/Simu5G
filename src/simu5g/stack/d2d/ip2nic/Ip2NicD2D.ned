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

package simu5g.stack.d2d.ip2nic;

import simu5g.stack.ip2nic.Ip2Nic;

//
// Device-to-Device (D2D) aware version of ~Ip2Nic. It overrides the packet
// analysis and next-hop resolution with the D2D-capable behavior: IP-multicast
// becomes a D2D_MULTI flow with a group-id assignment, unicast destinations get
// a D2D peer lookup, and the next hop is routed directly to the peer when the
// peering is in Direct Mode (DM). Selected by the D2D-capable NICs.
//
simple Ip2NicD2D extends Ip2Nic
{
    parameters:
        @class("Ip2NicD2D");
        string d2dBinderModule = default("d2dBinder");  // The network's ~D2dBinder, which holds the D2D state
}
