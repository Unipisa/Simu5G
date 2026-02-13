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
// Functions as a (de)multiplexer between the upper layer (PDCP) and the
// submodules of the ~LteRlc module (representing the available RLC modes: TM, UM, AM).
// Incoming packets from PDCP are dispatched to the appropriate RLC mode based on
// the RlcType field in the FlowControlInfo tag. Packets from the RLC modes are
// forwarded to PDCP.
//
simple RlcUpperLayerDispatcher
{
    parameters:
        @display("i=block/classifier");

    gates:
        //#
        //# Gates connecting to PDCP layer
        //#
        input upperLayerIn;     // from PDCP
        output upperLayerOut;   // to PDCP

        //#
        //# Gates connecting to TM/UM/AM modules
        //#
        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP
}

