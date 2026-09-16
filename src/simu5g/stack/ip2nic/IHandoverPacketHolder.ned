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
package simu5g.stack.ip2nic;

//
// Manages packet buffering/tunnelling during handover
//
moduleinterface IHandoverPacketHolder
{
    parameters:
        @display("i=block/layer");
    gates:
        input upperLayerIn;
        input x2In @loose; // Only used in eNodeB/gNodeB nodes
        output stackOut;
        output hoManagerOut @loose; // eNodeB/gNodeB only: to forward packets to handover manager for X2 tunneling
}
