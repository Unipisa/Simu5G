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


package simu5g.stack.rlc.um;

//
// This module implements the transmission side functionalities of the Unacknowledged
// Mode (UM) of the Radio Link Control (RLC) layer for a specific connection.
// It stores packets coming from the Packet Data Convergence Protocol (PDCP) layer
// in a transmission buffer and, upon receiving a notification of a transmission opportunity
// from the Medium Access Control (MAC) layer, it sends down an RLC Protocol Data Unit
// (PDU) of the appropriate size (which may involve segmentation and/or concatenation).
//
simple UmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string packetFlowManagerModule = default("");
        string umModule = default("^.um");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
}

