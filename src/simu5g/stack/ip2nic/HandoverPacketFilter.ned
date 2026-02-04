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
// The HandoverPacketFilter module acts as a bridge between the IP network layer and the cellular (LTE/NR) protocol stack.
// It exhibits slightly different behavior when operating in the UE compared to the BS.
// Its main functionalities include:
// - Registering itself as an interface for accessibility from upper layers (e.g., IP Layer).
// - Adding control information to IP packets (5-tuple) in the downstream direction. Additionally, in the BS
//   module, it appends the destination ID to the control information to ensure packets are sent to the correct UE.
//
simple HandoverPacketFilter
{
    parameters:
        string nodeType;
        string binderModule = default("binder");
        string handoverManagerModule = default("");
        @display("i=block/layer");
    gates:
        // connection to network layer.
        input upperLayerIn;
        input x2In @loose; // only used in eNodeB/gNodeB nodes
        output stackOut;
}
