//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.d2d.rrc;

import simu5g.stack.rrc.ConnectionControlEnb;

//
// D2D-capable variant of the ~ConnectionControlEnb. It adds the sidelink bearers of
// the UEs the base station serves: a D2D unicast or multicast flow gets a bearer
// with a fixed transitional configuration (RLC UM on LCG 3), outside the bearer
// definition system; a multicast bearer's TX leg is installed at the sender and an
// RX leg at each group member, and a node joining a group later gets the RX legs of
// the group's known senders. The sidelink node pairs' DRB identities and the
// established multicast flows are kept network-wide in the ~D2dBinder, since a
// sidelink bearer's ends are UEs whose serving base stations may differ and change.
// Selected via 'rrc.connectionControl.typename' by the D2D-capable base station NICs.
//
simple ConnectionControlEnbD2D extends ConnectionControlEnb
{
    parameters:
        @class(ConnectionControlEnbD2D);
}
