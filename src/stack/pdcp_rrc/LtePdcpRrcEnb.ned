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
// The LtePdcpRrcEnb module implements the PDCP/RRC layer for an LTE NIC employed at
// eNodeBs. It performs header compression and maps incoming IP datagrams to different
// Radio Link Control (RLC) modes. Additionally, it supports Dual Connectivity and Split Bearer
// functionalities.
//
simple LtePdcpRrcEnb extends LtePdcpRrcBase
{
    @class("LtePdcpRrcEnb");
}


