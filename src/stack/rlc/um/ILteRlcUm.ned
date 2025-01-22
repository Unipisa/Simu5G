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


package simu5g.stack.rlc.um;

//
// Interface module for the Unacknowledged Mode (UM) of the Radio Link Control (RLC) layer.
// It defines gates to the Packet Data Convergence Protocol (PDCP) module and the ~LteRlcMux
// module. UM enables segmentation and/or concatenation of RLC Service Data Units (SDUs) before
// transmission, according to the size of the RLC Protocol Data Unit (PDU) decided by the Medium Access
// Control (MAC) layer. On the receiving side, RLC SDUs are reassembled before being
// forwarded to the Packet Data Convergence Protocol (PDCP) layer. In-sequence delivery of
// RLC SDUs is also guaranteed.
//
moduleinterface ILteRlcUm
{
    parameters:
        int queueSize @unit(B);
        bool mapAllLcidsToSingleBearer;
    gates:
        inout UM_Sap_up;    // Gate to PDCP
        inout UM_Sap_down;    // Gate to RLC Mux
}

