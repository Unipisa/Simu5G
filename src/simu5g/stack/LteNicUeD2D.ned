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
// This module represents a Device-to-Device (D2D) capable User Equipment (UE)
// network interface. It extends the basic LTE UE functionality by adding D2D
// communication capabilities, allowing direct communication between UEs without
// the need to route traffic through the base station.
//
module LteNicUeD2D extends LteNicUe
{
    parameters:
        pdcpRrc.typename = default("LtePdcpRrcUeD2D");
        mac.typename = default("LteMacUeD2D");
        phy.typename = default("LtePhyUeD2D");
        packetFlowManager.typename = default("PacketFlowManagerUe");

        d2dCapable = default(true);             // DO NOT CHANGE
        bool d2dInitialMode = default(false);
        @display("bgb=704,443");
}

