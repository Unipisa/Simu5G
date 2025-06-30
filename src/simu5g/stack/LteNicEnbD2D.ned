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

import simu5g.stack.d2dModeSelection.ID2DModeSelection;

//
// The LteNicEnbD2D module represents an LTE eNodeB network interface with
// support for D2D (Device-to-Device) communications. It extends the basic
// LteNicEnb module to manage D2D-capable UEs (User Equipments). A D2D mode
// selection algorithm can be specified using the 'd2dModeSelectionType'
// parameter.
//
module LteNicEnbD2D extends LteNicEnb
{
    parameters:
        pdcpRrc.typename = default("LtePdcpRrcEnbD2D");
        mac.typename = default("LteMacEnbD2D");
        phy.typename = default("LtePhyEnbD2D");
        packetFlowManager.typename = default("PacketFlowManagerEnb");
        d2dCapable = default(true);   // DO NOT CHANGE

    submodules:
        d2dModeSelection: <default("D2DModeSelectionBase")> like ID2DModeSelection {
            @display("p=623,314,row");
        }
}
