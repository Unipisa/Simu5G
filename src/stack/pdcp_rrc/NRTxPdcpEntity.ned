
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
// This module represents the Transmit Entity for the New Radio (NR) PDCP Layer.
// It is responsible for handling the transmission tasks in the NR PDCP layer,
// such as maintaining the sequence numbering for one logical connection.
// 
// It inherits from the `LteTxPdcpEntity` class and extends its functionality 
// specifically for NR. Key operations include the delivery of PDCP PDUs to 
// lower layers and setting identification parameters for PDUs.
//
// @neddoc: This module represents the Transmit Entity of the New Radio (NR) 
// Packet Data Convergence Protocol (PDCP) within the Simu5G framework. 
// The NRTxPdcpEntity is responsible for managing the transmission-related 
// functions of the NR PDCP layer, which include handling the PDCP Packet Data 
// Units (PDUs) and ensuring their correct delivery to the lower layers or 
// across dual connectivity scenarios in an LTE/NR network. It extends the 
// functionality of the LteTxPdcpEntity.
//
simple NRTxPdcpEntity extends LteTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @class("NRTxPdcpEntity");
        @display("i=block/segm");
}

