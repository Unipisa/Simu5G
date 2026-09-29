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

package simu5g.stack.ethernet;

//
// The top of a UE's cellular NIC when its session is an Ethernet PDU session (the
// sessionType parameter of ~LteUe): it makes the NIC an Ethernet port of the UE's link
// layer, as INET's Ethernet interfaces are. Frames of the UE's link layer (full frames,
// MAC header and FCS) go down into the NIC's stack, where the uplink QoS rules classify
// them and ~Ip2Nic strips the FCS before SDAP; the downlink frames the NIC delivers up
// (their FCS rebuilt by ~Ip2Nic) leave with the receive tags of an Ethernet interface.
// The link layer's ~EthernetLayer filters them by destination address, unless it is
// promiscuous (a UE in bridge mode).
//
simple EthernetPortal
{
    parameters:
        @display("i=block/layer");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input lowerLayerIn;
        output lowerLayerOut;
}
