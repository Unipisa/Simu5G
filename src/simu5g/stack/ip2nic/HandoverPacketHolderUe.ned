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
// Whether the UE is attached at all is read from a mirror of RRC's stack attachment
// ledger, which RRC pushes on every handover event (see BearerManagement).
// Releases buffered packets to the protocol stack once handover completes successfully.
//
simple HandoverPacketHolderUe like IHandoverPacketHolder
{
    parameters:
        @display("i=block/layer");
    gates:
        input upperLayerIn;
        input x2In @loose; // Required by the interface, but UNUSED
        output stackOut;
        output hoManagerOut @loose; // Required by the interface, but UNUSED
}
