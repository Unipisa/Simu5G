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
// Manages uplink packet buffering during handover at the user equipment.
// Temporarily holds packets from the network layer when handover is in progress.
// Tracks the current serving base station identifier to determine connectivity status.
// Releases buffered packets to the protocol stack once handover completes successfully.
//
simple HandoverPacketHolderUe like IHandoverPacketHolder
{
    parameters:
        string binderModule = default("binder");
        @display("i=block/layer");
    gates:
        input upperLayerIn;
        input x2In @loose; // required by the interface, but UNUSED
        output stackOut;
}
