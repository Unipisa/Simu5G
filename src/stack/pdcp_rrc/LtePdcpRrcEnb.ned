
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
// eNodeB PDCP/RRC layer of LTE stack.
// 
// This module represents the PDCP/RRC layer for an LTE eNodeB (evolved Node B). 
// It handles packet data convergence and radio resource control, ensuring efficient 
// data transfer between the eNodeB and the user equipment (UE). The module supports 
// key LTE features including Dual Connectivity, providing flexibility and performance 
// enhancements in managing LTE networks. It is a critical component for simulations 
// that require detailed eNodeB behavior in LTE networks.
//
// This module defines the PDCP and RRC layers for eNodeB in the LTE stack. 
// It extends the functionalities of the base class LtePdcpRrcBase, 
// providing specific implementations for the eNodeB context.
//
simple LtePdcpRrcEnb extends LtePdcpRrcBase
{
    @class("LtePdcpRrcEnb");
}


