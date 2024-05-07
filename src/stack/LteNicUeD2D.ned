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


package simu5g.stack;

//
// D2D-capable User Equipment of LTE stack
//
module LteNicUeD2D extends LteNicUe
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUeD2D");
        LteMacType = default("LteMacUeD2D");
        LtePhyType = default("LtePhyUeD2D");
        PacketFlowManagerType = default("PacketFlowManagerUe");

        d2dCapable = default(true);             // DO NOT CHANGE
        bool d2dInitialMode = default(false);
        @display("bgb=704,443");
}

