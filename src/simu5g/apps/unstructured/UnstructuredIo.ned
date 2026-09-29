//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.apps.unstructured;

//
// Attaches an application to the cellular NIC of a UE whose session is of type
// Unstructured (the UE's sessionType parameter, see ~LteUe): the payload goes down
// to the NIC and comes up from it as is, with no transport or network layer in
// between. Packets arriving on trafficIn are sent to the NIC tagged with the
// "unstructured" protocol; what the NIC delivers up as that protocol leaves on
// trafficOut. The module registers as the node's receiver of the protocol, so a UE
// has at most one of it. It is connected to the UE's network-layer dispatcher (see
// the unstructuredApp submodules of ~LteUe).
//
simple UnstructuredIo
{
    parameters:
        string interfaceTableModule;  // Relative module path of the interface table
        string interface = default("cellular");  // The name of the cellular NIC's interface
        @display("i=block/socket");
        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);
        @statistic[packetSent](title="packets sent"; source=packetSent; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[packetReceived](title="packets received"; source=packetReceived; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
    gates:
        input trafficIn;
        output trafficOut;
        input socketIn;
        output socketOut;
}
