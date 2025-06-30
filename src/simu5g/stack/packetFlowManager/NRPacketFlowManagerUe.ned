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
// This simple module represents the Packet Flow Manager for User Equipment
// (UE) in an NR (New Radio) network. It manages the flow of packets for various
// services within the UE, ensuring that data traffic is efficiently handled
// in accordance with NR specifications.
//
simple NRPacketFlowManagerUe like IPacketFlowManager
{
    @class("PacketFlowManagerUe");

    string macModule = default("^.mac");
    string pfmType = default("NRPacketFlowManagerUe");
}

