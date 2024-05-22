
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
// This module implements a simplified version of the GTP-U protocol for tunneling packets over X2 interfaces.
// It is primarily used for handover procedures in LTE networks, where it facilitates the transfer of packets between GTP peers.
// The module can handle two types of packets:
// 1. LteX2Message from the X2 Manager, which encapsulates IP datagrams.
// 2. GtpUserX2Msg from UDP-IP layers, which are detunneled and sent to the X2 Manager.
//
// It leverages UDP sockets for communication and interacts with the LTE Binder module to resolve peer addresses.
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
