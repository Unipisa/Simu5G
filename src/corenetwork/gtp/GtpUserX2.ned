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

package simu5g.corenetwork.gtp;

//
// This module implements a simplified version of the user plane of the GPRS Tunneling
// Protocol (GTP) used for handover procedures. In particular, it is employed to transfer
// data packets from the source e/gNodeB to the target e/gNodeB during a handover (i.e.,
// packets destined for a UE that is undergoing a handover).
// On one hand, this module encapsulates incoming IP datagrams from the local ~LteX2Manager
// into GTP packets and sends them over the X2 interface. On the other hand, it receives GTP
// packets from other e/gNodeBs and decapsulates the included IP datagrams, which are then
// forwarded to the local ~LteX2Manager.
//
simple GtpUserX2
{
    parameters:
        string binderModule = default("binder");

        int localPort = default(32);
        int tunnelPeerPort = default(32);

        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        output lteStackOut;
        input lteStackIn;
}
