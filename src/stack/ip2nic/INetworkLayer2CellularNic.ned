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
// INetworkLayer2CellularNic module interface acts as an intermediary between the network layer 
// and the cellular (LTE/NR) protocol stack. It enables the interactions of the cellular stack with the 
// network layer, enabling the transmission of IP packets through the protocol layers of 
// the LTE and NR technologies.
//
moduleinterface INetworkLayer2CellularNic
{
    parameters:
        string nodeType;
        string interfaceTableModule;
        string routingTableModule;
        @display("i=block/layer");
    gates:
        // connection to network layer.
        // These gate names are recognized by the network layer (same as the ones in the INetworkInterface interface)
        input upperLayerIn;
        output upperLayerOut;

        // connection to the cellular stack
        inout stackNic @labels(IPDatagram);
}

