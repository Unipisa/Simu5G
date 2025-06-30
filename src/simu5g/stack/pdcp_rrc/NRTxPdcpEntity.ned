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
// layer at the transmitting side of a specific connection. It extends the ~LteTxPdcpEntity module.
// Additionally, this module manages the creation process by adding support for Dual Connectivity functionality.
//
simple NRTxPdcpEntity extends LteTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @class("NRTxPdcpEntity");
        @display("i=block/segm");
}

