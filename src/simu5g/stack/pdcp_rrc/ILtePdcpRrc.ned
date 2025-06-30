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
// Interface for the PDCP/RRC layer of the LTE Stack.
//
// This module serves as an interface for the Packet Data Convergence Protocol (PDCP) layer.
// It exchanges IP datagrams with the upper layers and sends/receives PDCP Protocol Data Units (PDUs)
// to/from the underlying Radio Link Control (RLC) layer.
// Its primary responsibilities include header compression and connection identification.
//
moduleinterface ILtePdcpRrc
{
    parameters:
        @display("i=block/fork");
        int headerCompressedSize @unit(B);

    gates:
        inout DataPort;
        inout EUTRAN_RRC_Sap;
        inout TM_Sap[];
        inout UM_Sap[];
        inout AM_Sap[];
}


