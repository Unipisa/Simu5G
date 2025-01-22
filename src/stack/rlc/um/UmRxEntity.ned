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
// This module implements the receiving-side functionalities of the Unacknowledged
// Mode (UM) of the Radio Link Control (RLC) layer for a specific connection.
// It stores RLC Protocol Data Units (PDUs) arriving from the Medium Access
// Control (MAC) layer and, when possible, performs reassembly and in-sequence
// delivery of packets to the Packet Data Convergence Protocol (PDCP) layer.
//
simple UmRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string binderModule = default("binder");
        string umModule = default("^.um");
        string macModule = default("^.^.mac");  // TODO set to "^.^.nrMac" when parent module name is "nrRlc"
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
        int rxWindowSize = default(16);
}

