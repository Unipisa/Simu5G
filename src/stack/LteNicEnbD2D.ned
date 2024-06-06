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

import simu5g.stack.d2dModeSelection.D2DModeSelection;


//
// eNodeB of LTE stack with support for D2D-capable UEs
//
module LteNicEnbD2D extends LteNicEnb
{
    parameters:
        pdcpRrc.typename = default("LtePdcpRrcEnbD2D");
        LteMacType = default("LteMacEnbD2D");
        LtePhyType = default("LtePhyEnbD2D");
        packetFlowManager.typename = default("PacketFlowManagerEnb");
        d2dCapable = default(true);   // DO NOT CHANGE
        string d2dModeSelectionType = default("D2DModeSelectionBase");

    submodules:
        d2dModeSelection: <d2dModeSelectionType> like D2DModeSelection {
            @display("p=623,314,row");
        }
}
