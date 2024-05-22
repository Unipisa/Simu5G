
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
// This module defines the receiving entity for the NR PDCP layer. It is responsible for handling
// incoming PDCP Service Data Units (SDUs), performing reordering based on sequence numbers, and 
// delivering the SDUs to the upper layer. The module supports dual connectivity, where it can 
// reorder packets before delivery when necessary. Parameters include enabling out-of-order delivery, 
// setting the timeout for the reception buffer, and configuring the size of the reception window.
//
// ALTERNATIVE:
//
// This module represents the Receive Entity of the New Radio (NR) Packet Data Convergence Protocol (PDCP) layer.
// It extends the functionalities of LteRxPdcpEntity to support NR-specific features such as:
// - Managing the reorder window for packet delivery.
// - Handling out-of-order delivery based on the configuration.
// - Implementing timers to manage reordering.
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
