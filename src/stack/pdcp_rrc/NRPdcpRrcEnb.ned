
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
// This module represents the PDCP/RRC layer for an NR gNodeB (Next Generation eNodeB). 
// It extends the LtePdcpRrcEnbD2D module to incorporate NR-specific features, making it a crucial 
// component in simulations involving the NR (New Radio) stack for 5G networks.
// Functions provided by this module include management of packet flows between the upper layers 
// and the lower layers, dual connectivity support, and adaptation to the NR-speciﬁc requirements 
// for data handling and delivery.
//
//
// This module represents the PDCP/RRC layer of the NR stack for the gNodeB. 
// It is responsible for handling data communication tasks like header compression,
// packet decapsulation, and delivery to the application layer. Additionally, it 
// supports dual connectivity, allowing data forwarding to and from target nodes 
// using a Dual Connectivity Manager. This module extends LtePdcpRrcEnbD2D.
//
simple NRPdcpRrcEnb extends LtePdcpRrcEnbD2D
{
    parameters:
        @class("NRPdcpRrcEnb");
        string dualConnectivityManagerModule = default("^.dualConnectivityManager");
}



