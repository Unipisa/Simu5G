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
// This module handles the operations related to the NR PDCP (Packet Data Convergence Protocol)
// layer on the receiving side of a specific connection. It extends the ~LteRxPdcpEntity module.
// This module facilitates the creation of support for Dual Connectivity functionality. In
// particular, it manages the reordering of out-of-sequence PDCP PDUs in Dual Connectivity deployments.
//
simple NRRxPdcpEntity extends LteRxPdcpEntity
{
    parameters:
        @dynamic(true);
        @class("NRRxPdcpEntity");
        @display("i=block/segm");
        bool outOfOrderDelivery = default(false);
        double timeout @unit(s) = default(0.1s);   // Timeout for RX Buffer
        int rxWindowSize = default(4096);
}
