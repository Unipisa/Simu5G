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

import simu5g.stack.rlc.am.LteRlcAm;
import simu5g.stack.rlc.tm.LteRlcTm;
import simu5g.stack.rlc.um.ILteRlcUm;


//
// The LteRlc module implements the functionalities of the Radio Link Control (RLC) layer.
// It provides mechanisms for end-to-end connectivity over the radio interface. It
// sends and receives RLC Service Data Units (SDUs) to and from the Packet Data Convergence
// Protocol (PDCP) layer, and sends and receives RLC Protocol Data Units (PDUs) to and from the
// Medium Access Control (MAC) layer. Depending on the mode, this layer can provide
// segmentation, concatenation, in-sequence delivery, and retransmission of PDUs.
// Currently, only the Unacknowledged Mode (UM) has been implemented.
//
module LteRlc
{
    parameters:
        @display("i=block/transport");
        bool d2dCapable;                                             // inherit the value from the parent module

    gates:
        //#
        //# Gates connecting PDCP/RRC and RLC Layers
        //#

        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP

        //#
        //# Gates connecting RLC and MAC Layers
        //#

        input MAC_to_RLC;    // MAC to RLC
        output RLC_to_MAC;    // RLC to MAC

    submodules:
        // TM Module
        tm: LteRlcTm {
            @display("p=100,100;");
        }

        // UM Module
        um: <(d2dCapable ? "LteRlcUmD2D" : "LteRlcUm")> like ILteRlcUm {
            @display("p=200,100;");
        }

        // AM Module
        am: LteRlcAm {
            @display("p=300,100;");
        }

           // Muxer Module
        mux: LteRlcMux {
            @display("p=200,200");
        }

    connections:
        //#
        //# Connections from RLC (up)
        //# to TM/UM/AM modules
        //#
        tm.TM_Sap_up <--> TM_Sap;
        um.UM_Sap_up <--> UM_Sap;
        am.AM_Sap_up <--> AM_Sap;

        //#
        //# Connections from TM/UM/AM
        //# modules to the RLC Mux
        //#
        tm.TM_Sap_down <--> mux.TM_Sap;
        um.UM_Sap_down <--> mux.UM_Sap;
        am.AM_Sap_down <--> mux.AM_Sap;

        //#
        //# Connections between Mux
        //# and RLC (down)
        //#
        mux.MAC_to_RLC <-- MAC_to_RLC;
        mux.RLC_to_MAC --> RLC_to_MAC;
}

