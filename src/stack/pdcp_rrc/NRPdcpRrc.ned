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

import simu5g.stack.pdcp_rrc.LtePdcpRrcEnbD2D;


//
// gNodeB PDCP/RRC layer of NR stack
//
simple NRPdcpRrcEnb extends LtePdcpRrcEnbD2D {
    parameters:
        @class("NRPdcpRrcEnb");
}


//
// UE PDCP/RRC layer of NR stack
//
simple NRPdcpRrcUe extends LtePdcpRrcUeD2D {
    parameters:
        @class("NRPdcpRrcUe");
    gates:
        inout nr_DataPort;
        inout nr_EUTRAN_RRC_Sap;
        inout nr_TM_Sap;
        inout nr_UM_Sap;
        inout nr_AM_Sap;
}

// 
// Transmit Entity of NR PDCP
//
simple NRTxPdcpEntity extends LteTxPdcpEntity {
    parameters:
        @dynamic(true);
        @class("NRTxPdcpEntity");
        @display("i=block/segm");
}

// 
// Receive Entity of NR PDCP
//
simple NRRxPdcpEntity extends LteRxPdcpEntity {
    parameters:
        @dynamic(true);
        @class("NRRxPdcpEntity");
        @display("i=block/segm");
        bool outOfOrderDelivery = default(false);
        double timeout @unit(s) = default(0.1s);   // Timeout for RX Buffer
        int rxWindowSize = default(4096); 
}