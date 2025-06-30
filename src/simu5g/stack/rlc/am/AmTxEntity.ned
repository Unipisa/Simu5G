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


package simu5g.stack.rlc.am;

//
// This module implements the transmitting functionalities of the Acknowledged
// Mode (AM) of the Radio Link Control (RLC) layer for a specific connection.
// It stores packets coming from the Packet Data Convergence Protocol (PDCP) layer
// into a transmission buffer and, upon notification of a transmission opportunity
// from the Medium Access Control (MAC) layer, sends down a RLC Protocol Data Unit
// (PDU) of the appropriate size (possibly performing segmentation and/or concatenation).
// It implements an Automatic Repeat reQuest (ARQ) mechanism to enable the retransmission
// of unacknowledged RLC PDUs
//
simple AmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
        int maxRtx = default(3);
        double pduRtxTimeout @unit(s) = default(2.0s);
        double ctrlPduRtxTimeout @unit(s) = default(2.0s);
        double bufferStatusTimeout @unit(s) = default(2.0s);
        int txWindowSize = default(16);
}

