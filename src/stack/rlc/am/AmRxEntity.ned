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
// This module implements the receiving functionalities of the Acknowledged
// Mode (AM) of the Radio Link Control (RLC) layer for a specific connection.
// It stores RLC Protocol Data Units (PDUs) that arrive from the Medium Access
// Control (MAC) layer and, when possible, performs reassembly and in-sequence
// delivery of packets to the Packet Data Convergence Protocol (PDCP) layer.
// It also acknowledges the (un)successful reception of PDUs to the peering ~AmTxEntity
// to trigger retransmissions of missing packets and provide guaranteed
// delivery of packets to the PDCP layer
//
simple AmRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int rxWindowSize = default(20);
        double ackReportInterval @unit(s) = 0.10s;
        double statusReportInterval @unit(s) = 0.20s;
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}
