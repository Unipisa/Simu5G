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
// The NRPdcpRrcEnb module implements the PDCP/RRC layer for a NR NIC utilized at
// a Base Station. It extends the ~LtePdcpRrcEnbD2D module to enable Dual Connectivity
// and Split Bearer functionalities at the eNodeB module.
//
simple NRPdcpRrcEnb extends LtePdcpRrcEnbD2D
{
    parameters:
        @class("NRPdcpRrcEnb");
        string dualConnectivityManagerModule = default("^.dualConnectivityManager");
}



