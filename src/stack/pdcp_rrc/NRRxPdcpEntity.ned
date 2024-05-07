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
// Receive Entity of NR PDCP
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
