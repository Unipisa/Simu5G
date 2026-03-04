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
// Bridges the IP network layer and the cellular protocol stack, handling bidirectional packet flow.
// Extracts flow information from IP headers and attaches it as control tags for the stack.
// Supports dual connectivity by selecting between LTE and NR paths based on Type of Service field.
// In base stations, determines the destination user equipment from IP addresses.
// In split bearer mode, distributes packets alternately across LTE and NR interfaces.
//
simple Ip2Nic like INetworkLayer2CellularNic
{
    parameters:
        string nodeType;
        string binderModule = default("binder");
        string conversationalRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string streamingRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string interactiveRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string backgroundRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        bool hasSdap = default(false);  // when true, SDAP handles DRB/RLC assignment; Ip2Nic skips traffic classification
        @display("i=block/layer");
    gates:
        // connection to network layer
        input upperLayerIn;
        output upperLayerOut;

        // connection to the cellular stack
        input stackIn;
        output stackOut;
}
