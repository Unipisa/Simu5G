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


package simu5g.stack.rlc;

//
// Interface module for the Unacknowledged Mode (UM) of the Radio Link Control (RLC) layer
// It defines gates to the Packet Data Convergence Protocol (PDCP) module and the ~LteRlcMux
// module
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

