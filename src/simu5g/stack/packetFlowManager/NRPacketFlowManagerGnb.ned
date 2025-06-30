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
// NRPacketFlowManagerGnb is designed to handle packet flow management
// operations specifically for NR (New Radio) e/gNodeB in 5G simulations.
// It ensures that data packets are managed efficiently in a 5G network scenario,
// providing an accurate model of packet handling mechanisms.
//
simple NRPacketFlowManagerGnb like IPacketFlowManager
{
    @class("PacketFlowManagerEnb");

    string macModule = default("^.mac");
    string pfmType = default("NRPacketFlowManagerGnb");
}
