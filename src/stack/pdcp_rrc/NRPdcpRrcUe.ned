
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
// This module represents the PDCP/RRC layer for the User Equipment (UE) in the 
// New Radio (NR) stack. It handles the Packet Data Convergence Protocol (PDCP) 
// and Radio Resource Control (RRC) functionalities for NR, enabling the UE 
// to communicate efficiently in an NR network environment.
//
// ALTERNATIVE:
//
// The NRPdcpRrcUe module implements the PDCP (Packet Data Convergence Protocol)
// and RRC (Radio Resource Control) functionality for the UE (User Equipment)
// in the NR (New Radio) stack. This module handles packet forwarding to 
// the RLC (Radio Link Control) layer, supports Dual Connectivity, and manages 
// PDCP entities for packet flow. It extends the LtePdcpRrcUeD2D to include
// NR-specific functionalities.
//
simple NRPdcpRrcUe extends LtePdcpRrcUeD2D
{
    parameters:
        @class("NRPdcpRrcUe");
    gates:
        inout nr_DataPort;
        inout nr_EUTRAN_RRC_Sap;
        inout nr_TM_Sap;
        inout nr_UM_Sap;
        inout nr_AM_Sap;
}

