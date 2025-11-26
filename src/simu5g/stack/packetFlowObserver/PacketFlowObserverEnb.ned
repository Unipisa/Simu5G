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

package simu5g.stack.packetFlowObserver;

//
// This module is responsible for keeping tracking the flow of SDUs in the stack,
// and compute statistics from them. It is passive observer of the packet flow.
// Its main user is MEC services.
//
simple PacketFlowObserverEnb like IPacketFlowObserver
{
    @class("PacketFlowObserverEnb");

    string macModule = default("^.mac");
    string pfmType = default("PacketFlowObserverEnb");
}

