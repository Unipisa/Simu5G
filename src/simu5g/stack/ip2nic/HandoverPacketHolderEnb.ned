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
package simu5g.stack.ip2nic;

//
// Manages packet buffering and forwarding during handover procedures at the base station.
// When a user equipment begins handover, tunnels downlink packets to the target base station via the backhaul interface.
// At the target base station, buffers incoming packets from both the backhaul tunnel and the network layer.
// Upon handover completion, releases buffered packets to the protocol stack in the correct order.
//
simple HandoverPacketHolderEnb like IHandoverPacketHolder
{
    parameters:
        string binderModule = default("binder");
        string handoverManagerModule = default("");
        @display("i=block/layer");
    gates:
        input upperLayerIn;
        input x2In @loose; // only used in eNodeB/gNodeB nodes
        output stackOut;
}
