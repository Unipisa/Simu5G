
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


package simu5g.stack.pdcp_rrc;

//
// Receive Entity of NR PDCP.
//
// This module is responsible for handling lower layer data reception as part of the PDCP layer 
// in LTE and NR networks. It processes incoming PDCP PDUs, decapsulating and forwarding them
// to the appropriate upper layer protocols within the simulation framework. This entity can be
// utilized for simulating various scenarios involving data integrity, sequence management, and 
// dual connectivity support in next-generation mobile networks. 
//
// @module @title LTE RX PDCP Entity
// @description This module represents the LTE RX entity for the PDCP (Packet Data Convergence Protocol) layer. It handles incoming packets from the RLC layer, decompresses the header, processes the PDCP SDU, and delivers data to the IP layer.
//
simple LteRxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
}
