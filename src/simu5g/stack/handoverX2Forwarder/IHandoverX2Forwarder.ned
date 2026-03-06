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

package simu5g.stack.handoverX2Forwarder;

import simu5g.x2.IX2User;

//
// Interface for the Handover X2 Forwarder. This module extends the ~X2User module interface
// to define the basic contract that any Handover X2 Forwarder must adhere to.
//
moduleinterface IHandoverX2Forwarder extends IX2User
{
    parameters:
        @display("i=block/cogwheel");
    gates:
        output tunnelOut; // data packets received via the X2 tunnel during handover
}

