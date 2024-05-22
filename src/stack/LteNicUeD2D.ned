
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
// This module represents a Device-to-Device (D2D) capable User Equipment (UE) in the LTE stack.
// It extends the basic LTE UE functionality by adding D2D communication capabilities.
// This allows direct communication between UEs without the need to route traffic through the base station.
// The module includes parameters for configuring the D2D PDCP/RRC, MAC, and PHY layers, as well as the Packet Flow Manager.
//
// Parameters:
// - LtePdcpRrcType: Specifies the type of PDCP/RRC layer for D2D communication.
// - LteMacType: Specifies the type of MAC layer for D2D communication.
// - LtePhyType: Specifies the type of PHY layer for D2D communication.
// - PacketFlowManagerType: Specifies the type of Packet Flow Manager.
// - d2dCapable: Indicates if the UE is D2D capable (default: true).
// - d2dInitialMode: Indicates if the D2D mode is enabled at the initial stage (default: false).
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

