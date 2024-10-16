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

package simu5g.stack.packetFlowManager;

//
// This module implements a Packet Flow Manager for User Equipment (UE) within
// an LTE/NR network. It handles the mapping and management of data packets
// across different protocol layers, such as PDCP, RLC, MAC, and PHY. The
// PacketFlowManagerUe ensures that packets are tracked, queued, and processed
// correctly for transmission and reception, thereby enabling effective communication
// within the cellular network.
//
// Parameters:
// - pfmType: Specifies the type of Packet Flow Manager (default: "PacketFlowManagerUe").
//
simple PacketFlowManagerUe like IPacketFlowManager
{
    @class("PacketFlowManagerUe");

    string macModule = default("^.mac");
    string pfmType = default("PacketFlowManagerUe");
}

